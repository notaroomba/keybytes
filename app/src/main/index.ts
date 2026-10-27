// Electron main process. The renderer talks to the Keyword with Web Serial and
// Web Bluetooth; Electron has no built-in device picker for either, so we forward
// the device lists to the renderer and hand back whatever the user picks.
// This side also owns the tray, the app menu, the frontmost-app watcher used for
// per-app profiles, and running "host actions" (open URL/app, run a command).
import { app, BrowserWindow, dialog, ipcMain, Menu, nativeImage, nativeTheme, shell, Tray } from 'electron';
import type { MenuItemConstructorOptions } from 'electron';
import { execFile, spawn } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import type { HostAction, MainSettings, MenuCommand, PickerDevice, TrayState } from '../shared/bridge';

const IS_MAC = process.platform === 'darwin';
const IS_WIN = process.platform === 'win32';
const dataPath = (name: string) => path.join(app.getPath('userData'), name);

// Chromium gates Web Bluetooth behind this flag on some platforms.
app.commandLine.appendSwitch('enable-experimental-web-platform-features');

if (!app.requestSingleInstanceLock()) app.quit();

let win: BrowserWindow | null = null;
let tray: Tray | null = null;
let quitting = false;
let pendingPick: { kind: 'serial' | 'bluetooth'; callback: (id: string) => void } | null = null;
let capturing = false;
let settings: Partial<MainSettings> = { closeToTray: false, perApp: false };
let trayState: TrayState = { status: 'Not connected', profiles: [], active: null, battery: null };

function finishPick(id: string) {
  if (!pendingPick) return;
  const { callback } = pendingPick;
  pendingPick = null;
  callback(id || '');
}

const send = (channel: string, ...args: unknown[]) => { if (win && !win.isDestroyed()) win.webContents.send(channel, ...args); };

function showWindow() {
  if (!win || win.isDestroyed()) return createWindow();
  if (win.isMinimized()) win.restore();
  win.show();
  win.focus();
}

// ---- Window ----

function loadBounds() {
  try {
    const b = JSON.parse(fs.readFileSync(dataPath('window.json'), 'utf8'));
    if ([b.width, b.height].every(Number.isFinite)) return b as Electron.Rectangle;
  } catch {}
  return { width: 1000, height: 700 };
}
function saveBounds() {
  if (!win || win.isDestroyed() || win.isMinimized() || win.isFullScreen()) return;
  try { fs.writeFileSync(dataPath('window.json'), JSON.stringify(win.getBounds())); } catch {}
}

function createWindow() {
  win = new BrowserWindow({
    ...loadBounds(),
    minWidth: 520,
    minHeight: 480,
    title: 'Keyword Monitor',
    backgroundColor: nativeTheme.shouldUseDarkColors ? '#14161a' : '#f5f6f8',
    show: false,
    webPreferences: {
      preload: path.join(import.meta.dirname, '../preload/index.cjs'),
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: true,
    },
  });
  const w = win;
  w.once('ready-to-show', () => w.show());
  w.on('resize', saveBounds);
  w.on('move', saveBounds);
  w.on('close', (e) => {
    if (settings.closeToTray && !quitting) {
      e.preventDefault();
      w.hide();
    }
  });

  const ses = w.webContents.session;

  ses.setPermissionCheckHandler((_wc, permission) =>
    ['serial', 'bluetooth', 'notifications'].includes(permission));
  ses.setPermissionRequestHandler((_wc, permission, cb) => cb(permission === 'notifications'));
  ses.setDevicePermissionHandler((details) =>
    ['serial', 'bluetooth'].includes(details.deviceType));

  // The Keyword's NUS needs an encrypted link. macOS pairs through its own prompt;
  // on Windows and Linux Electron asks us, and the user already picked the device.
  ses.setBluetoothPairingHandler((details, callback) => {
    if (details.pairingKind === 'providePin') callback({ confirmed: false });
    else callback({ confirmed: true });
  });

  ses.on('select-serial-port', (event, portList, _wc, callback) => {
    event.preventDefault();
    finishPick(''); // cancel any older picker
    pendingPick = { kind: 'serial', callback };
    send('chooser:list', {
      kind: 'serial',
      devices: portList.map((p): PickerDevice => ({
        id: p.portId,
        name: p.displayName || p.portName,
        detail: [p.portName, p.vendorId && `VID ${p.vendorId}`, p.productId && `PID ${p.productId}`]
          .filter(Boolean).join(' · '),
        // The Keyword enumerates as 0483:5740 (ST's VID) with the product string "Keyword".
        likely: /keyword/i.test(p.displayName || '') || ['0483', '1155'].includes(String(p.vendorId).toLowerCase()),
      })),
    });
  });
  // Web Serial needs listeners on these for navigator.serial connect/disconnect events.
  ses.on('serial-port-added', () => {});
  ses.on('serial-port-removed', () => {});

  // Fires again every time a new device is found while the scan runs.
  w.webContents.on('select-bluetooth-device', (event, devices, callback) => {
    event.preventDefault();
    if (!pendingPick || pendingPick.kind !== 'bluetooth') {
      finishPick('');
      pendingPick = { kind: 'bluetooth', callback };
    } else {
      pendingPick.callback = callback;
    }
    send('chooser:list', {
      kind: 'bluetooth',
      devices: devices.map((d): PickerDevice => ({
        id: d.deviceId,
        name: d.deviceName || 'Unnamed device',
        detail: d.deviceId,
        likely: /^keyword/i.test(d.deviceName || ''),
      })),
    });
  });

  // While the shortcut recorder is open, swallow keys before menus and the OS-level
  // accelerators (Cmd+W, Cmd+Q, Alt+F4...) see them and pass them to the page.
  w.webContents.on('before-input-event', (event, input) => {
    if (!capturing) return;
    event.preventDefault();
    send('capture:key', {
      type: input.type, code: input.code,
      ctrlKey: input.control, shiftKey: input.shift, altKey: input.alt, metaKey: input.meta,
    });
  });

  // Links in the page open in the browser, never in the app.
  w.webContents.setWindowOpenHandler(({ url }) => {
    if (/^https?:/i.test(url)) shell.openExternal(url);
    return { action: 'deny' };
  });

  // In development the page comes from electron-vite's dev server. scripts/mac-electron.sh
  // passes its URL as a switch, since `open` drops environment variables.
  const devUrl = process.env.ELECTRON_RENDERER_URL || app.commandLine.getSwitchValue('renderer-url');
  if (!app.isPackaged && devUrl) w.loadURL(devUrl);
  else w.loadFile(path.join(import.meta.dirname, '../renderer/index.html'));
}

// ---- Menu ----

function buildMenu() {
  const m = (cmd: MenuCommand) => () => send('menu', cmd);
  const profileItems = trayState.profiles.slice(0, 9).map((p, i): MenuItemConstructorOptions => ({
    label: p.name, type: 'radio', checked: p.id === trayState.active,
    accelerator: `CmdOrCtrl+${i + 1}`, click: () => send('menu', 'profile', p.id),
  }));
  const template: MenuItemConstructorOptions[] = [
    ...(IS_MAC ? [<MenuItemConstructorOptions>{
      label: app.name,
      submenu: [
        { role: 'about' }, { type: 'separator' },
        { label: 'Settings…', accelerator: 'Cmd+,', click: m('settings') },
        { type: 'separator' }, { role: 'hide' }, { role: 'hideOthers' }, { role: 'unhide' },
        { type: 'separator' }, { role: 'quit' },
      ],
    }] : []),
    {
      label: 'File',
      submenu: [
        { label: 'Connect over USB', accelerator: 'CmdOrCtrl+U', click: m('connect-usb') },
        { label: 'Connect over Bluetooth', accelerator: 'CmdOrCtrl+B', click: m('connect-ble') },
        { label: 'Disconnect', accelerator: 'CmdOrCtrl+Shift+D', click: m('disconnect') },
        { type: 'separator' },
        { label: 'Import Profiles…', accelerator: 'CmdOrCtrl+O', click: m('import') },
        { label: 'Export Profile…', accelerator: 'CmdOrCtrl+E', click: m('export') },
        { label: 'Export All Profiles…', accelerator: 'CmdOrCtrl+Shift+E', click: m('export-all') },
        { type: 'separator' },
        { label: 'Update Keyword Firmware…', click: m('update-firmware') },
        { label: 'Console', accelerator: 'CmdOrCtrl+Shift+K', click: m('console') },
        ...(IS_MAC ? [] : <MenuItemConstructorOptions[]>[{ type: 'separator' }, { label: 'Settings…', accelerator: 'Ctrl+,', click: m('settings') },
          { type: 'separator' }, { role: 'quit' }]),
      ],
    },
    {
      label: 'Edit',
      submenu: [
        // Not role: 'undo', which would only undo text fields. The page decides.
        { label: 'Undo', accelerator: 'CmdOrCtrl+Z', click: m('undo') },
        { label: 'Redo', accelerator: IS_MAC ? 'Cmd+Shift+Z' : 'Ctrl+Y', click: m('redo') },
        { type: 'separator' },
        { role: 'cut' }, { role: 'copy' }, { role: 'paste' }, { role: 'selectAll' },
        { type: 'separator' },
        { label: 'Find Key…', accelerator: 'CmdOrCtrl+F', click: m('find') },
      ],
    },
    {
      label: 'Profiles',
      submenu: [
        ...profileItems,
        { type: 'separator' },
        { label: 'Next Profile', accelerator: 'CmdOrCtrl+]', click: m('next-profile') },
        { label: 'Previous Profile', accelerator: 'CmdOrCtrl+[', click: m('prev-profile') },
        { type: 'separator' },
        { label: 'New Profile…', accelerator: 'CmdOrCtrl+N', click: m('new-profile') },
        { label: 'Profile Library…', accelerator: 'CmdOrCtrl+L', click: m('library') },
        { label: 'Send to Keyword', accelerator: 'CmdOrCtrl+Enter', click: m('apply') },
      ],
    },
    {
      label: 'View',
      submenu: [
        { label: 'Toggle Theme', accelerator: 'CmdOrCtrl+Shift+L', click: m('toggle-theme') },
        { type: 'separator' },
        { role: 'resetZoom' }, { role: 'zoomIn' }, { role: 'zoomOut' },
        { type: 'separator' }, { role: 'reload' }, { role: 'toggleDevTools' }, { role: 'togglefullscreen' },
      ],
    },
    { role: 'windowMenu' },
    {
      role: 'help',
      submenu: [{ label: 'Open Data Folder', click: () => shell.openPath(app.getPath('userData')) }],
    },
  ];
  Menu.setApplicationMenu(Menu.buildFromTemplate(template));
}

// ---- Tray / menu bar ----

// Draws a 16pt key outline (rounded square with a dot) so no image file is needed.
function trayIcon() {
  const size = 32; // @2x
  const buf = Buffer.alloc(size * size * 4);
  const [r, g, b] = IS_MAC ? [0, 0, 0] : [128, 128, 128];
  const put = (x: number, y: number, a: number) => { const i = (y * size + x) * 4; buf[i] = b; buf[i + 1] = g; buf[i + 2] = r; buf[i + 3] = a; };
  const inRounded = (x: number, y: number, x0: number, y0: number, x1: number, y1: number, rad: number) => {
    const cx = Math.min(Math.max(x, x0 + rad), x1 - rad);
    const cy = Math.min(Math.max(y, y0 + rad), y1 - rad);
    return (x - cx) ** 2 + (y - cy) ** 2 <= rad * rad;
  };
  for (let y = 0; y < size; y++) {
    for (let x = 0; x < size; x++) {
      const px = x + 0.5, py = y + 0.5;
      const outer = inRounded(px, py, 3, 5, 29, 27, 6);
      const inner = inRounded(px, py, 6, 8, 26, 24, 3.5);
      const dot = (px - 16) ** 2 + (py - 16) ** 2 <= 9;
      if ((outer && !inner) || dot) put(x, y, 255);
    }
  }
  const img = nativeImage.createFromBitmap(buf, { width: size, height: size, scaleFactor: 2 });
  if (IS_MAC) img.setTemplateImage(true);
  return img;
}

function updateTray() {
  if (!tray) {
    tray = new Tray(trayIcon());
    tray.on('click', () => { if (!IS_MAC) showWindow(); });
  }
  const { status, profiles, active, battery } = trayState;
  tray.setToolTip(`Keyword Monitor · ${status}`);
  if (IS_MAC) tray.setTitle(battery != null ? `${battery}%` : '');
  tray.setContextMenu(Menu.buildFromTemplate([
    { label: status, enabled: false },
    ...(battery != null ? [{ label: `Battery ${battery}%`, enabled: false }] : []),
    { type: 'separator' },
    ...profiles.map((p): MenuItemConstructorOptions => ({ label: p.name, type: 'radio', checked: p.id === active, click: () => send('menu', 'profile', p.id) })),
    { type: 'separator' },
    { label: 'Show Keyword Monitor', click: showWindow },
    { label: 'Quit', click: () => { quitting = true; app.quit(); } },
  ]));
}

// ---- Frontmost app, for per-app profiles ----

let watcher: { stop: () => void } | null = null;
let lastFront: string | null = null;

function reportFront(name: string) {
  name = (name || '').trim();
  if (!name || name === lastFront) return;
  lastFront = name;
  send('app:front', name);
}

function startWatcher() {
  if (watcher) return;
  if (IS_MAC) {
    // lsappinfo ships with macOS and needs no Automation permission.
    const tick = () => execFile('lsappinfo', ['front'], (err, asn) => {
      if (err || !asn.trim()) return;
      execFile('lsappinfo', ['info', '-only', 'name', asn.trim()], (err2, out) => {
        const m = !err2 && /"LSDisplayName"="([^"]*)"/.exec(out);
        if (m) reportFront(m[1]);
      });
    });
    const t = setInterval(tick, 1200);
    tick();
    watcher = { stop: () => clearInterval(t) };
  } else if (IS_WIN) {
    // One long-lived PowerShell that prints the foreground process name when it changes.
    const script = `
Add-Type @"
using System; using System.Runtime.InteropServices;
public class FG { [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
[DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint p); }
"@
$last = ""
while ($true) {
  $p = 0; [void][FG]::GetWindowThreadProcessId([FG]::GetForegroundWindow(), [ref]$p)
  try { $n = (Get-Process -Id $p).ProcessName } catch { $n = "" }
  if ($n -ne $last) { [Console]::Out.WriteLine($n); [Console]::Out.Flush(); $last = $n }
  Start-Sleep -Milliseconds 1000
}`;
    const ps = spawn('powershell.exe', ['-NoProfile', '-NonInteractive', '-Command', script], { windowsHide: true });
    ps.stdout.on('data', (d) => d.toString().split(/\r?\n/).forEach(reportFront));
    ps.on('error', () => {});
    watcher = { stop: () => ps.kill() };
  } else {
    const tick = () => execFile('xdotool', ['getactivewindow', 'getwindowclassname'], (err, out) => { if (!err) reportFront(out); });
    const t = setInterval(tick, 1200);
    watcher = { stop: () => clearInterval(t) };
  }
}

function stopWatcher() {
  watcher?.stop();
  watcher = null;
  lastFront = null;
}

// ---- Host actions ----

function runHostAction({ type, value: raw }: HostAction): { ok: boolean; error?: string } {
  let value = String(raw || '').trim();
  if (!value) return { ok: false, error: 'Nothing to run' };
  try {
    if (type === 'url') {
      if (!/^[a-z][a-z0-9+.-]*:/i.test(value)) value = `https://${value}`;
      shell.openExternal(value);
    } else if (type === 'app') {
      if (IS_MAC) spawn('open', ['-a', value], { detached: true, stdio: 'ignore' }).unref();
      else if (IS_WIN) spawn('cmd', ['/c', 'start', '', value], { detached: true, stdio: 'ignore', windowsHide: true }).unref();
      else spawn(value, [], { detached: true, stdio: 'ignore' }).unref();
    } else if (type === 'cmd') {
      spawn(value, { shell: true, detached: true, stdio: 'ignore', windowsHide: true }).unref();
    } else {
      return { ok: false, error: `Unknown action ${type}` };
    }
    return { ok: true };
  } catch (e) {
    return { ok: false, error: (e as Error).message };
  }
}

// ---- IPC ----

ipcMain.on('chooser:pick', (_e, id: string) => finishPick(id));
ipcMain.on('capture:set', (_e, on: boolean) => { capturing = !!on; });

ipcMain.handle('store:load', () => {
  try { return JSON.parse(fs.readFileSync(dataPath('profiles.json'), 'utf8')); } catch { return null; }
});
ipcMain.handle('store:save', (_e, store) => {
  const tmp = dataPath('profiles.json.tmp');
  fs.writeFileSync(tmp, JSON.stringify(store, null, 2));
  fs.renameSync(tmp, dataPath('profiles.json'));
});
ipcMain.handle('file:save', async (_e, defaultName: string, text: string) => {
  const { canceled, filePath } = await dialog.showSaveDialog(win!, {
    title: 'Export',
    defaultPath: defaultName.replace(/[^\w .-]+/g, '_') || 'profile.keyword.json',
    filters: [{ name: 'Keyword profile', extensions: ['json'] }],
  });
  if (canceled || !filePath) return false;
  fs.writeFileSync(filePath, text);
  return true;
});
ipcMain.handle('file:open', async () => {
  const { canceled, filePaths } = await dialog.showOpenDialog(win!, {
    title: 'Import profiles',
    filters: [{ name: 'Keyword profile', extensions: ['json'] }],
    properties: ['openFile', 'multiSelections'],
  });
  if (canceled || !filePaths.length) return [];
  return filePaths.map((p) => ({ name: path.basename(p), text: fs.readFileSync(p, 'utf8') }));
});
ipcMain.handle('host:run', (_e, action: HostAction) => runHostAction(action));
ipcMain.handle('app:info', () => ({ version: app.getVersion(), platform: process.platform }));
ipcMain.on('open:data', () => shell.openPath(app.getPath('userData')));

ipcMain.on('settings:apply', (_e, s: MainSettings) => {
  settings = { ...settings, ...s };
  nativeTheme.themeSource = ['light', 'dark'].includes(s.theme) ? s.theme : 'system';
  if (!process.env.KEYWORD_NO_LOGIN_ITEM && (IS_MAC || IS_WIN)) {
    try { app.setLoginItemSettings({ openAtLogin: !!s.launchAtLogin }); } catch {}
  }
  if (s.perApp) startWatcher(); else stopWatcher();
});

ipcMain.on('state:update', (_e, s: TrayState) => {
  const profilesChanged = JSON.stringify(s.profiles) !== JSON.stringify(trayState.profiles) || s.active !== trayState.active;
  trayState = s;
  updateTray();
  if (profilesChanged) buildMenu();
});

// ---- App lifecycle ----

app.on('second-instance', showWindow);

app.whenReady().then(() => {
  buildMenu();
  createWindow();
  updateTray();
  app.on('activate', showWindow);
});

app.on('before-quit', () => { quitting = true; });

app.on('window-all-closed', () => {
  if (!IS_MAC && !settings.closeToTray) app.quit();
});
