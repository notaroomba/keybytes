// Electron main process. The renderer talks to the Keyword with Web Serial and
// Web Bluetooth; Electron has no built-in device picker for either, so we forward
// the device lists to the renderer and hand back whatever the user picks.
const { app, BrowserWindow, ipcMain, dialog } = require('electron');
const fs = require('fs');
const path = require('path');

const storePath = () => path.join(app.getPath('userData'), 'profiles.json');

// Chromium gates Web Bluetooth behind this flag on some platforms.
app.commandLine.appendSwitch('enable-experimental-web-platform-features');

let win;
let pendingPick = null; // { kind, callback }
let capturing = false;

function finishPick(id) {
  if (!pendingPick) return;
  const { callback } = pendingPick;
  pendingPick = null;
  callback(id || '');
}

function createWindow() {
  win = new BrowserWindow({
    width: 900,
    height: 640,
    minWidth: 520,
    minHeight: 480,
    title: 'Keyword Monitor',
    backgroundColor: '#14161a',
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      contextIsolation: true,
      nodeIntegration: false,
    },
  });

  const ses = win.webContents.session;

  ses.setPermissionCheckHandler((_wc, permission) =>
    ['serial', 'bluetooth'].includes(permission));
  ses.setDevicePermissionHandler((details) =>
    ['serial', 'bluetooth'].includes(details.deviceType));

  ses.on('select-serial-port', (event, portList, _wc, callback) => {
    event.preventDefault();
    finishPick(''); // cancel any older picker
    pendingPick = { kind: 'serial', callback };
    win.webContents.send('chooser:list', {
      kind: 'serial',
      devices: portList.map((p) => ({
        id: p.portId,
        name: p.displayName || p.portName,
        detail: [p.portName, p.vendorId && `VID ${p.vendorId}`, p.productId && `PID ${p.productId}`]
          .filter(Boolean).join(' · '),
        // 0x0483 is ST's USB vendor id, so it's most likely the Keyword.
        likely: String(p.vendorId).toLowerCase() === '0483' || p.vendorId === '1155',
      })),
    });
  });
  ses.on('serial-port-added', () => {});
  ses.on('serial-port-removed', () => {});

  // Fires again every time a new device is found while the scan runs.
  win.webContents.on('select-bluetooth-device', (event, devices, callback) => {
    event.preventDefault();
    if (!pendingPick || pendingPick.kind !== 'bluetooth') {
      finishPick('');
      pendingPick = { kind: 'bluetooth', callback };
    } else {
      pendingPick.callback = callback;
    }
    win.webContents.send('chooser:list', {
      kind: 'bluetooth',
      devices: devices.map((d) => ({
        id: d.deviceId,
        name: d.deviceName || 'Unnamed device',
        detail: d.deviceId,
        likely: /^keyword/i.test(d.deviceName || ''),
      })),
    });
  });

  // While the shortcut recorder is open, swallow keys before menus and the OS-level
  // accelerators (Cmd+W, Cmd+Q, Alt+F4...) see them and pass them to the page.
  win.webContents.on('before-input-event', (event, input) => {
    if (!capturing) return;
    event.preventDefault();
    win.webContents.send('capture:key', {
      type: input.type, code: input.code,
      ctrlKey: input.control, shiftKey: input.shift, altKey: input.alt, metaKey: input.meta,
    });
  });

  win.loadFile(path.join(__dirname, 'renderer', 'index.html'));
}

ipcMain.on('chooser:pick', (_e, id) => finishPick(id));
ipcMain.on('capture:set', (_e, on) => { capturing = !!on; });

ipcMain.handle('store:load', () => {
  try { return JSON.parse(fs.readFileSync(storePath(), 'utf8')); } catch { return null; }
});
ipcMain.handle('store:save', (_e, store) => {
  const tmp = storePath() + '.tmp';
  fs.writeFileSync(tmp, JSON.stringify(store, null, 2));
  fs.renameSync(tmp, storePath());
});
ipcMain.handle('profile:export', async (_e, name, text) => {
  const { canceled, filePath } = await dialog.showSaveDialog(win, {
    title: 'Export profile',
    defaultPath: `${name.replace(/[^\w -]+/g, '_') || 'profile'}.keyword.json`,
    filters: [{ name: 'Keyword profile', extensions: ['json'] }],
  });
  if (canceled || !filePath) return false;
  fs.writeFileSync(filePath, text);
  return true;
});
ipcMain.handle('profile:import', async () => {
  const { canceled, filePaths } = await dialog.showOpenDialog(win, {
    title: 'Import profile',
    filters: [{ name: 'Keyword profile', extensions: ['json'] }],
    properties: ['openFile'],
  });
  if (canceled || !filePaths.length) return null;
  return fs.readFileSync(filePaths[0], 'utf8');
});

app.whenReady().then(() => {
  createWindow();
  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
});

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit();
});
