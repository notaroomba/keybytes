// The app's behaviour: talking to the Keyword, profiles, undo, per-app switching and
// firmware updates. Components read the zustand store and call these functions.
import type { MenuCommand, PickerList, TrayState } from '../../shared/bridge';
import * as Hid from './lib/hid.ts';
import * as Profiles from './lib/profiles.ts';
import {
  checkAppImage, cmd, entryCommands, LineBuffer, OTA, otaChunks, parseLine, profileCommands, type Message,
} from './lib/protocol.ts';
import * as Templates from './lib/templates.ts';
import {
  BleOtaTarget, BleTransport, DemoOtaTarget, DemoTransport, OtaModeError, SerialTransport, sleep,
  type OtaTarget, type Transport,
} from './lib/transports.ts';
import type { Entry, HostAction, Profile, Settings, Side } from './lib/types.ts';
import {
  activeProfileOf, emptyDevice, initialOta, useApp, type ActivityTone, type AppState, type KeyState, type OtaState,
} from './store.ts';

const POLL_MS = 2000; // ask SIDES? this often in case a push was missed
const KEYS_POLL_MS = 10000; // full KEYS? resync
const BATT_POLL_MS = 30000;
const STALE_MS = 5000; // no line from the Keyword for this long means stale
export const SIDE_NAMES: Record<Side, string> = { L: 'left', R: 'right', T: 'top edge' };
export const MAC = /Mac/i.test(navigator.platform);
export const MOD_KEY = MAC ? 'Cmd' : 'Ctrl';
// Clicking our own window shouldn't count as "switched app" for per-app profiles.
const SELF_APPS = new Set(['keyword monitor', 'electron', 'keyword-monitor']);

export const bridge = window.desktop ?? null;
const get = useApp.getState;
const set = useApp.setState;

let nextId = 1;
const now = () => new Date().toLocaleTimeString();

// ---- Log, activity, toasts, notifications ----

export function log(dir: '->' | '<-' | '!!', text: string) {
  set((s) => ({ log: [...s.log.slice(-1500), { id: nextId++, time: now(), dir, text }] }));
}

export function activity(text: string, tone?: ActivityTone) {
  set((s) => ({ activity: [{ id: nextId++, time: now(), text, tone }, ...s.activity].slice(0, 200) }));
}

export function toast(text: string, opts: { error?: boolean; undo?: boolean; ms?: number } = {}) {
  const id = nextId++;
  set((s) => ({ toasts: [...s.toasts, { id, text, error: opts.error, undo: opts.undo }] }));
  const ms = opts.ms ?? 4000;
  setTimeout(() => set((s) => ({ toasts: s.toasts.map((t) => (t.id === id ? { ...t, leaving: true } : t)) })), ms);
  setTimeout(() => dismissToast(id), ms + 300);
}
export const dismissToast = (id: number) => set((s) => ({ toasts: s.toasts.filter((t) => t.id !== id) }));

function notify(title: string, body: string) {
  if (!settings().notifications || document.hasFocus()) return;
  try { new Notification(title, { body, silent: true }); } catch { /* notifications unavailable */ }
}

function setStatus(status: string, tone: AppState['tone'] = 'idle') {
  set({ status, tone });
}

// ---- Saved state (main.js writes it to disk; localStorage when run in a browser) ----

const storage = bridge
  ? { load: () => bridge.loadStore(), save: (s: unknown) => bridge.saveStore(s) }
  : {
      load: async () => { try { return JSON.parse(localStorage.getItem('keyword-profiles') || 'null'); } catch { return null; } },
      save: async (s: unknown) => { try { localStorage.setItem('keyword-profiles', JSON.stringify(s)); } catch { /* storage full or blocked */ } },
    };

export const settings = (): Settings => get().store.settings;
export const activeProfile = (): Profile => activeProfileOf(get());

let saveTimer: ReturnType<typeof setTimeout> | undefined;
function persist() {
  clearTimeout(saveTimer);
  saveTimer = setTimeout(() => storage.save(get().store).catch((e: Error) => log('!!', `could not save: ${e.message}`)), 150);
}

export function updateSettings(patch: Partial<Settings>) {
  set((s) => ({ store: { ...s.store, settings: { ...s.store.settings, ...patch } } }));
  persist();
  applySettings();
}

// ---- Undo history ----

type Snapshot = { active: string; profiles: Profile[] };
const history = new Profiles.History<Snapshot>();
let manualActive: string | null = null; // profile picked by hand, for returning after per-app switches

const snapshot = (): Snapshot => ({ active: get().store.active, profiles: get().store.profiles });
function refreshHistory() {
  set({ history: { canUndo: history.canUndo, canRedo: history.canRedo, undoLabel: history.undoLabel, redoLabel: history.redoLabel } });
}

/**
 * Every user edit goes through here so it can be undone. fn changes a copy of the profiles.
 * sync: 'full' pushes the whole profile to the Keyword afterwards, a function runs custom sync.
 */
function mutate(label: string, fn: (d: Snapshot) => void, sync: 'full' | (() => void) | null = 'full') {
  history.record(snapshot(), label);
  const draft = structuredClone(snapshot());
  fn(draft);
  set((s) => ({ store: { ...s.store, active: draft.active, profiles: draft.profiles } }));
  refreshHistory();
  persist();
  if (sync === 'full') applyProfile(true);
  else if (typeof sync === 'function') sync();
}

function restore(entry: { state: Snapshot; label: string } | null, verb: string) {
  if (!entry) return;
  const fixed = Profiles.normalizeStore({ ...entry.state, settings: settings() });
  set((s) => ({ store: { ...s.store, profiles: fixed.profiles, active: fixed.active } }));
  manualActive = fixed.active;
  refreshHistory();
  persist();
  applyProfile(true);
  toast(`${verb} ${entry.label}`);
}
export const undo = () => restore(history.undo(snapshot()), 'Undid');
export const redo = () => restore(history.redo(snapshot()), 'Redid');

// ---- Names and labels ----

export const shortUid = (uid: string) => uid.slice(-4);
export const entryOf = (uid: string, p: Profile = activeProfile()): Entry | undefined => p.keys[uid];
export const keyName = (uid: string, p: Profile = activeProfile()) => entryOf(uid, p)?.name || `Keybit ${shortUid(uid)}`;

export function hostLabel(h: HostAction): string {
  switch (h.type) {
    case 'url': return `Open ${h.value.replace(/^https?:\/\//, '')}`;
    case 'app': return `Open ${h.value}`;
    case 'cmd': return `Run ${h.value}`;
    case 'next-profile': return 'Next profile';
    case 'profile': return `Profile: ${get().store.profiles.find((p) => p.id === h.value)?.name || '?'}`;
  }
}

/** Short label for what an entry does, or '' if nothing. */
export function actionLabel(m: Entry | undefined): string {
  if (!m) return '';
  if (m.host) return hostLabel(m.host);
  if (m.macro?.length) return Hid.macroLabel(m.macro, MAC);
  if (m.consumer) return Hid.consumerLabel(m.consumer);
  return Hid.comboLabel(m.mods || 0, m.key || 0, MAC);
}
export const holdLabel = (h: Entry['hold']) => (h && (h.mods || h.key) ? Hid.comboLabel(h.mods, h.key, MAC) : '');

export const can = (c: string) => get().device.caps.includes(c);
export const isOutdated = (k: KeyState | undefined, kb: number | null) => k?.v != null && kb != null && k.v < kb;

// ---- Keys ----

/**
 * Relayed Keybits may only be reported by their low 32 bits. Map those to a full UID we
 * already know (connected, or set up in any profile) so they keep their setup.
 */
function resolveUid(uid: string): string {
  if (!uid || uid.length !== 8) return uid;
  const s = get();
  const known = [...Object.keys(s.keys), ...s.store.profiles.flatMap((p) => Object.keys(p.keys))];
  return known.find((u) => u.length === 24 && u.endsWith(uid)) || uid;
}

let highlightTimer: ReturnType<typeof setTimeout> | undefined;

function addKey(uid: string, at: Side | null, v: number | null, relay: { via: string | null; depth: number | null } | null) {
  const s = get();
  const known = s.keys[uid];
  // Seen in full for the first time: carry over a setup saved under its short UID.
  if (!known && uid.length === 24) {
    const short = uid.slice(-8);
    if (s.store.profiles.some((p) => p.keys[short] && !p.keys[uid])) {
      const profiles = s.store.profiles.map((p) => {
        if (!p.keys[short] || p.keys[uid]) return p;
        const keys = { ...p.keys, [uid]: p.keys[short] };
        delete keys[short];
        return { ...p, keys };
      });
      set({ store: { ...s.store, profiles } });
      persist();
    }
  }
  const next: KeyState = {
    at,
    pressed: known?.pressed || false,
    v: v ?? known?.v ?? null,
    flashing: known?.flashing ?? null,
    via: relay?.via ? resolveUid(relay.via) : known?.via ?? null,
    depth: relay?.depth ?? known?.depth ?? null,
  };
  set((st) => ({ keys: { ...st.keys, [uid]: next } }));
  if (!known) {
    const where = at ? ` on the ${SIDE_NAMES[at]}` : relay?.via ? ` behind ${keyName(resolveUid(relay.via))}` : '';
    activity(`${keyName(uid)} connected${where}`, 'add');
    notify('Keybit connected', `${keyName(uid)}${where}`);
    clearTimeout(highlightTimer);
    set({ highlight: uid });
    highlightTimer = setTimeout(() => set({ highlight: null }), 1600);
  }
}

function removeKey(uid: string) {
  const k = get().keys[uid];
  if (!k) return;
  set((s) => {
    const keys = { ...s.keys };
    delete keys[uid];
    return { keys };
  });
  const where = k.at ? ` from the ${SIDE_NAMES[k.at]}` : '';
  activity(`${keyName(uid)} disconnected${where}`, 'remove');
  notify('Keybit disconnected', `${keyName(uid)}${where}`);
}

function patchKey(uid: string, patch: Partial<KeyState>) {
  set((s) => (s.keys[uid] ? { keys: { ...s.keys, [uid]: { ...s.keys[uid], ...patch } } } : {}));
}

export async function copyUid(uid: string) {
  try {
    await navigator.clipboard.writeText(uid);
    toast('UID copied');
  } catch {
    toast(uid);
  }
}

// ---- Protocol handling ----

let transport: Transport | null = null;
let timers: ReturnType<typeof setInterval>[] = [];
let lastRx = 0;
let syncSeen: Set<string> | null = null; // uids seen during a KEYS? reply
let reconnect: { tries: number; timer?: ReturnType<typeof setTimeout>; kind: Transport['kind']; device: BluetoothDevice | null } | null = null;

export const demo = () => (transport instanceof DemoTransport ? transport : null);

// Lets a flow wait for a particular reply. pred returns true to take the message.
type Waiter = { pred: (m: Message) => boolean; resolve: (m: Message | null) => void; timer: ReturnType<typeof setTimeout> };
const waiters = new Set<Waiter>();
function waitForMessage(pred: (m: Message) => boolean, ms: number): Promise<Message | null> {
  return new Promise((resolve) => {
    const w: Waiter = { pred, resolve, timer: setTimeout(() => { waiters.delete(w); resolve(null); }, ms) };
    waiters.add(w);
  });
}

function handleLine(line: string) {
  lastRx = Date.now();
  log('<-', line);
  const msg = parseLine(line);
  for (const w of [...waiters]) {
    if (w.pred(msg)) {
      clearTimeout(w.timer);
      waiters.delete(w);
      w.resolve(msg);
    }
  }
  switch (msg.type) {
    case 'hello': {
      const first = !get().device.fw;
      set((s) => ({ device: { ...s.device, fw: msg.fw, caps: [...msg.caps], kb: msg.kb } }));
      setStatus(`Connected over ${transport?.label}`, 'connected');
      if (msg.proto !== 1) log('!!', `protocol ${msg.proto}, app expects 1`);
      if (first) {
        if (can('batt')) send(cmd.battery());
        applyProfile(true); // now that caps are known
      }
      break;
    }
    case 'sides': {
      set((s) => ({ sides: { ...s.sides, ...msg.sides } }));
      // A KEY- may have been missed: drop keys on a side that is now empty.
      for (const [side, present] of Object.entries(msg.sides)) {
        if (!present) for (const [uid, k] of Object.entries(get().keys)) if (k.at === side) removeKey(uid);
      }
      break;
    }
    case 'keyAdd': {
      const uid = resolveUid(msg.uid);
      syncSeen?.add(uid);
      addKey(uid, msg.at, msg.v, msg.via !== undefined ? { via: msg.via ?? null, depth: msg.depth ?? null } : null);
      break;
    }
    case 'keyRemove':
      removeKey(resolveUid(msg.uid));
      break;
    case 'keysEnd':
      if (syncSeen) for (const uid of Object.keys(get().keys)) if (!syncSeen.has(uid)) removeKey(uid);
      syncSeen = null;
      break;
    case 'press':
      onPress(resolveUid(msg.uid), msg.down);
      break;
    case 'battery': {
      const prev = get().device.battery;
      set((s) => ({ device: { ...s.device, battery: { pct: msg.pct, charging: msg.charging, usb: msg.usb } } }));
      if (prev && prev.pct > 15 && msg.pct <= 15 && !msg.charging) {
        toast(`Keyword battery low (${msg.pct}%)`, { error: true });
        notify('Keyword battery low', `${msg.pct}% left`);
      }
      break;
    }
    case 'flashProgress':
      patchKey(msg.uid, { flashing: msg.pct });
      break;
    case 'ok':
      if (msg.what === 'FLASH' && msg.uid) {
        const uid = msg.uid.toUpperCase();
        patchKey(uid, { flashing: null });
        activity(`${keyName(uid)} reflashed`, 'add');
        toast(`${keyName(uid)} reflashed`);
        syncKeys();
      }
      break;
    case 'error':
      log('!!', msg.message);
      activity(`Keyword: ${msg.message}`, 'remove');
      if (/^flash/i.test(msg.message)) {
        set((s) => ({ keys: Object.fromEntries(Object.entries(s.keys).map(([u, k]) => [u, { ...k, flashing: null }])) }));
      }
      break;
  }
}

let statsDirty = false;
function onPress(uid: string, down: boolean) {
  const k = get().keys[uid];
  if (k) patchKey(uid, { pressed: down });
  else addKey(uid, null, null, null);
  if (!down) return;
  set((s) => ({ store: { ...s.store, stats: { ...s.store.stats, [uid]: (s.store.stats[uid] || 0) + 1 } } }));
  statsDirty = true;
  const m = entryOf(uid);
  if (m?.host) void runHost(m.host, uid);
  pressListeners.forEach((fn) => fn(uid));
}
setInterval(() => {
  if (statsDirty) {
    statsDirty = false;
    persist();
  }
}, 10000);

const pressListeners = new Set<(uid: string) => void>();
/** Lets a component react to presses (the Assign dialog flashes the key's name). */
export function onKeyPress(fn: (uid: string) => void) {
  pressListeners.add(fn);
  return () => { pressListeners.delete(fn); };
}

export async function runHost(h: HostAction, uid: string) {
  if (h.type === 'next-profile') return cycleProfile(1);
  if (h.type === 'profile') {
    if (get().store.profiles.some((p) => p.id === h.value)) switchTo(h.value, { manual: true });
    return;
  }
  if (!bridge) return toast(`${hostLabel(h)} (only in the desktop app)`);
  const r = await bridge.runHost(h);
  if (!r.ok) toast(`${keyName(uid)}: ${r.error}`, { error: true });
}

export async function send(...lines: string[]) {
  const t = transport;
  if (!t) return;
  for (const l of lines) {
    log('->', l);
    try {
      await t.write(l + '\n');
    } catch (e) {
      log('!!', `write failed: ${(e as Error).message}`);
      return;
    }
  }
}

function syncKeys() {
  syncSeen = new Set();
  void send(cmd.keys());
}

export function applyProfile(quiet: boolean) {
  if (!transport || !get().device.fw) return;
  const p = activeProfile();
  void send(...profileCommands(p.keys, new Set(get().device.caps)));
  if (!quiet) {
    activity(`Sent "${p.name}" to the Keyword`);
    toast(`Sent "${p.name}" to the Keyword`);
  }
}

/** Programs one key after an edit. UNMAP first clears any old macro or hold. */
function syncKey(uid: string) {
  const m = entryOf(uid);
  void send(cmd.unmap(uid), ...(m ? entryCommands(uid, m, new Set(get().device.caps)) : []), cmd.save());
}

export function flashKey(uid: string) {
  if (!get().keys[uid]) return;
  patchKey(uid, { flashing: 0 });
  activity(`Reflashing ${keyName(uid)}…`);
  void send(cmd.flash(uid));
}

// ---- Connecting ----

async function connect(t: Transport, { auto = false } = {}): Promise<boolean> {
  await disconnect({ quiet: true });
  setStatus(auto ? `Reconnecting over ${t.label}…` : `Connecting over ${t.label}…`, 'busy');
  const lines = new LineBuffer(handleLine);
  try {
    await t.open((text) => lines.push(text), () => onLinkLost(t));
  } catch (e) {
    set({ chooser: null });
    if (e instanceof OtaModeError) {
      setStatus('Not connected');
      void openOta({ device: e.device });
      return false;
    }
    if (auto) throw e;
    const err = e as Error;
    setStatus(err.name === 'NotFoundError' ? 'Not connected' : `Could not connect: ${err.message}`);
    return false;
  }
  stopReconnect();
  transport = t;
  lastRx = Date.now();
  set({ device: emptyDevice(), transport: t.kind });
  setStatus(`Connected over ${t.label} · waiting for the Keyword…`, 'busy');
  activity(`Connected over ${t.label}`);
  const id = t.identity();
  if (id) updateSettings({ lastConnection: id });
  await send(cmd.hello(), cmd.sides());
  syncKeys();
  timers = [
    setInterval(() => {
      void send(cmd.sides());
      if (Date.now() - lastRx > STALE_MS) setStatus(`Connected over ${t.label} · no reply from the Keyword`, 'stale');
    }, POLL_MS),
    setInterval(syncKeys, KEYS_POLL_MS),
    setInterval(() => { if (can('batt')) void send(cmd.battery()); }, BATT_POLL_MS),
  ];
  return true;
}

export async function disconnect({ quiet = false } = {}) {
  timers.forEach(clearInterval);
  timers = [];
  const t = transport;
  transport = null;
  if (t) {
    await t.close();
    if (!quiet) activity('Disconnected');
  }
  if (!quiet) stopReconnect();
  set({ keys: {}, sides: {}, device: emptyDevice(), transport: null });
  if (!reconnect) setStatus('Not connected');
}

function onLinkLost(t: Transport) {
  if (transport !== t) return;
  log('!!', 'device disconnected');
  activity('Keyword disconnected', 'remove');
  notify('Keyword disconnected', settings().autoReconnect ? 'Trying to reconnect…' : '');
  void disconnect({ quiet: true });
  if (settings().autoReconnect) startReconnect(t);
  else setStatus('Not connected');
}

/** Retries with backoff. Serial waits for the port to come back; BLE re-dials the same device. */
function startReconnect(t: Transport) {
  stopReconnect();
  const r: NonNullable<typeof reconnect> = { tries: 0, kind: t.kind, device: t instanceof BleTransport ? t.device : null };
  reconnect = r;
  set({ reconnecting: true });
  const attempt = async () => {
    if (reconnect !== r || transport) return;
    r.tries++;
    setStatus(`Reconnecting… (try ${r.tries})`, 'busy');
    try {
      if (r.kind === 'ble' && r.device) await connect(new BleTransport(r.device), { auto: true });
      else if (r.kind === 'serial') {
        const port = await findSerialPort();
        if (!port) throw new Error('port not back yet');
        await connect(new SerialTransport(port), { auto: true });
      }
      return;
    } catch (e) {
      log('!!', `reconnect: ${(e as Error).message}`);
    }
    if (reconnect !== r) return;
    if (r.tries >= 20) {
      stopReconnect();
      setStatus('Not connected · gave up reconnecting');
      return;
    }
    r.timer = setTimeout(attempt, Math.min(30000, 1000 * 2 ** Math.min(r.tries, 5)));
  };
  r.timer = setTimeout(attempt, 1000);
}

function stopReconnect() {
  if (reconnect?.timer) clearTimeout(reconnect.timer);
  reconnect = null;
  set({ reconnecting: false });
}

async function findSerialPort(): Promise<SerialPort | null> {
  const last = settings().lastConnection;
  if (!('serial' in navigator) || last?.kind !== 'serial') return null;
  const ports = await navigator.serial.getPorts();
  return ports.find((p) => {
    const i = p.getInfo?.() ?? {};
    return last.usbVendorId == null || (i.usbVendorId === last.usbVendorId && i.usbProductId === last.usbProductId);
  }) ?? null;
}

/** On launch, quietly reconnect to the last Keyword if its permission is still granted. */
async function autoConnect() {
  const last = settings().lastConnection;
  if (!settings().autoReconnect || !last || transport) return;
  try {
    if (last.kind === 'serial') {
      const port = await findSerialPort();
      if (port) await connect(new SerialTransport(port), { auto: true });
    } else if (last.kind === 'ble' && navigator.bluetooth?.getDevices) {
      const dev = (await navigator.bluetooth.getDevices()).find((d) => d.id === last.id);
      if (dev) await connect(new BleTransport(dev), { auto: true });
    }
  } catch (e) {
    log('!!', `auto-connect: ${(e as Error).message}`);
    setStatus('Not connected');
  }
}

export function connectUsb() {
  if (!('serial' in navigator)) return setStatus('Web Serial is not available here');
  void connect(new SerialTransport());
}
export function connectBle() {
  if (!('bluetooth' in navigator)) return setStatus('Web Bluetooth is not available here');
  void connect(new BleTransport());
}
export const connectDemo = () => void connect(new DemoTransport());

// ---- Device chooser (main forwards the device list) ----

export function pickDevice(id: string) {
  window.devicePicker?.pick(id);
  set({ chooser: null });
}

// ---- Dialog helpers ----

export function askText(title: string, value = '', suggestions: string[] = []): Promise<string | null> {
  return new Promise((resolve) => set({ prompt: { title, value, suggestions, resolve } }));
}
export function askConfirm(title: string, body: string, ok = 'OK', cancel = 'Cancel'): Promise<boolean> {
  return new Promise((resolve) => set({ confirm: { title, body, ok, cancel, resolve } }));
}

export const openAssign = (uid: string) => set({ assignUid: uid });
export const closeAssign = () => {
  bridge?.setCapture(false);
  set({ assignUid: null });
};
export const openDialog = (dialog: AppState['dialog']) => set({ dialog });
export const closeDialog = () => {
  if (get().dialog === 'ota' && get().ota.busy) return;
  set({ dialog: null });
};

// ---- Editing keys ----

/** Saves what the Assign dialog built for a key. */
export function saveEntry(uid: string, e: Entry) {
  mutate(`assign ${keyName(uid)}`, (d) => {
    const p = d.profiles.find((x) => x.id === d.active)!;
    if (!Profiles.isMapped(e) && !e.name && !(e.hold && (e.hold.mods || e.hold.key))) delete p.keys[uid];
    else p.keys[uid] = e;
  }, () => syncKey(uid));
  activity(`${keyName(uid)} → ${actionLabel(entryOf(uid)) || 'nothing'}`);
}

export function unassign(uid: string) {
  const name = keyName(uid);
  mutate(`unassign ${name}`, (d) => { delete d.profiles.find((x) => x.id === d.active)!.keys[uid]; }, () => syncKey(uid));
  toast(`Unassigned ${name}`, { undo: true });
}

/** Hands an offline key's setup to a connected, unassigned Keybit. */
export async function moveKey(uid: string) {
  const keys = get().keys;
  const free = Object.keys(keys).filter((u) => !Profiles.isMapped(entryOf(u)));
  if (!free.length) return;
  let target: string | undefined = free[0];
  if (free.length > 1) {
    const labels = free.map((u) => `${keys[u].at ? SIDE_NAMES[keys[u].at!] : 'connected'} (…${u.slice(-4)})`);
    const pick = await askText(`Move "${keyName(uid)}" to which Keybit?`, labels[0], labels);
    if (!pick) return;
    target = free[labels.indexOf(pick)] ?? free.find((u) => pick.includes(u.slice(-4)));
    if (!target) return;
  }
  const to = target;
  const name = keyName(uid);
  mutate(`move ${name}`, (d) => {
    const p = d.profiles.find((x) => x.id === d.active)!;
    p.keys[to] = p.keys[uid];
    delete p.keys[uid];
  }, () => syncKey(to));
  const at = get().keys[to]?.at;
  toast(`Moved ${name} to the ${at ? SIDE_NAMES[at] : 'connected'} Keybit`, { undo: true });
}

export function forgetKey(uid: string) {
  const name = keyName(uid);
  mutate(`forget ${name}`, (d) => { delete d.profiles.find((x) => x.id === d.active)!.keys[uid]; }, null);
  toast(`Forgot ${name}`, { undo: true });
}

// ---- Profiles ----

export function switchTo(id: string, { manual = true } = {}) {
  const s = get();
  if (!s.store.profiles.some((p) => p.id === id) || id === s.store.active) return;
  set({ store: { ...s.store, active: id } });
  persist();
  if (manual) {
    manualActive = id;
    set({ autoSwitched: false });
  }
  applyProfile(true);
  if (manual) activity(`Switched to "${activeProfile().name}"`);
}

export function cycleProfile(dir: number) {
  const { profiles, active } = get().store;
  const i = profiles.findIndex((p) => p.id === active);
  const next = profiles[(i + dir + profiles.length) % profiles.length];
  switchTo(next.id);
  toast(`Profile: ${next.name}`);
}

function addProfile(p: Profile, label: string) {
  mutate(label, (d) => {
    d.profiles.push(p);
    d.active = p.id;
  });
  manualActive = p.id;
}

export async function newProfileFlow() {
  const name = await askText('New profile name', `Profile ${get().store.profiles.length + 1}`);
  if (name) addProfile(Profiles.newProfile(name), `new profile ${name}`);
}

export async function duplicateProfile() {
  const cur = activeProfile();
  const name = await askText('Name for the copy', `${cur.name} copy`);
  if (name) addProfile(Profiles.newProfile(name, cur.keys, cur.apps), `duplicate ${cur.name}`);
}

export async function renameProfile() {
  const cur = activeProfile();
  const name = await askText('Rename profile', cur.name);
  if (!name || name === cur.name) return;
  mutate(`rename ${cur.name}`, (d) => { d.profiles.find((p) => p.id === cur.id)!.name = name; }, null);
}

export function deleteProfile() {
  const { profiles } = get().store;
  if (profiles.length < 2) return;
  const cur = activeProfile();
  mutate(`delete ${cur.name}`, (d) => {
    d.profiles = d.profiles.filter((p) => p.id !== cur.id);
    d.active = d.profiles[0].id;
  });
  manualActive = get().store.active;
  toast(`Deleted "${cur.name}"`, { undo: true });
}

async function saveText(name: string, text: string) {
  if (bridge) return bridge.saveFile(name, text);
  const a = Object.assign(document.createElement('a'), {
    href: URL.createObjectURL(new Blob([text], { type: 'application/json' })),
    download: name,
  });
  a.click();
  URL.revokeObjectURL(a.href);
  return true;
}

export async function exportCurrent() {
  const p = activeProfile();
  if (await saveText(`${p.name}.keyword.json`, Profiles.exportProfile(p))) activity(`Exported "${p.name}"`);
}

export async function exportAll() {
  const { profiles } = get().store;
  if (await saveText('Keyword profiles.keyword.json', Profiles.exportLibrary(profiles))) activity(`Exported ${profiles.length} profiles`);
}

export async function importTexts(files: { name: string; text: string }[]) {
  const added: Profile[] = [];
  for (const f of files) {
    try {
      added.push(...Profiles.importProfiles(f.text));
    } catch (e) {
      toast(`${f.name}: ${(e as Error).message}`, { error: true });
    }
  }
  if (!added.length) return;
  // A shared profile could run anything on this computer, so ask before keeping commands.
  const risky = added.flatMap((p) => Object.values(p.keys).filter((e) => e.host && ['cmd', 'app'].includes(e.host.type)).map((e) => e.host!));
  if (risky.length) {
    const list = risky.slice(0, 8).map((h) => `• ${h.type === 'cmd' ? 'Run' : 'Open'}: ${h.value}`).join('\n');
    const keep = await askConfirm(
      'Keep actions that run things on this computer?',
      `These profiles run things on this computer when a key is pressed:\n\n${list}${risky.length > 8 ? '\n…' : ''}\n\nRemove them to import the rest without them.`,
      'Keep them',
      'Remove them',
    );
    if (!keep) {
      for (const p of added) {
        for (const [u, e] of Object.entries(p.keys)) if (e.host && ['cmd', 'app'].includes(e.host.type)) delete p.keys[u];
      }
    }
  }
  // Keep names unique so the menu stays readable.
  const names = new Set(get().store.profiles.map((p) => p.name));
  for (const p of added) {
    let n = p.name;
    let i = 2;
    while (names.has(n)) n = `${p.name} (${i++})`;
    p.name = n;
    names.add(n);
  }
  mutate(`import ${added.length} profile${added.length === 1 ? '' : 's'}`, (d) => {
    d.profiles.push(...added);
    d.active = added[0].id;
  });
  manualActive = get().store.active;
  toast(`Imported ${added.map((p) => `"${p.name}"`).join(', ')}`, { undo: true });
}

export async function importFlow() {
  if (bridge) return importTexts(await bridge.openFiles());
  const input = Object.assign(document.createElement('input'), { type: 'file', accept: '.json,application/json', multiple: true });
  input.onchange = async () => importTexts(await Promise.all([...(input.files ?? [])].map(async (f) => ({ name: f.name, text: await f.text() }))));
  input.click();
}

export async function addAppFlow() {
  const suggestions = get().seenApps.filter((a) => !SELF_APPS.has(a.toLowerCase())).sort();
  const name = await askText('Use this profile in which app?', '', suggestions);
  if (!name) return;
  const p = activeProfile();
  if (p.apps.some((a) => a.toLowerCase() === name.toLowerCase())) return;
  const other = Profiles.profileForApp(get().store.profiles, name);
  mutate(`add ${name}`, (d) => {
    for (const x of d.profiles) x.apps = x.apps.filter((a) => a.toLowerCase() !== name.toLowerCase());
    d.profiles.find((x) => x.id === p.id)!.apps.push(name);
  }, null);
  if (other) toast(`Moved ${name} from "${other.name}"`);
  if (!settings().perApp) toast('Turn on per-app switching in Settings for this to take effect');
}

export function removeApp(app: string) {
  const p = activeProfile();
  mutate(`remove ${app}`, (d) => {
    const x = d.profiles.find((y) => y.id === p.id)!;
    x.apps = x.apps.filter((a) => a !== app);
  }, null);
}

function onFrontApp(name: string) {
  if (!name || SELF_APPS.has(name.toLowerCase())) return;
  set((s) => ({ frontApp: name, seenApps: s.seenApps.includes(name) ? s.seenApps : [...s.seenApps, name] }));
  if (!settings().perApp) return;
  const target = Profiles.profileForApp(get().store.profiles, name);
  if (target) {
    if (target.id !== get().store.active) {
      switchTo(target.id, { manual: false });
      set({ autoSwitched: true });
      activity(`${name} → "${target.name}"`);
    }
  } else if (get().autoSwitched && manualActive && manualActive !== get().store.active) {
    switchTo(manualActive, { manual: false });
    set({ autoSwitched: false });
  }
}

// ---- Library ----

export function libraryUids() {
  const s = get();
  const known = [...new Set([...Object.keys(s.keys), ...Object.keys(activeProfile().keys)])];
  return Templates.orderUids(known, (u) => s.keys[u]?.at);
}

export function applyLibraryTemplate(t: Templates.Template, asNew: boolean) {
  const uids = libraryUids();
  if (!uids.length) return toast('Attach a Keybit (or try Demo) first, so there are keys to assign', { error: true });
  const { keys, apps, used, total } = Templates.applyTemplate(t, uids, MAC);
  if (asNew) addProfile(Profiles.newProfile(t.name, keys, apps), `add ${t.name}`);
  else mutate(`apply ${t.name}`, (d) => { Object.assign(d.profiles.find((p) => p.id === d.active)!.keys, keys); });
  set({ dialog: null });
  toast(`${t.name}: ${used} of ${total} actions assigned${used < total ? ' (attach more Keybits for the rest)' : ''}`, { undo: true });
}

// ---- Keyword firmware update (BLE OTA) ----

// The Keyword restarts into ST's BLE_Ota bootloader on OTA, erasing its app first (4-5 s).
// From there the image goes over Bluetooth whatever link the app was using.
const OTA_RESTART_MS = 6500;
let otaImage: Uint8Array | null = null;
let otaDevice: BluetoothDevice | null = null;
let otaDemo = false;
let otaCancelled = false;

const setOta = (patch: Partial<OtaState>) => set((s) => ({ ota: { ...s.ota, ...patch } }));
const otaSay = (status: string, { error = false, pct = null }: { error?: boolean; pct?: number | null } = {}) =>
  setOta({ status, error, pct });

export async function openOta({ device = null }: { device?: BluetoothDevice | null } = {}) {
  if (get().ota.busy) return set({ dialog: 'ota' });
  otaDevice = device;
  otaDemo = transport instanceof DemoTransport;
  otaCancelled = false;
  set({
    dialog: 'ota',
    ota: {
      ...initialOta(),
      stage: device ? 'find' : 'pick',
      hasDevice: !!device,
      fileName: otaImage ? get().ota.fileName : '',
      fileSize: otaImage?.length ?? 0,
    },
  });
  otaSay(device
    ? 'This Keyword is in update mode. Pick the firmware image and click Start update.'
    : transport
      ? (can('ota') ? '' : 'This Keyword firmware does not report OTA support. The update will be tried anyway.')
      : `Not connected. If the Keyword is already in update mode (it shows up as "${OTA.name}"), pick the image and find it.`);
  const fw = get().device.fw;
  if (transport && fw) {
    void send(cmd.stack());
    const m = await waitForMessage((x) => x.type === 'stack' || (x.type === 'error' && /stack/i.test(x.message)), 2000);
    if (m?.type === 'stack') {
      const stack = m.stack === 'ble' ? `BLE ${m.v || ''}` : m.stack;
      setOta({ stack: `Keyword firmware ${fw} · wireless stack ${stack}${m.fus ? ` · FUS ${m.fus}` : ''}` });
      if (m.stack === 'none') {
        otaSay('The Keyword has no Bluetooth stack on its radio core, so it can only be updated over USB DFU.', { error: true });
        setOta({ showDfu: true });
      }
    } else setOta({ stack: `Keyword firmware ${fw}` });
  }
}

export async function pickOtaFile(file: File | undefined) {
  otaImage = null;
  setOta({ fileName: '', fileSize: 0 });
  if (!file) return;
  const bytes = new Uint8Array(await file.arrayBuffer());
  const check = checkAppImage(bytes);
  if (check.ok) {
    otaImage = bytes;
    setOta({ fileName: file.name, fileSize: bytes.length });
    if (!get().ota.showDfu) otaSay('');
  } else otaSay(check.error, { error: true });
}

export function otaGo() {
  const { stage } = get().ota;
  if (stage === 'done') return set({ dialog: null });
  if (stage === 'pick' && transport) return otaRestartKeyword();
  return otaSend();
}

export function otaCancel() {
  if (get().ota.stage === 'sending') otaCancelled = true;
  else closeDialog();
}

export async function otaDfu() {
  if (!transport) return;
  const ok = await askConfirm(
    'Restart into USB DFU?',
    'The Keyword stops working as a keyboard until it is flashed with STM32CubeProgrammer (or dfu-util) and restarted.',
    'Restart into DFU',
  );
  if (!ok) return;
  void send(cmd.dfu());
  const m = await waitForMessage((x) => (x.type === 'ok' && x.what === 'DFU') || x.type === 'error', 2000);
  if (m?.type === 'ok') {
    await disconnect();
    // Over USB DFU, starting the app from the programmer (-g, :leave, -startfus, -fwupgrade)
    // rewrites nSWBOOT0 and can lock the Keyword out of DFU.
    otaSay("The Keyword is in USB DFU mode. Flash keyword_full.bin at 0x08000000 with STM32CubeProgrammer, then unplug and replug it. Don't use \"run after programming\" (-g / :leave) or the FUS/stack upgrade, which can lock it out of DFU.");
  } else otaSay(m?.type === 'error' ? `Keyword: ${m.message}` : 'No reply from the Keyword.', { error: true });
}

/** Step 1: ask the running Keyword to restart into the bootloader. */
async function otaRestartKeyword() {
  setOta({ busy: true, stage: 'waiting' });
  otaSay('Asking the Keyword to restart into update mode…');
  void send(cmd.ota());
  const m = await waitForMessage((x) => (x.type === 'ok' && x.what === 'OTA') || x.type === 'error', 3000);
  if (m?.type !== 'ok') {
    setOta({ busy: false, stage: 'pick' });
    if (m?.type === 'error' && /no BLE stack/i.test(m.message)) {
      otaSay('The Keyword has no Bluetooth stack on its radio core, so it can only be updated over USB DFU.', { error: true });
      setOta({ showDfu: true });
    } else otaSay(m?.type === 'error' ? `Keyword: ${m.message}` : 'The Keyword did not answer. Its firmware may be too old for OTA.', { error: true });
    return;
  }
  otaDemo = transport instanceof DemoTransport;
  await disconnect();
  activity('Keyword restarting into update mode');
  const until = Date.now() + (otaDemo ? 1500 : OTA_RESTART_MS);
  while (Date.now() < until) {
    otaSay(`The Keyword is erasing its app and restarting into update mode… ${Math.ceil((until - Date.now()) / 1000)}`);
    await sleep(250);
  }
  // Web Bluetooth only opens its picker from a click, so the user presses Find.
  setOta({ stage: 'find', busy: false });
  otaSay(`Ready. Click "Find ${OTA.name}" and pick it from the list.`);
}

/** Step 2: send the image to the bootloader. */
async function otaSend() {
  const image = otaImage;
  if (!image) return;
  if (!otaDemo && !('bluetooth' in navigator)) return otaSay('Web Bluetooth is not available here.', { error: true });
  otaCancelled = false;
  stopReconnect();
  const target: OtaTarget = otaDemo ? new DemoOtaTarget() : new BleOtaTarget(otaDevice);
  setOta({ busy: true, stage: 'waiting' });
  try {
    otaSay(`Connecting to ${OTA.name}…`);
    await target.connect();
    setOta({ stage: 'sending' });
    await target.start();
    // There's no flow control, so pace the writes to the bootloader's flash speed (~3 ms per 248 bytes).
    // Chunks fit common MTUs; if a size is refused, step down to a smaller one.
    const sizes = MAC ? [176, 16] : [240, 176, 16];
    let chunks = otaChunks(image, sizes[0]);
    let size = 0;
    let sent = 0;
    const t0 = Date.now();
    for (let i = 0; i < chunks.length; i++) {
      if (otaCancelled) throw new Error('cancelled');
      try {
        await target.write(chunks[i]);
      } catch (e) {
        if (i > 0 || size + 1 >= sizes.length) throw e;
        size++;
        log('!!', `OTA write of ${chunks[i].length} B refused (${(e as Error).message}), trying ${sizes[size]}`);
        chunks = otaChunks(image, sizes[size]);
        i = -1;
        continue;
      }
      sent += chunks[i].length;
      // macOS drops write-without-response packets that come too fast (a run of
      // 131 x 176 B went missing at 4 ms), so pace it much slower there.
      await sleep(MAC && !otaDemo ? 20 : Math.max(2, Math.ceil((chunks[i].length / 248) * 3) + 1));
      if (i % 8 === 0 || i === chunks.length - 1) {
        const rate = sent / Math.max(1, Date.now() - t0); // bytes per ms
        const left = Math.ceil((image.length - sent) / rate / 1000);
        otaSay(
          `Sending ${get().ota.fileName}… ${Math.round(sent / 1024)} of ${Math.round(image.length / 1024)} KB${left > 1 ? ` · about ${left} s left` : ''}`,
          { pct: Math.min(100, Math.round((sent / image.length) * 100)) },
        );
      }
    }
    otaSay('Checking the image…', { pct: 100 });
    await target.finish();
    const ack = await Promise.race([target.confirmed, target.lost.then(() => 'lost' as const), sleep(15000).then(() => 'timeout' as const)]);
    if (ack !== 1) {
      throw new Error(ack === 'lost' ? 'the Keyword disconnected before confirming'
        : ack === 'timeout' ? 'no confirmation from the Keyword' : `the Keyword rejected the image (${ack})`);
    }
    setOta({ stage: 'done' });
    otaSay('Updated. The Keyword is restarting with the new firmware.');
    activity('Keyword firmware updated', 'add');
    toast('Keyword firmware updated');
    target.close();
    otaDevice = null;
    setTimeout(() => { if (otaDemo) connectDemo(); else void autoConnect(); }, 3000);
  } catch (e) {
    const err = e as Error;
    await target.cancel();
    target.close();
    setOta({ stage: 'find' });
    if (err.name === 'NotFoundError') otaSay(`Didn't find ${OTA.name}. Click Find to try again.`);
    else if (err.message === 'cancelled') otaSay(`Cancelled. The Keyword stays in update mode (as "${OTA.name}") until it gets a good image.`);
    else otaSay(`Update failed: ${err.message}. The Keyword stays in update mode, so you can try again.`, { error: true });
    log('!!', `OTA: ${err.message}`);
  } finally {
    setOta({ busy: false });
  }
}

export const hasOtaImage = () => !!otaImage;

// ---- Settings and theme ----

const darkQuery = matchMedia('(prefers-color-scheme: dark)');
function applyTheme() {
  const t = settings().theme;
  const dark = t === 'dark' || (t === 'system' && darkQuery.matches);
  document.documentElement.dataset.theme = dark ? 'dark' : 'light';
  document.documentElement.style.colorScheme = dark ? 'dark' : 'light';
}
darkQuery.addEventListener('change', applyTheme);

function applySettings() {
  const s = settings();
  applyTheme();
  bridge?.applySettings({ theme: s.theme, closeToTray: s.closeToTray, launchAtLogin: s.launchAtLogin, perApp: s.perApp });
  if (!s.perApp) set({ autoSwitched: false });
}

export function toggleTheme() {
  updateSettings({ theme: document.documentElement.dataset.theme === 'dark' ? 'light' : 'dark' });
}

export async function setNotifications(on: boolean) {
  if (on && 'Notification' in window && Notification.permission === 'default') {
    try { await Notification.requestPermission(); } catch { /* ignored */ }
  }
  updateSettings({ notifications: on });
}

export function resetStats() {
  set((s) => ({ store: { ...s.store, stats: {} } }));
  persist();
  toast('Press counts reset');
}

// ---- Menu, tray and shortcuts ----

let capturing = false;
export function setCapture(on: boolean) {
  capturing = on;
  bridge?.setCapture(on);
}

const inTextField = () => {
  const a = document.activeElement as HTMLElement | null;
  return !!a && (a.tagName === 'TEXTAREA' || (a.tagName === 'INPUT' && !['checkbox', 'radio'].includes((a as HTMLInputElement).type)));
};
const anyDialogOpen = () => {
  const s = get();
  return !!(s.dialog || s.assignUid || s.prompt || s.confirm || s.chooser);
};

export function onMenu(command: MenuCommand, arg?: string) {
  if (capturing) return; // the recorder owns the keyboard
  switch (command) {
    case 'undo': if (inTextField()) document.execCommand('undo'); else if (!anyDialogOpen()) undo(); break;
    case 'redo': if (inTextField()) document.execCommand('redo'); else if (!anyDialogOpen()) redo(); break;
    case 'settings': set({ dialog: 'settings' }); break;
    case 'import': void importFlow(); break;
    case 'export': void exportCurrent(); break;
    case 'export-all': void exportAll(); break;
    case 'connect-usb': connectUsb(); break;
    case 'connect-ble': connectBle(); break;
    case 'disconnect': void disconnect(); break;
    case 'profile': if (arg) switchTo(arg); break;
    case 'next-profile': cycleProfile(1); break;
    case 'prev-profile': cycleProfile(-1); break;
    case 'new-profile': void newProfileFlow(); break;
    case 'library': set({ dialog: 'library' }); break;
    case 'apply': applyProfile(false); break;
    case 'toggle-theme': toggleTheme(); break;
    case 'find': document.querySelector<HTMLInputElement>('#key-filter')?.select(); break;
    case 'update-firmware': void openOta(); break;
    case 'console': set((s) => ({ consoleOpen: !s.consoleOpen })); break;
  }
}

let lastTray = '';
function pushTrayState(s: AppState) {
  if (!bridge) return;
  const state: TrayState = {
    status: s.status,
    profiles: s.store.profiles.map((p) => ({ id: p.id, name: p.name })),
    active: s.store.active,
    battery: s.transport && s.device.battery ? s.device.battery.pct : null,
  };
  const j = JSON.stringify(state);
  if (j !== lastTray) {
    lastTray = j;
    bridge.updateState(state);
  }
}

// ---- Start ----

let started = false;
export async function start() {
  if (started) return;
  started = true;
  useApp.subscribe(pushTrayState);
  bridge?.onMenu(onMenu);
  bridge?.onFrontApp(onFrontApp);
  window.devicePicker?.onList((list: PickerList) => {
    list.devices.sort((a, b) => Number(b.likely) - Number(a.likely));
    set({ chooser: list });
  });

  // In a plain browser (no Electron menu) handle the same shortcuts here.
  if (!bridge) {
    document.addEventListener('keydown', (e) => {
      const mod = MAC ? e.metaKey : e.ctrlKey;
      if (!mod || capturing) return;
      const k = e.key.toLowerCase();
      const profiles = get().store.profiles;
      if (k === 'z' && !e.shiftKey) { if (!inTextField()) { e.preventDefault(); onMenu('undo'); } }
      else if ((k === 'z' && e.shiftKey) || k === 'y') { if (!inTextField()) { e.preventDefault(); onMenu('redo'); } }
      else if (k === 'f') { e.preventDefault(); onMenu('find'); }
      else if (/^[1-9]$/.test(k) && profiles[Number(k) - 1]) { e.preventDefault(); onMenu('profile', profiles[Number(k) - 1].id); }
    });
  }

  // A Keyword plugged back in over USB reconnects straight away.
  if ('serial' in navigator) {
    navigator.serial.addEventListener('connect', async () => {
      if (transport || get().ota.busy || !settings().autoReconnect || settings().lastConnection?.kind !== 'serial') return;
      const port = await findSerialPort();
      if (port) connect(new SerialTransport(port), { auto: true }).catch((e: Error) => log('!!', e.message));
    });
  }

  const loaded = Profiles.normalizeStore(await storage.load());
  set({ store: loaded });
  manualActive = loaded.active;
  applySettings();
  if (bridge) {
    const info = await bridge.appInfo();
    set({ about: `Keyword Monitor ${info.version} · ${info.platform}` });
  }
  void autoConnect();
}
