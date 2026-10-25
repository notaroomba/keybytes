/* global KeywordProtocol, Hid, Profiles, Templates */
const { SIDES, NUS, OTA, LineBuffer, parseLine, cmd, entryCommands, profileCommands, hasHold, MAX_MACRO_STEPS, DEFAULT_HOLD_MS, otaBaseCommand, checkAppImage, otaChunks } = KeywordProtocol;

const POLL_MS = 2000;       // ask for SIDES? this often in case a push was missed
const KEYS_POLL_MS = 10000; // full KEYS? resync
const BATT_POLL_MS = 30000;
const STALE_MS = 5000;      // no line from the Keyword for this long means stale
const SIDE_NAMES = { L: 'left', R: 'right', T: 'top edge' };
const MAC = /Mac/i.test(navigator.platform);
const MOD_KEY = MAC ? 'Cmd' : 'Ctrl';
// Clicking our own window shouldn't count as "switched app" for per-app profiles.
const SELF_APPS = new Set(['keyword monitor', 'electron', 'keyword-monitor']);

const $ = (sel) => document.querySelector(sel);
const $$ = (sel) => [...document.querySelectorAll(sel)];
const el = (tag, props = {}, ...children) => {
  const e = Object.assign(document.createElement(tag), props);
  e.append(...children.filter((c) => c != null && c !== false));
  return e;
};
const logEl = $('#log');
const bridge = window.desktop || null;

function log(dir, text) {
  const t = new Date().toLocaleTimeString();
  logEl.textContent += `${t} ${dir} ${text}\n`;
  if (logEl.textContent.length > 20000) logEl.textContent = logEl.textContent.slice(-15000);
  logEl.scrollTop = logEl.scrollHeight;
}

// ---- Transports: each has open(onText, onClose), write(text), close() ----

class SerialTransport {
  label = 'USB serial';
  kind = 'serial';
  constructor(port) { this.port = port || null; } // an already-granted port skips the picker
  async open(onText, onClose) {
    if (!this.port) this.port = await navigator.serial.requestPort();
    await this.port.open({ baudRate: 115200 }); // CDC ignores the baud rate
    // The Keyword only pushes events once DTR is set.
    try { await this.port.setSignals({ dataTerminalReady: true }); } catch {}
    this.writer = this.port.writable.getWriter();
    this.closed = false;
    (async () => {
      const decoder = new TextDecoder();
      try {
        while (!this.closed && this.port.readable) {
          this.reader = this.port.readable.getReader();
          try {
            for (;;) {
              const { value, done } = await this.reader.read();
              if (done) break;
              onText(decoder.decode(value, { stream: true }));
            }
          } finally {
            this.reader.releaseLock();
          }
        }
      } catch (e) {
        log('!!', `serial: ${e.message}`);
      }
      if (!this.closed) onClose();
    })();
  }
  identity() {
    const i = this.port.getInfo?.() || {};
    return { kind: 'serial', usbVendorId: i.usbVendorId ?? null, usbProductId: i.usbProductId ?? null };
  }
  async write(text) {
    await this.writer.write(new TextEncoder().encode(text));
  }
  async close() {
    this.closed = true;
    try { await this.reader?.cancel(); } catch {}
    try { this.writer?.releaseLock(); } catch {}
    try { await this.port?.close(); } catch {}
  }
}

class BleTransport {
  label = 'Bluetooth';
  kind = 'ble';
  constructor(device) { this.device = device || null; }
  async open(onText, onClose) {
    if (!this.device) {
      this.device = await navigator.bluetooth.requestDevice({
        // NUS's 128-bit UUID isn't advertised, so match on the name.
        filters: [{ namePrefix: 'Keyword' }],
        optionalServices: [NUS.service],
      });
    }
    this.onClose = onClose;
    this.onDisconnect = () => { if (!this.closed) this.onClose(); };
    this.device.addEventListener('gattserverdisconnected', this.onDisconnect);
    const server = await this.device.gatt.connect();
    const svc = await server.getPrimaryService(NUS.service);
    this.rx = await svc.getCharacteristic(NUS.rx);
    const tx = await svc.getCharacteristic(NUS.tx);
    const decoder = new TextDecoder();
    tx.addEventListener('characteristicvaluechanged', (e) =>
      onText(decoder.decode(e.target.value, { stream: true })));
    await tx.startNotifications();
    this.closed = false;
  }
  identity() { return { kind: 'ble', id: this.device.id, name: this.device.name || '' }; }
  async write(text) {
    const bytes = new TextEncoder().encode(text);
    for (let i = 0; i < bytes.length; i += 20) { // fits the default 23-byte ATT MTU
      const chunk = bytes.slice(i, i + 20);
      if (this.rx.writeValueWithoutResponse) await this.rx.writeValueWithoutResponse(chunk);
      else await this.rx.writeValue(chunk);
    }
  }
  async close() {
    this.closed = true;
    this.device?.removeEventListener('gattserverdisconnected', this.onDisconnect);
    if (this.device?.gatt.connected) this.device.gatt.disconnect();
  }
}

// Pretends to be a Keyword (with every optional feature) so the app can be tried without hardware.
class DemoTransport {
  label = 'Demo';
  kind = 'demo';
  uids = { L: '3B0A4F1C88D2E61F5F1821C1', R: '91C7002DE4B3A75C0A33B7E2', T: '0F5E19A2C6B84D37C4D90015' };
  versions = { L: 3, R: 2, T: 3 };
  present = { L: true, R: false, T: false };
  maps = {};
  battery = 82;
  kb = 3;
  async open(onText) {
    this.onText = onText;
    this.battTimer = setInterval(() => {
      if (this.battery > 5) { this.battery -= 1; this.emit(this.battLine()); }
    }, 20000);
  }
  identity() { return { kind: 'demo' }; }
  emit(line) { setTimeout(() => this.onText?.(line + '\n'), 20); }
  battLine() { return `BATT pct=${this.battery} chg=0 usb=0`; }
  sidesLine() { return 'SIDES ' + SIDES.map((s) => `${s}=${this.present[s] ? 1 : 0}`).join(' '); }
  keyLine(s) { return `KEY+ uid=${this.uids[s]} at=${s} v=${this.versions[s]}`; }
  // A Keybit chained behind the right one, reported by its low 32 bits like a real relay.
  chained = 'D00DF00D';
  chainedLine() { return `KEY+ uid=${this.chained} at=X via=${this.uids.R} depth=1 v=3`; }
  toggle(side) {
    this.present[side] = !this.present[side];
    this.emit(this.present[side] ? this.keyLine(side) : `KEY- uid=${this.uids[side]}`);
    if (side === 'R') this.emit(this.present.R ? this.chainedLine() : `KEY- uid=${this.chained}`);
    this.emit(this.sidesLine());
  }
  sideOf(uid) { return SIDES.find((s) => this.uids[s] === uid && this.present[s]) || (uid === this.chained && this.present.R ? 'X' : undefined); }
  tap(uid) {
    if (!this.sideOf(uid)) return;
    this.emit(`PRESS uid=${uid} down=1`);
    setTimeout(() => this.emit(`PRESS uid=${uid} down=0`), 180);
  }
  flash(uid) {
    const s = this.sideOf(uid);
    if (!s) return this.emit(`ERR flash ${uid} not present`);
    if (s === 'X') return this.emit('ERR flash only Keybits touching the Keyword can be reflashed');
    let pct = 0;
    const t = setInterval(() => {
      pct += 20;
      this.emit(`FLASH uid=${uid} pct=${pct}`);
      if (pct >= 100) {
        clearInterval(t);
        this.versions[s] = this.kb;
        this.emit(`OK FLASH uid=${uid}`);
        this.emit(this.keyLine(s));
      }
    }, 300);
  }
  async write(text) {
    for (const line of text.split('\n').map((s) => s.trim()).filter(Boolean)) {
      const [c] = line.split(' ');
      const f = Object.fromEntries(line.split(' ').slice(1).map((p) => p.split('=')));
      if (c === 'HELLO') this.emit(`KEYWORD v=demo proto=1 caps=batt,hold,macro,flash,ota kb=${this.kb}`);
      else if (c === 'SIDES?') this.emit(this.sidesLine());
      else if (c === 'BATT?') this.emit(this.battLine());
      else if (c === 'KEYS?') {
        for (const s of SIDES) if (this.present[s]) this.emit(this.keyLine(s));
        if (this.present.R) this.emit(this.chainedLine());
        this.emit('KEYS END');
      } else if (['MAP', 'MAPC', 'MACRO', 'HOLD'].includes(c)) { this.maps[f.uid] = f; this.emit(`OK ${c} uid=${f.uid}`); }
      else if (c === 'UNMAP') { delete this.maps[f.uid]; this.emit(`OK UNMAP uid=${f.uid}`); }
      else if (c === 'CLEAR') { this.maps = {}; this.emit('OK CLEAR'); }
      else if (c === 'SAVE') this.emit('OK SAVE');
      else if (c === 'FLASH') this.flash(f.uid);
      else if (c === 'STACK?') this.emit('STACK ble v=1.24.0 fus=2.2.0');
      else if (c === 'OTA') this.emit('OK OTA');
      else if (c === 'DFU') this.emit('OK DFU');
      else this.emit(`ERR unknown command ${c}`);
    }
  }
  async close() { clearInterval(this.battTimer); this.onText = null; }
}

// ---- Keyword firmware update targets: connect(), start(), write(chunk), finish(), cancel(), close() ----

// ST's BLE_Ota bootloader, which the Keyword restarts into after OTA.
class BleOtaTarget {
  async connect() {
    this.device = await navigator.bluetooth.requestDevice({
      filters: [{ name: OTA.name }],
      optionalServices: [OTA.service],
    });
    const server = await this.device.gatt.connect();
    const svc = await server.getPrimaryService(OTA.service);
    this.base = await svc.getCharacteristic(OTA.base);
    this.raw = await svc.getCharacteristic(OTA.raw);
    this.confirm = await svc.getCharacteristic(OTA.confirm);
    this.confirmed = new Promise((resolve) => {
      this.confirm.addEventListener('characteristicvaluechanged', (e) => resolve(e.target.value.getUint8(0)));
    });
    await this.confirm.startNotifications(); // indications, as far as Web Bluetooth is concerned
    this.lost = new Promise((resolve) => this.device.addEventListener('gattserverdisconnected', resolve, { once: true }));
  }
  async writeBase(bytes) {
    if (this.base.writeValueWithoutResponse) await this.base.writeValueWithoutResponse(bytes);
    else await this.base.writeValue(bytes);
  }
  start() { return this.writeBase(otaBaseCommand(OTA.START, OTA.APP_OFFSET)); }
  write(chunk) { return this.raw.writeValueWithoutResponse ? this.raw.writeValueWithoutResponse(chunk) : this.raw.writeValue(chunk); }
  finish() { return this.writeBase(otaBaseCommand(OTA.FINISH)); }
  cancel() { return this.writeBase(otaBaseCommand(OTA.CANCEL)).catch(() => {}); }
  close() { if (this.device?.gatt.connected) this.device.gatt.disconnect(); }
}

// Stands in for the bootloader in Demo mode.
class DemoOtaTarget {
  async connect() { await sleep(400); this.bytes = 0; }
  async start() { this.bytes = 0; }
  async write(chunk) { this.bytes += chunk.length; }
  async finish() { await sleep(300); this.confirmed = Promise.resolve(this.bytes > 0 ? 1 : 0); this.lost = new Promise(() => {}); }
  async cancel() {}
  close() {}
}

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

// ---- Saved state (main.js writes it to disk; localStorage when run in a browser) ----

const storage = bridge
  ? { load: () => bridge.loadStore(), save: (s) => bridge.saveStore(s) }
  : {
      load: async () => { try { return JSON.parse(localStorage.getItem('keyword-profiles')); } catch { return null; } },
      save: async (s) => { try { localStorage.setItem('keyword-profiles', JSON.stringify(s)); } catch {} },
    };

let store = Profiles.normalizeStore(null);
const history = new Profiles.History();
const activeProfile = () => store.profiles.find((p) => p.id === store.active) || store.profiles[0];
const settings = () => store.settings;

let saveTimer = null;
function persist() {
  clearTimeout(saveTimer);
  saveTimer = setTimeout(() => storage.save(store).catch((e) => log('!!', `could not save: ${e.message}`)), 150);
}

// Every user edit goes through here so it can be undone.
// sync: 'full' pushes the whole profile to the Keyword afterwards; a function runs custom sync.
function mutate(label, fn, sync = 'full') {
  history.record({ active: store.active, profiles: store.profiles }, label);
  fn();
  persist();
  render();
  if (sync === 'full') applyProfile(true);
  else if (typeof sync === 'function') sync();
}

function restore(entry, verb) {
  if (!entry) return;
  store.profiles = Profiles.normalizeStore({ ...entry.state, settings: store.settings }).profiles;
  store.active = entry.state.active;
  manualActive = store.active;
  persist();
  render();
  applyProfile(true);
  toast(`${verb} ${entry.label}`);
}
const undo = () => restore(history.undo({ active: store.active, profiles: store.profiles }), 'Undid');
const redo = () => restore(history.redo({ active: store.active, profiles: store.profiles }), 'Redid');

// ---- Connection state ----

let transport = null;
let timers = [];
let lastRx = 0;
let device = { fw: null, caps: new Set(), kb: null, battery: null };
const keys = new Map(); // uid -> { at, pressed, v, flashing } for Keybits the Keyword reports
let sides = {};         // from SIDES lines
let syncSeen = null;    // uids seen during a KEYS? reply
let reconnect = null;   // { timer, tries } while trying to get a dropped link back
let flashUid = null;    // card to highlight on the next render
let frontApp = null;
let manualActive = null; // profile the user picked by hand, for returning after per-app switches
let autoSwitched = false;
const seenApps = new Set();

const caps = () => device.caps;
const can = (c) => caps().has(c);

function setStatus(text, cls) {
  $('#status-text').textContent = text;
  $('#status').className = 'status ' + (cls || '');
  pushState();
}

function activity(text, cls) {
  const list = $('#activity');
  list.prepend(el('li', { className: cls || '' }, el('time', { textContent: new Date().toLocaleTimeString() }), el('span', { textContent: text })));
  while (list.children.length > 200) list.lastChild.remove();
}

function toast(text, opts = {}) {
  const t = el('div', { className: 'toast' + (opts.error ? ' error' : '') }, el('span', { textContent: text }));
  if (opts.undo) t.append(el('button', { className: 'link', textContent: 'Undo', onclick: () => { undo(); t.remove(); } }));
  $('#toasts').append(t);
  setTimeout(() => t.classList.add('out'), opts.ms || 4000);
  setTimeout(() => t.remove(), (opts.ms || 4000) + 400);
}

function notify(title, body) {
  if (!settings().notifications || document.hasFocus()) return;
  try { new Notification(title, { body, silent: true }); } catch {}
}

const shortUid = (uid) => uid.slice(-4);
const entryOf = (uid) => activeProfile().keys[uid];
const keyName = (uid) => entryOf(uid)?.name || `Keybit ${shortUid(uid)}`;

function hostLabel(h) {
  switch (h.type) {
    case 'url': return `Open ${h.value.replace(/^https?:\/\//, '')}`;
    case 'app': return `Open ${h.value}`;
    case 'cmd': return `Run ${h.value}`;
    case 'next-profile': return 'Next profile';
    case 'profile': return `Profile: ${store.profiles.find((p) => p.id === h.value)?.name || '?'}`;
  }
  return '?';
}

// Short label for what an entry does, or '' if nothing.
function actionLabel(m) {
  if (!m) return '';
  if (m.host) return hostLabel(m.host);
  if (m.macro?.length) return Hid.macroLabel(m.macro, MAC);
  if (m.consumer) return Hid.consumerLabel(m.consumer);
  return Hid.comboLabel(m.mods || 0, m.key || 0, MAC);
}
const comboFor = (uid) => actionLabel(entryOf(uid));
const holdLabel = (h) => (h && (h.mods || h.key) ? Hid.comboLabel(h.mods, h.key, MAC) : '');

// Relayed Keybits may only be reported by their low 32 bits. Map those to a full
// UID we already know (connected, or set up in any profile) so they keep their setup.
function resolveUid(uid) {
  if (!uid || uid.length !== 8) return uid;
  const known = [...keys.keys(), ...store.profiles.flatMap((p) => Object.keys(p.keys))];
  return known.find((u) => u.length === 24 && u.endsWith(uid)) || uid;
}

function addKey(uid, at, v, relay) {
  const known = keys.get(uid);
  // Seen in full for the first time: carry over a setup saved under its short UID.
  if (!known && uid.length === 24) {
    for (const p of store.profiles) {
      const short = uid.slice(-8);
      if (p.keys[short] && !p.keys[uid]) { p.keys[uid] = p.keys[short]; delete p.keys[short]; persist(); }
    }
  }
  keys.set(uid, {
    at, pressed: known?.pressed || false, v: v ?? known?.v ?? null, flashing: known?.flashing ?? null,
    via: relay?.via ? resolveUid(relay.via) : known?.via ?? null, depth: relay?.depth ?? known?.depth ?? null,
  });
  if (!known) {
    const where = at ? ` on the ${SIDE_NAMES[at]}` : relay?.via ? ` behind ${keyName(resolveUid(relay.via))}` : '';
    activity(`${keyName(uid)} connected${where}`, 'add');
    notify('Keybit connected', `${keyName(uid)}${where}`);
    flashUid = uid;
  }
}
function removeKey(uid) {
  if (!keys.has(uid)) return;
  const { at } = keys.get(uid);
  keys.delete(uid);
  const where = at ? ` from the ${SIDE_NAMES[at]}` : '';
  activity(`${keyName(uid)} disconnected${where}`, 'remove');
  notify('Keybit disconnected', `${keyName(uid)}${where}`);
}

const outdated = (k) => k?.v != null && device.kb != null && k.v < device.kb;

// ---- Rendering ----

function render() {
  const p = activeProfile();

  // Board
  for (const s of SIDES) {
    const uid = [...keys].find(([, k]) => k.at === s)?.[0];
    const present = !!uid || !!sides[s];
    const slot = $(`.slot[data-side="${s}"]`);
    slot.classList.toggle('present', present);
    slot.classList.toggle('pressed', !!(uid && keys.get(uid).pressed));
    slot.querySelector('.slot-label').textContent = !present ? '' : uid ? (comboFor(uid) || keyName(uid)) : 'Keybit';
    slot.title = uid ? `${keyName(uid)} · uid ${uid}` : '';
    slot.dataset.uid = uid || '';
  }
  $('#board-profile').textContent = p.name;

  // Key list: connected first (left, top, right), then mapped keys that are offline.
  const order = { L: 0, T: 1, R: 2 };
  const online = [...keys].sort((a, b) => (order[a[1].at] ?? 9) - (order[b[1].at] ?? 9)).map(([u]) => u);
  const offline = Object.keys(p.keys).filter((u) => !keys.has(u)).sort();
  const q = $('#key-filter').value.trim().toLowerCase();
  const matches = (uid) => !q || [keyName(uid), comboFor(uid), uid, holdLabel(entryOf(uid)?.hold)].join(' ').toLowerCase().includes(q);
  const list = $('#keys');
  list.replaceChildren();
  for (const uid of [...online, ...offline].filter(matches)) list.append(keyCard(uid));
  flashUid = null;
  const any = online.length + offline.length > 0;
  $('#keys-empty').hidden = list.children.length > 0;
  $('#keys-empty').textContent = q && any ? `No keys match "${q}".`
    : transport ? "No Keybits connected. Attach one to the Keyword's left, right or top edge."
      : 'Connect to the Keyword, or try Demo, to see its Keybits.';

  // Profiles
  $('#profile-select').replaceChildren(...store.profiles.map((x, i) =>
    el('option', { value: x.id, textContent: x.name, title: i < 9 ? `${MOD_KEY}+${i + 1}` : '', selected: x.id === store.active })));
  $('#p-delete').disabled = store.profiles.length < 2;
  $('#p-apply').disabled = !transport;
  renderApps();

  // Undo/redo
  $('#btn-undo').disabled = !history.canUndo;
  $('#btn-undo').title = history.canUndo ? `Undo ${history.undoLabel} (${MOD_KEY}+Z)` : 'Nothing to undo';
  $('#btn-redo').disabled = !history.canRedo;
  $('#btn-redo').title = history.canRedo ? `Redo ${history.redoLabel}` : 'Nothing to redo';

  // Status pills
  const fwPill = $('#pill-fw');
  fwPill.hidden = !transport || !device.fw;
  fwPill.textContent = `Firmware ${device.fw}`;
  const b = device.battery;
  $('#pill-batt').hidden = !transport || !b;
  if (b) {
    $('#pill-batt .batt-fill').style.width = `${b.pct}%`;
    $('#pill-batt').classList.toggle('low', b.pct <= 15 && !b.charging);
    $('#pill-batt').classList.toggle('charging', b.charging);
    $('#pill-batt .batt-text').textContent = `${b.pct}%${b.charging ? ' charging' : b.usb ? ' on USB' : ''}`;
  }
  const appPill = $('#pill-app');
  appPill.hidden = !settings().perApp || !frontApp;
  if (frontApp) appPill.textContent = autoSwitched ? `${frontApp} → ${p.name}` : `App: ${frontApp}`;
  appPill.title = 'Per-app profiles are on (Settings)';

  pushState();
}

function keyCard(uid) {
  const k = keys.get(uid);
  const m = entryOf(uid);
  const combo = comboFor(uid);
  const hold = holdLabel(m?.hold);
  const where = !k ? 'Not connected'
    : k.at ? SIDE_NAMES[k.at][0].toUpperCase() + SIDE_NAMES[k.at].slice(1)
      : k.via ? `Chained behind ${keyName(k.via)}${k.depth > 1 ? ` (${k.depth} hops)` : ''}` : 'Connected';
  const presses = store.stats[uid] || 0;

  const btns = el('div', { className: 'btns' });
  btns.append(el('button', { className: 'ghost small', textContent: 'Assign…', onclick: () => openAssign(uid) }));
  if (k && transport instanceof DemoTransport) {
    btns.append(el('button', { className: 'ghost small', textContent: 'Tap', title: 'Simulate a press', onclick: () => transport.tap(uid) }));
  }
  if (k && !k.via && can('flash') && k.flashing == null) {
    btns.append(el('button', {
      className: 'ghost small' + (outdated(k) ? ' attention' : ''), textContent: outdated(k) ? 'Update' : 'Reflash',
      title: outdated(k) ? `Keybit firmware v${k.v}, Keyword has v${device.kb}` : 'Reflash this Keybit', onclick: () => flashKey(uid),
    }));
  }
  if (!k && m && [...keys.keys()].some((u) => !Profiles.isMapped(entryOf(u)))) {
    btns.append(el('button', { className: 'ghost small', textContent: 'Move to…', title: 'Give this setup to a connected Keybit, e.g. after swapping one out', onclick: () => moveKey(uid) }));
  }
  if (!k) btns.append(el('button', { className: 'ghost small', textContent: 'Forget', onclick: () => forgetKey(uid) }));

  const kind = m?.host ? 'host' : m?.macro?.length ? 'macro' : '';
  const badges = el('span', { className: 'badges' },
    k?.v != null ? el('span', { className: 'badge ver' + (outdated(k) ? ' warn' : ''), textContent: `v${k.v}`, title: outdated(k) ? 'Firmware update available' : 'Keybit firmware' }) : null,
    kind === 'host' ? el('span', { className: 'badge', textContent: 'computer', title: 'Runs on this computer while the app is open' }) : null,
    kind === 'macro' ? el('span', { className: 'badge', textContent: 'macro' }) : null);

  const meta = el('div', { className: 'meta' },
    `${where} · `,
    el('button', { className: 'uid link', textContent: `…${uid.slice(-8)}`, title: `${uid}\nClick to copy`, onclick: () => copyUid(uid) }),
    presses ? ` · ${presses.toLocaleString()} press${presses === 1 ? '' : 'es'}` : '',
    k?.flashing != null ? el('span', { className: 'flashing', textContent: ` · Flashing ${k.flashing}%` }) : null);

  const li = el('li', { className: [k ? '' : 'offline', k?.pressed ? 'pressed' : '', flashUid === uid ? 'flash' : ''].join(' ') },
    el('div', { className: 'info' }, el('div', { className: 'name' }, keyName(uid), badges), meta),
    el('div', { className: 'actions-col' },
      el('span', { className: 'combo' + (combo ? '' : ' none'), textContent: combo || 'not assigned', title: combo }),
      hold ? el('span', { className: 'hold', textContent: `hold: ${hold}` }) : null),
    btns);
  li.dataset.uid = uid;
  li.addEventListener('dblclick', (e) => { if (!e.target.closest('button')) openAssign(uid); });
  return li;
}

function renderApps() {
  const p = activeProfile();
  const box = $('#apps');
  box.replaceChildren(...p.apps.map((a) => el('span', { className: 'chip' }, a,
    el('button', { className: 'chip-x', textContent: '×', title: `Stop using in ${a}`, onclick: () => mutate(`remove ${a}`, () => { p.apps = p.apps.filter((x) => x !== a); }, null) }))));
  $('#apps-hint').textContent = !settings().perApp
    ? 'Turn on "Switch profiles by the app in front" in Settings to use this.'
    : p.apps.length ? '' : 'Not tied to any app. Other profiles switch back to this one when you leave their app.';
}

let lastState = '';
function pushState() {
  if (!bridge) return;
  const s = {
    status: $('#status-text').textContent,
    profiles: store.profiles.map((p) => ({ id: p.id, name: p.name })),
    active: store.active,
    battery: transport && device.battery ? device.battery.pct : null,
  };
  const j = JSON.stringify(s);
  if (j !== lastState) { lastState = j; bridge.updateState(s); }
}

async function copyUid(uid) {
  try { await navigator.clipboard.writeText(uid); toast('UID copied'); } catch { toast(uid); }
}

// ---- Protocol handling ----

// Lets a flow wait for a particular reply. pred returns true to take the message.
const waiters = new Set();
function waitForMessage(pred, ms) {
  return new Promise((resolve) => {
    const w = { pred, resolve, timer: setTimeout(() => { waiters.delete(w); resolve(null); }, ms) };
    waiters.add(w);
  });
}

function handleLine(line) {
  lastRx = Date.now();
  log('<-', line);
  const msg = parseLine(line);
  for (const w of [...waiters]) if (w.pred(msg)) { clearTimeout(w.timer); waiters.delete(w); w.resolve(msg); }
  switch (msg.type) {
    case 'hello': {
      const first = !device.fw;
      device = { ...device, fw: msg.fw, caps: msg.caps, kb: msg.kb };
      setStatus(`Connected over ${transport.label}`, 'connected');
      if (msg.proto !== 1) log('!!', `protocol ${msg.proto}, app expects 1`);
      if (first) {
        if (can('batt')) send(cmd.battery());
        applyProfile(true); // now that caps are known
      }
      break;
    }
    case 'sides':
      for (const [s, v] of Object.entries(msg.sides)) {
        sides[s] = v;
        // A KEY- may have been missed: drop keys on a side that is now empty.
        if (!v) for (const [uid, k] of keys) if (k.at === s) removeKey(uid);
      }
      break;
    case 'keyAdd': {
      const uid = resolveUid(msg.uid);
      syncSeen?.add(uid);
      addKey(uid, msg.at, msg.v, msg.via !== undefined ? { via: msg.via, depth: msg.depth } : null);
      break;
    }
    case 'keyRemove':
      removeKey(resolveUid(msg.uid));
      break;
    case 'keysEnd':
      if (syncSeen) for (const uid of [...keys.keys()]) if (!syncSeen.has(uid)) removeKey(uid);
      syncSeen = null;
      break;
    case 'press':
      onPress(resolveUid(msg.uid), msg.down);
      return;
    case 'battery': {
      const prev = device.battery;
      device.battery = { pct: msg.pct, charging: msg.charging, usb: msg.usb };
      if (prev && prev.pct > 15 && msg.pct <= 15 && !msg.charging) {
        toast(`Keyword battery low (${msg.pct}%)`, { error: true });
        notify('Keyword battery low', `${msg.pct}% left`);
      }
      break;
    }
    case 'flashProgress': {
      const k = keys.get(msg.uid);
      if (k) k.flashing = msg.pct;
      break;
    }
    case 'ok':
      if (msg.what === 'FLASH' && msg.uid) {
        const uid = msg.uid.toUpperCase();
        const k = keys.get(uid);
        if (k) k.flashing = null;
        activity(`${keyName(uid)} reflashed`, 'add');
        toast(`${keyName(uid)} reflashed`);
        syncKeys();
      }
      break;
    case 'error': {
      log('!!', msg.message);
      activity(`Keyword: ${msg.message}`, 'remove');
      if (/^flash/i.test(msg.message)) for (const k of keys.values()) k.flashing = null;
      break;
    }
  }
  render();
}

let statsDirty = false;
let pressRender = null;
function onPress(uid, down) {
  const k = keys.get(uid);
  if (k) k.pressed = down;
  else addKey(uid, null, null);
  if (down) {
    store.stats[uid] = (store.stats[uid] || 0) + 1;
    statsDirty = true;
    const m = entryOf(uid);
    if (m?.host) runHost(m.host, uid);
  }
  // Cheap update instead of a full render on every press.
  const li = document.querySelector(`#keys li[data-uid="${uid}"]`);
  if (li && k) li.classList.toggle('pressed', down);
  const slot = document.querySelector(`.slot[data-uid="${uid}"]`);
  if (slot) slot.classList.toggle('pressed', down);
  if (!k) render();
  else if (down) { clearTimeout(pressRender); pressRender = setTimeout(render, 400); }
  if (assign.open && assignUid === uid && down) $('#assign-name').classList.add('blink');
}
setInterval(() => { if (statsDirty) { statsDirty = false; persist(); } }, 10000);

async function runHost(h, uid) {
  if (h.type === 'next-profile') return cycleProfile(1);
  if (h.type === 'profile') {
    if (store.profiles.some((p) => p.id === h.value)) switchTo(h.value, { manual: true });
    return;
  }
  if (!bridge) return toast(`${hostLabel(h)} (only in the bridge app)`);
  const r = await bridge.runHost(h);
  if (!r.ok) toast(`${keyName(uid)}: ${r.error}`, { error: true });
}

async function send(...lines) {
  if (!transport) return;
  for (const l of lines) {
    log('->', l);
    try { await transport.write(l + '\n'); } catch (e) { log('!!', `write failed: ${e.message}`); return; }
  }
}

function syncKeys() {
  syncSeen = new Set();
  send(cmd.keys());
}

function applyProfile(quiet) {
  if (!transport || !device.fw) return;
  send(...profileCommands(activeProfile().keys, caps()));
  if (!quiet) {
    activity(`Sent "${activeProfile().name}" to the Keyword`);
    toast(`Sent "${activeProfile().name}" to the Keyword`);
  }
}

// Programs one key after an edit. UNMAP first clears any old macro/hold.
function syncKey(uid) {
  const m = entryOf(uid);
  const c = caps();
  send(cmd.unmap(uid), ...(m ? entryCommands(uid, m, c) : []), cmd.save());
}

function flashKey(uid) {
  const k = keys.get(uid);
  if (!k) return;
  k.flashing = 0;
  activity(`Reflashing ${keyName(uid)}…`);
  send(cmd.flash(uid));
  render();
}

// ---- Connecting ----

async function connect(t, { auto = false } = {}) {
  await disconnect({ quiet: true });
  setStatus(auto ? `Reconnecting over ${t.label}…` : `Connecting over ${t.label}…`, 'busy');
  const lines = new LineBuffer(handleLine);
  try {
    await t.open((text) => lines.push(text), () => onLinkLost(t));
  } catch (e) {
    closeDialog($('#chooser'));
    if (auto) throw e;
    setStatus(e.name === 'NotFoundError' ? 'Not connected' : `Could not connect: ${e.message}`);
    return false;
  }
  stopReconnect();
  transport = t;
  device = { fw: null, caps: new Set(), kb: null, battery: null };
  lastRx = Date.now();
  setStatus(`Connected over ${t.label} · waiting for the Keyword…`, 'busy');
  const demo = t instanceof DemoTransport;
  document.body.classList.toggle('demo', demo);
  $('#demo-hint').hidden = !demo;
  $('#btn-disconnect').hidden = false;
  activity(`Connected over ${t.label}`);
  if (!demo) { settings().lastConnection = t.identity(); persist(); }
  render();
  await send(cmd.hello(), cmd.sides());
  syncKeys();
  timers = [
    setInterval(() => {
      send(cmd.sides());
      if (Date.now() - lastRx > STALE_MS) setStatus(`Connected over ${t.label} · no reply from the Keyword`, 'stale');
    }, POLL_MS),
    setInterval(syncKeys, KEYS_POLL_MS),
    setInterval(() => { if (can('batt')) send(cmd.battery()); }, BATT_POLL_MS),
  ];
  return true;
}

async function disconnect({ quiet = false } = {}) {
  timers.forEach(clearInterval);
  timers = [];
  const t = transport;
  transport = null;
  if (t) {
    await t.close();
    if (!quiet) activity('Disconnected');
  }
  if (!quiet) stopReconnect();
  document.body.classList.remove('demo');
  $('#demo-hint').hidden = true;
  $('#btn-disconnect').hidden = true;
  keys.clear();
  sides = {};
  device = { fw: null, caps: new Set(), kb: null, battery: null };
  render();
  if (!reconnect) setStatus('Not connected');
}

function onLinkLost(t) {
  if (transport !== t) return;
  log('!!', 'device disconnected');
  activity('Keyword disconnected', 'remove');
  notify('Keyword disconnected', settings().autoReconnect ? 'Trying to reconnect…' : '');
  disconnect({ quiet: true });
  if (settings().autoReconnect) startReconnect(t);
  else setStatus('Not connected');
}

// Retries with backoff. Serial waits for the port to come back; BLE re-dials the same device.
function startReconnect(t) {
  stopReconnect();
  reconnect = { tries: 0, timer: null, kind: t.kind, device: t.device };
  const attempt = async () => {
    if (!reconnect || transport) return;
    reconnect.tries++;
    setStatus(`Reconnecting… (try ${reconnect.tries})`, 'busy');
    try {
      if (reconnect.kind === 'ble' && reconnect.device) await connect(new BleTransport(reconnect.device), { auto: true });
      else if (reconnect.kind === 'serial') {
        const port = await findSerialPort();
        if (!port) throw new Error('port not back yet');
        await connect(new SerialTransport(port), { auto: true });
      }
      return;
    } catch (e) {
      log('!!', `reconnect: ${e.message}`);
    }
    if (!reconnect) return;
    if (reconnect.tries >= 20) { stopReconnect(); setStatus('Not connected · gave up reconnecting'); return; }
    reconnect.timer = setTimeout(attempt, Math.min(30000, 1000 * 2 ** Math.min(reconnect.tries, 5)));
  };
  reconnect.timer = setTimeout(attempt, 1000);
}
function stopReconnect() {
  if (reconnect?.timer) clearTimeout(reconnect.timer);
  reconnect = null;
}

async function findSerialPort() {
  const last = settings().lastConnection;
  if (!('serial' in navigator) || last?.kind !== 'serial') return null;
  const ports = await navigator.serial.getPorts();
  return ports.find((p) => {
    const i = p.getInfo?.() || {};
    return last.usbVendorId == null || (i.usbVendorId === last.usbVendorId && i.usbProductId === last.usbProductId);
  }) || null;
}

// On launch, quietly reconnect to the last Keyword if its permission is still granted.
async function autoConnectOnLaunch() {
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
    log('!!', `auto-connect: ${e.message}`);
    setStatus('Not connected');
  }
}

// A Keyword plugged back in over USB reconnects straight away.
if ('serial' in navigator) {
  navigator.serial.addEventListener('connect', async () => {
    if (transport || ota.busy || !settings().autoReconnect || settings().lastConnection?.kind !== 'serial') return;
    const port = await findSerialPort();
    if (port) connect(new SerialTransport(port), { auto: true }).catch((e) => log('!!', e.message));
  });
}

// ---- Dialog helpers ----

function closeDialog(d) { if (d.open) d.close(); }

function askText(title, value, suggestions = []) {
  const d = $('#prompt');
  $('#prompt-title').textContent = title;
  const input = $('#prompt-input');
  input.value = value || '';
  $('#prompt-list').replaceChildren(...suggestions.map((s) => el('option', { value: s })));
  return new Promise((resolve) => {
    const done = (v) => { closeDialog(d); resolve(v); };
    $('#prompt-ok').onclick = () => done(input.value.trim() || null);
    $('#prompt-cancel').onclick = () => done(null);
    input.onkeydown = (e) => { if (e.key === 'Enter') { e.preventDefault(); done(input.value.trim() || null); } };
    d.oncancel = () => resolve(null);
    d.showModal();
    input.select();
  });
}

// ---- Device chooser (Electron forwards the device list from main.js) ----

const chooser = $('#chooser');
if (window.devicePicker) {
  window.devicePicker.onList(({ kind, devices }) => {
    $('#chooser-title').textContent = kind === 'serial' ? 'Choose the Keyword serial port' : 'Choose the Keyword';
    const list = $('#chooser-list');
    list.replaceChildren();
    devices.sort((a, b) => b.likely - a.likely);
    for (const d of devices) {
      const b = el('button', { className: d.likely ? 'likely' : '' },
        el('span', { textContent: d.name }), el('small', { textContent: d.detail || '' }));
      b.onclick = () => { window.devicePicker.pick(d.id); closeDialog(chooser); };
      list.append(el('li', {}, b));
    }
    $('#chooser-empty').textContent = kind === 'serial' ? 'No serial ports found. Is the Keyword plugged in?' : 'Searching for Bluetooth devices…';
    $('#chooser-empty').hidden = devices.length > 0;
    if (!chooser.open) chooser.showModal();
  });
}
$('#chooser-cancel').onclick = () => { window.devicePicker?.pick(''); closeDialog(chooser); };
chooser.addEventListener('cancel', () => window.devicePicker?.pick(''));

// ---- Assign dialog ----

const assign = $('#assign');
let assignUid = null;
let tab = 'shortcut';
let draft = null; // { mods, key, consumer, macro: [], host: {type, value}, hold: {mods, key, ms} }

$('#assign-mods').append(...Hid.MODS.map((m) =>
  el('label', {}, el('input', { type: 'checkbox', value: m.bit }), Hid.modName(m, MAC))));
// Option values are "k:<usage>" for keyboard keys and "c:<usage>" for media keys.
$('#assign-key').append(el('option', { value: 'k:0', textContent: '(no key, modifiers only)' }),
  el('optgroup', { label: 'Keys' }, ...Hid.KEYS.map((k) => el('option', { value: `k:${k.usage}`, textContent: k.label }))),
  el('optgroup', { label: 'Media' }, ...Hid.CONSUMER.map((c) => el('option', { value: `c:${c.usage}`, textContent: c.label }))));

function setTab(t) {
  tab = t;
  $$('#assign .tab').forEach((b) => b.setAttribute('aria-selected', String(b.dataset.tab === t)));
  $$('#assign .tab-panel').forEach((p) => { p.hidden = p.dataset.panel !== t; });
  $('#assign-test').hidden = t !== 'host';
  $('#hold-box').hidden = t === 'host';
  showDraft();
  const focus = { shortcut: '#capture', macro: '#macro-capture', host: '#host-value' }[t];
  setTimeout(() => $(focus)?.focus(), 0);
}
$$('#assign .tab').forEach((b) => { b.onclick = () => setTab(b.dataset.tab); });

const shortcutLabel = () => (draft.consumer ? Hid.consumerLabel(draft.consumer) : Hid.comboLabel(draft.mods, draft.key, MAC));

function showDraft() {
  // Shortcut tab
  $('#assign-mods').querySelectorAll('input').forEach((c) => { c.checked = !!(draft.mods & (1 << c.value)); c.disabled = !!draft.consumer; });
  $('#assign-key').value = draft.consumer ? `c:${draft.consumer}` : `k:${draft.key}`;
  $('#capture-combo').textContent = shortcutLabel() || 'Click here and press a key or shortcut…';

  // Macro tab
  const steps = $('#macro-steps');
  steps.replaceChildren(...draft.macro.map((st, i) => el('span', { className: 'chip' }, Hid.stepLabel(st, MAC),
    el('button', { className: 'chip-x', textContent: '×', title: 'Remove', onclick: (e) => { e.stopPropagation(); draft.macro.splice(i, 1); showDraft(); } }))));
  if (!draft.macro.length) steps.append(el('span', { className: 'hint', textContent: 'Click here and press keys to record…' }));
  const macroNote = [`${draft.macro.length}/${MAX_MACRO_STEPS} steps.`];
  if (device.fw && !can('macro')) macroNote.push("This Keyword's firmware doesn't support macros yet, so it won't be sent.");
  $('#macro-note').textContent = macroNote.join(' ');

  // Host tab
  const type = draft.host.type;
  $('#host-type').value = type;
  const needsValue = ['url', 'app', 'cmd'].includes(type);
  $('#host-value').hidden = !needsValue;
  $('#host-value').placeholder = { url: 'https://example.com or spotify:…', app: MAC ? 'App name, e.g. Spotify' : 'App or program, e.g. notepad', cmd: 'Shell command, e.g. open ~/Downloads' }[type] || '';
  $('#host-value').setAttribute('list', type === 'app' ? 'app-names' : '');
  if ($('#host-value').value !== draft.host.value && needsValue) $('#host-value').value = draft.host.value;
  $('#host-profile').hidden = type !== 'profile';
  $('#host-profile').replaceChildren(...store.profiles.map((p) => el('option', { value: p.id, textContent: p.name, selected: p.id === draft.host.value })));

  // Hold
  const hl = holdLabel(draft.hold);
  $('#hold-combo').textContent = hl || 'Click here and press what to send while held…';
  $('#hold-summary').textContent = hl ? `· ${hl}` : '';
  $('#hold-ms').value = draft.hold.ms || DEFAULT_HOLD_MS;
  $('#hold-note').textContent = device.fw && !can('hold')
    ? "This Keyword's firmware doesn't support hold actions yet, so it won't be sent."
    : 'A quick tap sends the main action; holding longer than this sends the hold action.';
}

$('#assign-mods').addEventListener('change', () => {
  draft.mods = [...$('#assign-mods').querySelectorAll('input')].reduce((m, c) => m | (c.checked ? 1 << c.value : 0), 0);
  showDraft();
});
$('#assign-key').addEventListener('change', (e) => {
  const [kind, usage] = e.target.value.split(':');
  if (kind === 'c') Object.assign(draft, { mods: 0, key: 0, consumer: Number(usage) });
  else Object.assign(draft, { key: Number(usage), consumer: 0 });
  showDraft();
});

// Key search: results are clickable chips; Enter picks the first.
function showSearch() {
  const q = $('#key-search').value;
  const res = q.trim() ? Hid.search(q).slice(0, 14) : [];
  $('#key-results').replaceChildren(...res.map((r) => el('button', {
    className: 'chip pick', textContent: r.label + (r.kind === 'c' ? ' ♪' : ''),
    onclick: () => pickSearch(r),
  })));
}
function pickSearch(r) {
  if (r.kind === 'c') Object.assign(draft, { mods: 0, key: 0, consumer: r.usage });
  else Object.assign(draft, { key: r.usage, consumer: 0 });
  $('#key-search').value = '';
  showSearch();
  showDraft();
}
$('#key-search').addEventListener('input', showSearch);
$('#key-search').addEventListener('keydown', (e) => {
  if (e.key === 'Enter') { e.preventDefault(); const r = Hid.search($('#key-search').value)[0]; if (r) pickSearch(r); }
});

// Capture boxes. Electron hands us every key (even Cmd+Q) while one is focused.
let captureTarget = null;
function onCaptured(e) {
  const r = Hid.fromEvent(e);
  if (!r || !captureTarget) return;
  if (captureTarget === 'shortcut') {
    if (r.consumer) Object.assign(draft, { mods: 0, key: 0, consumer: r.consumer });
    else Object.assign(draft, { mods: r.mods, key: r.usage, consumer: 0 });
  } else if (captureTarget === 'macro') {
    if (r.consumer || !r.usage) return; // wait for a real key; media keys can't go in a macro
    if (draft.macro.length >= MAX_MACRO_STEPS) return toast(`Macros hold up to ${MAX_MACRO_STEPS} steps`);
    draft.macro.push([r.mods, r.usage]);
  } else if (captureTarget === 'hold') {
    if (r.consumer) return;
    draft.hold = { ...draft.hold, mods: r.mods, key: r.usage };
  }
  showDraft();
}
for (const [id, target] of [['#capture', 'shortcut'], ['#macro-capture', 'macro'], ['#hold-capture', 'hold']]) {
  const box = $(id);
  box.addEventListener('keydown', (e) => { e.preventDefault(); e.stopPropagation(); onCaptured(e); });
  box.addEventListener('focus', () => { captureTarget = target; bridge?.setCapture(true); box.classList.add('listening'); });
  box.addEventListener('blur', () => { captureTarget = null; bridge?.setCapture(false); box.classList.remove('listening'); });
}
bridge?.onCaptureKey((k) => { if (k.type === 'keyDown') onCaptured(k); });

$('#macro-add-text').onclick = () => {
  const { steps, skipped } = Hid.textToSteps($('#macro-text').value);
  const room = MAX_MACRO_STEPS - draft.macro.length;
  draft.macro.push(...steps.slice(0, room));
  if (steps.length > room) toast(`Only ${room} more step${room === 1 ? '' : 's'} fit; the rest was cut`);
  if (skipped.length) toast(`Skipped characters with no US key: ${[...new Set(skipped)].join(' ')}`);
  $('#macro-text').value = '';
  showDraft();
};
$('#macro-text').addEventListener('keydown', (e) => { if (e.key === 'Enter') { e.preventDefault(); $('#macro-add-text').click(); } });
$('#macro-clear').onclick = () => { draft.macro = []; showDraft(); };

$('#host-type').onchange = (e) => {
  draft.host = { type: e.target.value, value: e.target.value === 'profile' ? store.profiles[0].id : '' };
  $('#host-value').value = '';
  showDraft();
};
$('#host-value').oninput = (e) => { draft.host.value = e.target.value; };
$('#host-profile').onchange = (e) => { draft.host.value = e.target.value; };
$('#hold-ms').onchange = (e) => { draft.hold.ms = Math.min(2000, Math.max(50, Number(e.target.value) || DEFAULT_HOLD_MS)); };
$('#hold-clear').onclick = () => { draft.hold = { mods: 0, key: 0, ms: draft.hold.ms }; showDraft(); };

function openAssign(uid) {
  assignUid = uid;
  const m = entryOf(uid) || {};
  draft = {
    mods: m.mods || 0, key: m.key || 0, consumer: m.consumer || 0,
    macro: (m.macro || []).map((s) => [...s]),
    host: m.host ? { ...m.host } : { type: 'url', value: '' },
    hold: m.hold ? { ...m.hold } : { mods: 0, key: 0, ms: DEFAULT_HOLD_MS },
  };
  $('#assign-name').textContent = keyName(uid);
  $('#assign-name').classList.remove('blink');
  $('#assign-label').value = m.name || '';
  $('#assign-clear').disabled = !Profiles.isMapped(m) && !m.name;
  $('#key-search').value = '';
  $('#host-value').value = m.host?.value || '';
  $('#app-names').replaceChildren(...[...seenApps].sort().map((a) => el('option', { value: a })));
  $('#hold-box').open = hasHold(m);
  showSearch();
  assign.showModal();
  setTab(m.host ? 'host' : m.macro?.length ? 'macro' : 'shortcut');
}
function closeAssign() {
  bridge?.setCapture(false);
  closeDialog(assign);
}
assign.addEventListener('close', () => bridge?.setCapture(false));
$('#assign-cancel').onclick = closeAssign;

// The entry the dialog describes, for the current tab.
function draftEntry() {
  const name = $('#assign-label').value.trim();
  let e;
  if (tab === 'host') {
    const v = draft.host.type === 'profile' ? $('#host-profile').value : draft.host.type === 'next-profile' ? '' : $('#host-value').value.trim();
    e = { host: { type: draft.host.type, value: v } };
  } else if (tab === 'macro') e = draft.macro.length ? { macro: draft.macro } : { mods: 0, key: 0 };
  else e = draft.consumer ? { mods: 0, key: 0, consumer: draft.consumer } : { mods: draft.mods, key: draft.key };
  if (tab !== 'host' && (draft.hold.mods || draft.hold.key)) e.hold = { ...draft.hold };
  if (name) e.name = name;
  return Profiles.cleanEntry(e);
}

$('#assign-save').onclick = () => {
  const uid = assignUid;
  const e = draftEntry();
  if (e.host && ['url', 'app', 'cmd'].includes(e.host.type) && !e.host.value) {
    toast('Fill in what to open or run first', { error: true });
    return $('#host-value').focus();
  }
  const p = activeProfile();
  mutate(`assign ${keyName(uid)}`, () => {
    if (!Profiles.isMapped(e) && !e.name && !hasHold(e)) delete p.keys[uid];
    else p.keys[uid] = e;
  }, () => syncKey(uid));
  activity(`${keyName(uid)} → ${comboFor(uid) || 'nothing'}`);
  closeAssign();
};
$('#assign-clear').onclick = () => {
  const uid = assignUid;
  mutate(`unassign ${keyName(uid)}`, () => { delete activeProfile().keys[uid]; }, () => syncKey(uid));
  closeAssign();
  toast(`Unassigned ${keyName(uid)}`, { undo: true });
};
$('#assign-test').onclick = () => {
  const e = draftEntry();
  if (e.host) runHost(e.host, assignUid);
};

// Hands an offline key's setup to a connected, unassigned Keybit.
async function moveKey(uid) {
  const free = [...keys.keys()].filter((u) => !Profiles.isMapped(entryOf(u)));
  if (!free.length) return;
  let target = free[0];
  if (free.length > 1) {
    const labels = free.map((u) => `${keys.get(u).at ? SIDE_NAMES[keys.get(u).at] : 'connected'} (…${u.slice(-4)})`);
    const pick = await askText(`Move "${keyName(uid)}" to which Keybit?`, labels[0], labels);
    if (!pick) return;
    target = free[labels.indexOf(pick)] ?? free.find((u) => pick.includes(u.slice(-4)));
    if (!target) return;
  }
  const name = keyName(uid);
  mutate(`move ${name}`, () => {
    const p = activeProfile();
    p.keys[target] = p.keys[uid];
    delete p.keys[uid];
  }, () => syncKey(target));
  toast(`Moved ${name} to the ${SIDE_NAMES[keys.get(target)?.at] || 'connected'} Keybit`, { undo: true });
}

function forgetKey(uid) {
  const name = keyName(uid);
  mutate(`forget ${name}`, () => { delete activeProfile().keys[uid]; }, null);
  toast(`Forgot ${name}`, { undo: true });
}

// ---- Profiles ----

function switchTo(id, { manual = true, record = false } = {}) {
  if (!store.profiles.some((p) => p.id === id)) return;
  if (id === store.active) return;
  const doIt = () => { store.active = id; };
  if (record) mutate('switch profile', doIt, null);
  else { doIt(); persist(); render(); }
  if (manual) { manualActive = id; autoSwitched = false; }
  applyProfile(true);
  if (manual) activity(`Switched to "${activeProfile().name}"`);
  render();
}

function cycleProfile(dir) {
  const i = store.profiles.findIndex((p) => p.id === store.active);
  const next = store.profiles[(i + dir + store.profiles.length) % store.profiles.length];
  switchTo(next.id);
  toast(`Profile: ${next.name}`);
}

$('#profile-select').onchange = (e) => switchTo(e.target.value);
$('#p-new').onclick = newProfileFlow;
async function newProfileFlow() {
  const name = await askText('New profile name', `Profile ${store.profiles.length + 1}`);
  if (!name) return;
  const p = Profiles.newProfile(name);
  mutate(`new profile ${name}`, () => { store.profiles.push(p); store.active = p.id; });
  manualActive = p.id;
}
$('#p-dup').onclick = async () => {
  const cur = activeProfile();
  const name = await askText('Name for the copy', `${cur.name} copy`);
  if (!name) return;
  const p = Profiles.newProfile(name, cur.keys);
  mutate(`duplicate ${cur.name}`, () => { store.profiles.push(p); store.active = p.id; });
  manualActive = p.id;
};
$('#p-rename').onclick = async () => {
  const cur = activeProfile();
  const name = await askText('Rename profile', cur.name);
  if (!name || name === cur.name) return;
  mutate(`rename ${cur.name}`, () => { cur.name = name; }, null);
};
$('#p-delete').onclick = () => {
  if (store.profiles.length < 2) return;
  const cur = activeProfile();
  mutate(`delete ${cur.name}`, () => {
    store.profiles = store.profiles.filter((p) => p.id !== cur.id);
    store.active = store.profiles[0].id;
  });
  manualActive = store.active;
  toast(`Deleted "${cur.name}"`, { undo: true });
};

async function saveText(name, text) {
  if (bridge) return bridge.saveFile(name, text);
  const a = el('a', { href: URL.createObjectURL(new Blob([text], { type: 'application/json' })), download: name });
  a.click();
  URL.revokeObjectURL(a.href);
  return true;
}
async function exportCurrent() {
  const p = activeProfile();
  if (await saveText(`${p.name}.keyword.json`, Profiles.exportProfile(p))) activity(`Exported "${p.name}"`);
}
async function exportAll() {
  if (await saveText('Keyword profiles.keyword.json', Profiles.exportLibrary(store.profiles))) activity(`Exported ${store.profiles.length} profiles`);
}
$('#p-export').onclick = exportCurrent;
$('#set-export-all').onclick = exportAll;

function importTexts(files) {
  const added = [];
  for (const f of files) {
    try { added.push(...Profiles.importProfiles(f.text)); } catch (e) { toast(`${f.name}: ${e.message}`, { error: true }); }
  }
  if (!added.length) return;
  // A shared profile could run anything on this computer, so ask before keeping commands.
  const risky = added.flatMap((p) => Object.values(p.keys).filter((e) => e.host && ['cmd', 'app'].includes(e.host.type)).map((e) => e.host));
  if (risky.length) {
    const list = risky.slice(0, 8).map((h) => `• ${h.type === 'cmd' ? 'Run' : 'Open'}: ${h.value}`).join('\n');
    const keep = confirm(`These profiles run things on this computer when a key is pressed:\n\n${list}${risky.length > 8 ? '\n…' : ''}\n\nKeep them? Cancel removes those actions and imports the rest.`);
    if (!keep) for (const p of added) for (const [u, e] of Object.entries(p.keys)) if (e.host && ['cmd', 'app'].includes(e.host.type)) delete p.keys[u];
  }
  // Keep names unique so the menu stays readable.
  const names = new Set(store.profiles.map((p) => p.name));
  for (const p of added) {
    let n = p.name, i = 2;
    while (names.has(n)) n = `${p.name} (${i++})`;
    p.name = n;
    names.add(n);
  }
  mutate(`import ${added.length} profile${added.length === 1 ? '' : 's'}`, () => {
    store.profiles.push(...added);
    store.active = added[0].id;
  });
  manualActive = store.active;
  toast(`Imported ${added.map((p) => `"${p.name}"`).join(', ')}`, { undo: true });
}
async function importFlow() {
  if (bridge) return importTexts(await bridge.openFiles());
  const input = el('input', { type: 'file', accept: '.json,application/json', multiple: true });
  input.onchange = async () => importTexts(await Promise.all([...input.files].map(async (f) => ({ name: f.name, text: await f.text() }))));
  input.click();
}
$('#p-import').onclick = importFlow;
$('#p-apply').onclick = () => applyProfile(false);

// Drag a profile file onto the window to import it.
document.addEventListener('dragover', (e) => { e.preventDefault(); document.body.classList.add('dropping'); });
document.addEventListener('dragleave', (e) => { if (!e.relatedTarget) document.body.classList.remove('dropping'); });
document.addEventListener('drop', async (e) => {
  e.preventDefault();
  document.body.classList.remove('dropping');
  const files = [...(e.dataTransfer?.files || [])].filter((f) => /\.json$/i.test(f.name));
  if (files.length) importTexts(await Promise.all(files.map(async (f) => ({ name: f.name, text: await f.text() }))));
});

// Apps for per-app switching
$('#p-add-app').onclick = async () => {
  const suggestions = [...seenApps].filter((a) => !SELF_APPS.has(a.toLowerCase())).sort();
  const name = await askText('Use this profile in which app?', '', suggestions);
  if (!name) return;
  const p = activeProfile();
  if (p.apps.some((a) => a.toLowerCase() === name.toLowerCase())) return;
  const other = Profiles.profileForApp(store.profiles, name);
  mutate(`add ${name}`, () => {
    if (other) other.apps = other.apps.filter((a) => a.toLowerCase() !== name.toLowerCase());
    p.apps.push(name);
  }, null);
  if (other) toast(`Moved ${name} from "${other.name}"`);
  if (!settings().perApp) toast('Turn on per-app switching in Settings for this to take effect');
};

function onFrontApp(name) {
  if (!name || SELF_APPS.has(name.toLowerCase())) return;
  seenApps.add(name);
  frontApp = name;
  if (!settings().perApp) return render();
  const target = Profiles.profileForApp(store.profiles, name);
  if (target) {
    if (target.id !== store.active) {
      switchTo(target.id, { manual: false });
      autoSwitched = true;
      activity(`${name} → "${target.name}"`);
    }
  } else if (autoSwitched && manualActive && manualActive !== store.active) {
    switchTo(manualActive, { manual: false });
    autoSwitched = false;
  }
  render();
}
bridge?.onFrontApp(onFrontApp);

// ---- Library ----

function openLibrary() {
  const known = [...new Set([...keys.keys(), ...Object.keys(activeProfile().keys)])];
  const uids = Templates.orderUids(known, (u) => keys.get(u)?.at);
  const list = $('#library-list');
  list.replaceChildren(...Templates.TEMPLATES.map((t) => {
    const preview = t.build(MAC).map((a) => a.name || actionLabel(a)).join(' · ');
    return el('li', {},
      el('div', {}, el('b', { textContent: t.name }), el('p', { className: 'hint', textContent: t.description }),
        el('p', { className: 'preview', textContent: preview })),
      el('div', { className: 'btns' },
        el('button', { className: 'small', textContent: 'New profile', onclick: () => useTemplate(t, uids, true) }),
        el('button', { className: 'ghost small', textContent: 'Apply here', title: 'Overwrite these keys in the current profile', onclick: () => useTemplate(t, uids, false) })));
  }));
  $('#library').showModal();
}
function useTemplate(t, uids, asNew) {
  if (!uids.length) return toast('Attach a Keybit (or try Demo) first, so there are keys to assign', { error: true });
  const { keys: k, apps, used, total } = Templates.applyTemplate(t, uids, MAC);
  if (asNew) {
    const p = Profiles.newProfile(t.name, k, apps);
    mutate(`add ${t.name}`, () => { store.profiles.push(p); store.active = p.id; });
    manualActive = p.id;
  } else {
    const p = activeProfile();
    mutate(`apply ${t.name}`, () => { Object.assign(p.keys, k); });
  }
  closeDialog($('#library'));
  toast(`${t.name}: ${used} of ${total} actions assigned${used < total ? ' (attach more Keybits for the rest)' : ''}`, { undo: true });
}
$('#p-library').onclick = openLibrary;
$('#library-close').onclick = () => closeDialog($('#library'));

// ---- Keyword firmware update (BLE OTA) ----

// The Keyword restarts into ST's BLE_Ota bootloader on OTA, erasing its app first (4-5 s).
// From there the image goes over Bluetooth whatever link the app was using.
const OTA_RESTART_MS = 6500;
const ota = { busy: false, image: null, fileName: '', stage: 'pick', target: null, demo: false, cancelled: false };

function otaSet(text, { error = false, pct = null } = {}) {
  const st = $('#ota-status');
  st.textContent = text;
  st.classList.toggle('error-text', error);
  $('#ota-progress').hidden = pct == null;
  if (pct != null) $('#ota-progress').value = pct;
}

function otaButtons() {
  const go = $('#ota-go');
  const realLink = transport && !(transport instanceof DemoTransport);
  const labels = {
    pick: realLink || transport ? 'Start update' : `Find ${OTA.name}`,
    find: `Find ${OTA.name}`,
    sending: 'Sending…', done: 'Done', waiting: 'Restarting…',
  };
  go.textContent = labels[ota.stage];
  go.disabled = ota.stage === 'sending' || ota.stage === 'waiting' || (ota.stage !== 'done' && !ota.image);
  $('#ota-cancel').textContent = ota.stage === 'sending' ? 'Cancel' : 'Close';
  $('#ota-file').disabled = ota.busy;
}

async function openOta() {
  if (ota.busy) { $('#ota').showModal(); return; }
  Object.assign(ota, { stage: 'pick', cancelled: false, demo: transport instanceof DemoTransport });
  $('#ota-dfu').hidden = true;
  $('#ota-stack').textContent = '';
  otaSet(transport
    ? (can('ota') ? '' : 'This Keyword firmware does not report OTA support. The update will be tried anyway.')
    : `Not connected. If the Keyword is already in update mode (it shows up as "${OTA.name}"), pick the image and find it.`);
  otaButtons();
  closeDialog($('#settings'));
  $('#ota').showModal();
  if (transport && device.fw) {
    send(cmd.stack());
    const m = await waitForMessage((x) => x.type === 'stack' || (x.type === 'error' && /stack/i.test(x.message)), 2000);
    if (m?.type === 'stack') {
      $('#ota-stack').textContent = `Keyword firmware ${device.fw} · wireless stack ${m.stack === 'ble' ? `BLE ${m.v || ''}` : m.stack}${m.fus ? ` · FUS ${m.fus}` : ''}`;
      if (m.stack === 'none') {
        otaSet('The Keyword has no Bluetooth stack on its radio core, so it can only be updated over USB DFU.', { error: true });
        $('#ota-dfu').hidden = false;
      }
    } else $('#ota-stack').textContent = `Keyword firmware ${device.fw}`;
  }
}

$('#ota-file').onchange = async (e) => {
  const f = e.target.files[0];
  ota.image = null;
  if (f) {
    const bytes = new Uint8Array(await f.arrayBuffer());
    const check = checkAppImage(bytes);
    if (check.ok) {
      ota.image = bytes;
      ota.fileName = f.name;
      $('#ota-file-info').textContent = `${f.name} · ${(bytes.length / 1024).toFixed(1)} KB`;
      if ($('#ota-dfu').hidden) otaSet('');
    } else {
      $('#ota-file-info').textContent = '';
      otaSet(check.error, { error: true });
    }
  }
  otaButtons();
};

$('#ota-go').onclick = async () => {
  if (ota.stage === 'done') return closeDialog($('#ota'));
  if (ota.stage === 'pick' && transport) return otaRestartKeyword();
  return otaSend();
};

$('#ota-cancel').onclick = async () => {
  if (ota.stage === 'sending') {
    ota.cancelled = true;
    return;
  }
  closeDialog($('#ota'));
};
$('#ota').addEventListener('cancel', (e) => { if (ota.busy) e.preventDefault(); });

$('#ota-dfu').onclick = async () => {
  if (!transport) return;
  if (!confirm('Restart the Keyword into its USB DFU bootloader? It stops working as a keyboard until it is flashed with STM32CubeProgrammer (or dfu-util) and restarted.')) return;
  send(cmd.dfu());
  const m = await waitForMessage((x) => (x.type === 'ok' && x.what === 'DFU') || x.type === 'error', 2000);
  if (m?.type === 'ok') {
    await disconnect();
    otaSet('The Keyword is in USB DFU mode. Flash keyword_full.bin at 0x08000000 with STM32CubeProgrammer.');
  } else otaSet(m ? `Keyword: ${m.message}` : 'No reply from the Keyword.', { error: true });
};

// Step 1: ask the running Keyword to restart into the bootloader.
async function otaRestartKeyword() {
  ota.busy = true;
  ota.stage = 'waiting';
  otaButtons();
  otaSet('Asking the Keyword to restart into update mode…');
  send(cmd.ota());
  const m = await waitForMessage((x) => (x.type === 'ok' && x.what === 'OTA') || x.type === 'error', 3000);
  if (m?.type !== 'ok') {
    ota.busy = false;
    ota.stage = 'pick';
    otaButtons();
    if (m && /no BLE stack/i.test(m.message)) {
      otaSet('The Keyword has no Bluetooth stack on its radio core, so it can only be updated over USB DFU.', { error: true });
      $('#ota-dfu').hidden = false;
    } else otaSet(m ? `Keyword: ${m.message}` : 'The Keyword did not answer. Its firmware may be too old for OTA.', { error: true });
    return;
  }
  ota.demo = transport instanceof DemoTransport;
  await disconnect();
  activity('Keyword restarting into update mode');
  const until = Date.now() + (ota.demo ? 1500 : OTA_RESTART_MS);
  while (Date.now() < until) {
    otaSet(`The Keyword is erasing its app and restarting into update mode… ${Math.ceil((until - Date.now()) / 1000)}`);
    await sleep(250);
  }
  ota.stage = 'find';
  otaButtons();
  // Web Bluetooth only opens its picker from a click, so the user presses Find.
  otaSet(`Ready. Click "Find ${OTA.name}" and pick it from the list.`);
}

// Step 2: send the image to the bootloader.
async function otaSend() {
  if (!ota.image) return;
  if (!ota.demo && !('bluetooth' in navigator)) return otaSet('Web Bluetooth is not available here.', { error: true });
  ota.busy = true;
  ota.cancelled = false;
  stopReconnect();
  const target = ota.demo ? new DemoOtaTarget() : new BleOtaTarget();
  ota.target = target;
  try {
    otaSet(`Connecting to ${OTA.name}…`);
    ota.stage = 'waiting';
    otaButtons();
    await target.connect();
    ota.stage = 'sending';
    otaButtons();
    await target.start();
    // There's no flow control, so pace the writes to the bootloader's flash speed (~3 ms per 248 bytes).
    // Chunks fit common MTUs; if a size is refused, step down to a smaller one.
    const sizes = MAC ? [176, 16] : [240, 176, 16];
    let chunks = otaChunks(ota.image, sizes[0]);
    let size = 0;
    let sent = 0;
    const t0 = Date.now();
    for (let i = 0; i < chunks.length; i++) {
      if (ota.cancelled) throw new Error('cancelled');
      try {
        await target.write(chunks[i]);
      } catch (e) {
        if (i > 0 || size + 1 >= sizes.length) throw e;
        size++;
        log('!!', `OTA write of ${chunks[i].length} B refused (${e.message}), trying ${sizes[size]}`);
        chunks = otaChunks(ota.image, sizes[size]);
        i = -1;
        continue;
      }
      sent += chunks[i].length;
      await sleep(Math.max(2, Math.ceil(chunks[i].length / 248 * 3) + 1));
      if (i % 8 === 0 || i === chunks.length - 1) {
        const rate = sent / Math.max(1, Date.now() - t0); // bytes per ms
        const left = Math.ceil((ota.image.length - sent) / rate / 1000);
        otaSet(`Sending ${ota.fileName}… ${Math.round(sent / 1024)} of ${Math.round(ota.image.length / 1024)} KB${left > 1 ? ` · about ${left} s left` : ''}`,
          { pct: Math.min(100, Math.round(sent / ota.image.length * 100)) });
      }
    }
    otaSet('Checking the image…', { pct: 100 });
    await target.finish();
    const ack = await Promise.race([target.confirmed, target.lost.then(() => 'lost'), sleep(15000).then(() => 'timeout')]);
    if (ack === 1) {
      ota.stage = 'done';
      otaSet('Updated. The Keyword is restarting with the new firmware.');
      activity('Keyword firmware updated', 'add');
      toast('Keyword firmware updated');
      target.close();
      setTimeout(() => { if (!ota.demo) autoConnectOnLaunch(); else connect(new DemoTransport()); }, 3000);
    } else {
      throw new Error(ack === 'lost' ? 'the Keyword disconnected before confirming'
        : ack === 'timeout' ? 'no confirmation from the Keyword' : `the Keyword rejected the image (${ack})`);
    }
  } catch (e) {
    await target.cancel?.();
    target.close();
    ota.stage = 'find';
    if (e.name === 'NotFoundError') otaSet(`Didn't find ${OTA.name}. Click Find to try again.`);
    else if (e.message === 'cancelled') otaSet(`Cancelled. The Keyword stays in update mode (as "${OTA.name}") until it gets a good image.`);
    else otaSet(`Update failed: ${e.message}. The Keyword stays in update mode, so you can try again.`, { error: true });
    log('!!', `OTA: ${e.message}`);
  } finally {
    ota.busy = false;
    ota.target = null;
    otaButtons();
  }
}

$('#set-ota').onclick = openOta;

// ---- Settings ----

function applySettings() {
  const s = settings();
  if (s.theme === 'system') delete document.documentElement.dataset.theme;
  else document.documentElement.dataset.theme = s.theme;
  bridge?.applySettings({ theme: s.theme, closeToTray: s.closeToTray, launchAtLogin: s.launchAtLogin, perApp: s.perApp });
  if (!s.perApp) { autoSwitched = false; }
}
function openSettings() {
  const s = settings();
  $('#set-theme').value = s.theme;
  for (const k of ['autoReconnect', 'perApp', 'notifications', 'closeToTray', 'launchAtLogin']) $(`#set-${k}`).checked = !!s[k];
  $('#settings').showModal();
}
$('#set-theme').onchange = (e) => { settings().theme = e.target.value; persist(); applySettings(); };
for (const k of ['autoReconnect', 'perApp', 'notifications', 'closeToTray', 'launchAtLogin']) {
  $(`#set-${k}`).onchange = async (e) => {
    settings()[k] = e.target.checked;
    if (k === 'notifications' && e.target.checked && 'Notification' in window && Notification.permission === 'default') {
      try { await Notification.requestPermission(); } catch {}
    }
    persist();
    applySettings();
    render();
  };
}
$('#set-reset-stats').onclick = () => { store.stats = {}; persist(); render(); toast('Press counts reset'); };
$('#set-data').onclick = () => bridge?.openDataFolder();
$('#set-data').hidden = !bridge;
$('#settings-close').onclick = () => closeDialog($('#settings'));
$('#btn-settings').onclick = openSettings;

function toggleTheme() {
  const dark = document.documentElement.dataset.theme === 'dark'
    || (!document.documentElement.dataset.theme && matchMedia('(prefers-color-scheme: dark)').matches);
  settings().theme = dark ? 'light' : 'dark';
  persist();
  applySettings();
}
$('#btn-theme').onclick = toggleTheme;

// ---- Buttons, menu and shortcuts ----

const connectUsb = () => {
  if (!('serial' in navigator)) return setStatus('Web Serial is not available here');
  connect(new SerialTransport());
};
const connectBle = () => {
  if (!('bluetooth' in navigator)) return setStatus('Web Bluetooth is not available here');
  connect(new BleTransport());
};
$('#btn-usb').onclick = connectUsb;
$('#btn-ble').onclick = connectBle;
$('#btn-demo').onclick = () => connect(new DemoTransport());
$('#btn-disconnect').onclick = () => disconnect();
$('#btn-undo').onclick = undo;
$('#btn-redo').onclick = redo;
$('#activity-clear').onclick = () => $('#activity').replaceChildren();
$('#key-filter').addEventListener('input', render);

$$('.board .slot').forEach((slot) => {
  slot.addEventListener('click', () => {
    if (transport instanceof DemoTransport) transport.toggle(slot.dataset.side);
    else if (slot.dataset.uid) openAssign(slot.dataset.uid);
  });
});

const inTextField = () => {
  const a = document.activeElement;
  return a && (a.tagName === 'TEXTAREA' || (a.tagName === 'INPUT' && !['checkbox', 'radio'].includes(a.type)));
};
const anyDialogOpen = () => $$('dialog').some((d) => d.open);

function onMenu(command, arg) {
  if (captureTarget) return; // the recorder owns the keyboard
  switch (command) {
    case 'undo': if (inTextField()) document.execCommand('undo'); else if (!anyDialogOpen()) undo(); break;
    case 'redo': if (inTextField()) document.execCommand('redo'); else if (!anyDialogOpen()) redo(); break;
    case 'settings': openSettings(); break;
    case 'import': importFlow(); break;
    case 'export': exportCurrent(); break;
    case 'export-all': exportAll(); break;
    case 'connect-usb': connectUsb(); break;
    case 'connect-ble': connectBle(); break;
    case 'disconnect': disconnect(); break;
    case 'profile': switchTo(arg); break;
    case 'next-profile': cycleProfile(1); break;
    case 'prev-profile': cycleProfile(-1); break;
    case 'new-profile': newProfileFlow(); break;
    case 'library': openLibrary(); break;
    case 'apply': applyProfile(false); break;
    case 'toggle-theme': toggleTheme(); break;
    case 'update-firmware': openOta(); break;
    case 'find': $('#key-filter').focus(); $('#key-filter').select(); break;
  }
}
bridge?.onMenu(onMenu);

// In a plain browser (no Electron menu) handle the same shortcuts here.
if (!bridge) {
  document.addEventListener('keydown', (e) => {
    const mod = MAC ? e.metaKey : e.ctrlKey;
    if (!mod || captureTarget) return;
    const k = e.key.toLowerCase();
    if (k === 'z' && !e.shiftKey) { if (!inTextField()) { e.preventDefault(); onMenu('undo'); } }
    else if ((k === 'z' && e.shiftKey) || k === 'y') { if (!inTextField()) { e.preventDefault(); onMenu('redo'); } }
    else if (k === 'f') { e.preventDefault(); onMenu('find'); }
    else if (/^[1-9]$/.test(k) && store.profiles[k - 1]) { e.preventDefault(); onMenu('profile', store.profiles[k - 1].id); }
  });
}

// ---- Start ----

storage.load().then(async (s) => {
  store = Profiles.normalizeStore(s);
  manualActive = store.active;
  applySettings();
  render();
  if (bridge) {
    const info = await bridge.appInfo();
    $('#set-about').textContent = `Keyword Monitor ${info.version} · ${info.platform}`;
  }
  autoConnectOnLaunch();
});
render();
