/* global KeywordProtocol, Hid, Profiles */
const { SIDES, NUS, LineBuffer, parseLine, cmd, profileCommands } = KeywordProtocol;

const POLL_MS = 2000;      // ask for SIDES? this often in case a push was missed
const KEYS_POLL_MS = 10000; // full KEYS? resync
const STALE_MS = 5000;     // no line from the Keyword for this long means stale
const SIDE_NAMES = { L: 'left', R: 'right', T: 'top edge' };
const MAC = /Mac/i.test(navigator.platform);

const $ = (sel) => document.querySelector(sel);
const el = (tag, props = {}, ...children) => {
  const e = Object.assign(document.createElement(tag), props);
  e.append(...children.filter((c) => c != null));
  return e;
};
const logEl = $('#log');

function log(dir, text) {
  const t = new Date().toLocaleTimeString();
  logEl.textContent += `${t} ${dir} ${text}\n`;
  if (logEl.textContent.length > 20000) logEl.textContent = logEl.textContent.slice(-15000);
  logEl.scrollTop = logEl.scrollHeight;
}

// ---- Transports: each has open(onText, onClose), write(text), close() ----

class SerialTransport {
  label = 'USB serial';
  async open(onText, onClose) {
    this.port = await navigator.serial.requestPort();
    await this.port.open({ baudRate: 115200 }); // CDC ignores the baud rate
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
  async open(onText, onClose) {
    this.device = await navigator.bluetooth.requestDevice({
      filters: [{ namePrefix: 'Keyword' }, { services: [NUS.service] }],
      optionalServices: [NUS.service],
    });
    this.device.addEventListener('gattserverdisconnected', () => { if (!this.closed) onClose(); });
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
    if (this.device?.gatt.connected) this.device.gatt.disconnect();
  }
}

// Pretends to be a Keyword so the app can be tried without hardware.
class DemoTransport {
  label = 'Demo';
  uids = { L: '3B0A4F1C88D2E61F5F1821C1', R: '91C7002DE4B3A75C0A33B7E2', T: '0F5E19A2C6B84D37C4D90015' };
  present = { L: true, R: false, T: false };
  maps = {};
  async open(onText) { this.onText = onText; }
  emit(line) { setTimeout(() => this.onText(line + '\n'), 20); }
  sidesLine() { return 'SIDES ' + SIDES.map((s) => `${s}=${this.present[s] ? 1 : 0}`).join(' '); }
  toggle(side) {
    this.present[side] = !this.present[side];
    this.emit(this.present[side] ? `KEY+ uid=${this.uids[side]} at=${side}` : `KEY- uid=${this.uids[side]}`);
    this.emit(this.sidesLine());
  }
  tap(uid) {
    if (!Object.entries(this.uids).some(([s, u]) => u === uid && this.present[s])) return;
    this.emit(`PRESS uid=${uid} down=1`);
    setTimeout(() => this.emit(`PRESS uid=${uid} down=0`), 180);
  }
  async write(text) {
    for (const line of text.split('\n').map((s) => s.trim()).filter(Boolean)) {
      const [c] = line.split(' ');
      const f = Object.fromEntries(line.split(' ').slice(1).map((p) => p.split('=')));
      if (c === 'HELLO') this.emit('KEYWORD v=demo proto=1');
      else if (c === 'SIDES?') this.emit(this.sidesLine());
      else if (c === 'KEYS?') {
        for (const s of SIDES) if (this.present[s]) this.emit(`KEY+ uid=${this.uids[s]} at=${s}`);
        this.emit('KEYS END');
      } else if (c === 'MAP') { this.maps[f.uid] = f; this.emit(`OK MAP uid=${f.uid}`); }
      else if (c === 'UNMAP') { delete this.maps[f.uid]; this.emit(`OK UNMAP uid=${f.uid}`); }
      else if (c === 'CLEAR') { this.maps = {}; this.emit('OK CLEAR'); }
      else if (c === 'SAVE') this.emit('OK SAVE');
      else this.emit(`ERR unknown command ${c}`);
    }
  }
  async close() {}
}

// ---- Profiles (persisted by main.js, or localStorage when run in a browser) ----

const storage = window.desktop
  ? { load: () => window.desktop.loadStore(), save: (s) => window.desktop.saveStore(s) }
  : {
      load: async () => { try { return JSON.parse(localStorage.getItem('keyword-profiles')); } catch { return null; } },
      save: async (s) => { try { localStorage.setItem('keyword-profiles', JSON.stringify(s)); } catch {} },
    };

let store = Profiles.normalizeStore(null);
const activeProfile = () => store.profiles.find((p) => p.id === store.active);
function persist() {
  storage.save(store).catch((e) => log('!!', `could not save profiles: ${e.message}`));
}

// ---- Connection state ----

let transport = null;
let timers = [];
let lastRx = 0;
const keys = new Map();  // uid -> { at, pressed } for Keybits the Keyword reports
let sides = {};          // from SIDES lines
let syncSeen = null;     // uids seen during a KEYS? reply

function setStatus(text, cls) {
  $('#status-text').textContent = text;
  $('#status').className = 'status ' + (cls || '');
}

function activity(text, cls) {
  const list = $('#activity');
  list.prepend(el('li', { className: cls || '' }, el('time', { textContent: new Date().toLocaleTimeString() }), el('span', { textContent: text })));
  while (list.children.length > 100) list.lastChild.remove();
}

const shortUid = (uid) => uid.slice(-4);
const keyName = (uid) => activeProfile().keys[uid]?.name || `Keybit ${shortUid(uid)}`;
const comboFor = (uid) => {
  const m = activeProfile().keys[uid];
  return m && (m.mods || m.key) ? Hid.comboLabel(m.mods, m.key, MAC) : '';
};

function addKey(uid, at, quiet) {
  const known = keys.get(uid);
  keys.set(uid, { at, pressed: known?.pressed || false });
  if (!known && !quiet) {
    activity(`${keyName(uid)} connected${at ? ` on the ${SIDE_NAMES[at]}` : ''}`, 'add');
    flashUid = uid;
  }
}
function removeKey(uid) {
  if (!keys.has(uid)) return;
  const { at } = keys.get(uid);
  keys.delete(uid);
  activity(`${keyName(uid)} disconnected${at ? ` from the ${SIDE_NAMES[at]}` : ''}`, 'remove');
}

let flashUid = null;

function render() {
  // Board
  for (const s of SIDES) {
    const uid = [...keys].find(([, k]) => k.at === s)?.[0];
    const present = !!uid || !!sides[s];
    const slot = $(`.slot[data-side="${s}"]`);
    slot.classList.toggle('present', present);
    slot.querySelector('.slot-label').textContent = !present ? '' : uid ? (comboFor(uid) || keyName(uid)) : 'Keybit';
    slot.title = uid ? `${keyName(uid)} · uid ${uid}` : '';
  }

  // Key list: connected first (left, top, right), then mapped keys that are offline.
  const order = { L: 0, T: 1, R: 2 };
  const online = [...keys].sort((a, b) => (order[a[1].at] ?? 9) - (order[b[1].at] ?? 9)).map(([u]) => u);
  const offline = Object.keys(activeProfile().keys).filter((u) => !keys.has(u)).sort();
  const list = $('#keys');
  list.replaceChildren();
  for (const uid of [...online, ...offline]) {
    const k = keys.get(uid);
    const combo = comboFor(uid);
    const where = k ? (k.at ? SIDE_NAMES[k.at][0].toUpperCase() + SIDE_NAMES[k.at].slice(1) : 'Connected') : 'Not connected';
    const btns = el('div', { className: 'btns' });
    btns.append(el('button', { className: 'ghost small', textContent: 'Assign…', onclick: () => openAssign(uid) }));
    if (k && transport instanceof DemoTransport) {
      btns.append(el('button', { className: 'ghost small', textContent: 'Tap', onclick: () => transport.tap(uid) }));
    }
    if (!k) btns.append(el('button', { className: 'ghost small', textContent: 'Forget', onclick: () => forgetKey(uid) }));
    const li = el('li', { className: [k ? '' : 'offline', k?.pressed ? 'pressed' : '', flashUid === uid ? 'flash' : ''].join(' ') },
      el('div', {}, el('div', { className: 'name', textContent: keyName(uid) }),
        el('div', { className: 'meta' }, `${where} · uid `, el('code', { textContent: uid }))),
      el('span', { className: 'combo' + (combo ? '' : ' none'), textContent: combo || 'not assigned' }),
      btns);
    li.dataset.uid = uid;
    list.append(li);
  }
  flashUid = null;
  $('#keys-empty').hidden = list.children.length > 0;
  $('#keys-empty').textContent = transport
    ? "No Keybits connected. Attach one to the Keyword's left, right or top edge."
    : 'Connect to the Keyword to see its Keybits.';

  // Profiles
  const sel = $('#profile-select');
  sel.replaceChildren(...store.profiles.map((p) => el('option', { value: p.id, textContent: p.name, selected: p.id === store.active })));
  $('#p-delete').disabled = store.profiles.length < 2;
  $('#p-apply').disabled = !transport;
}

function handleLine(line) {
  lastRx = Date.now();
  log('<-', line);
  const msg = parseLine(line);
  switch (msg.type) {
    case 'hello':
      setStatus(`Connected over ${transport.label} · Keyword firmware ${msg.fw}`, 'connected');
      if (msg.proto !== 1) log('!!', `protocol ${msg.proto}, app expects 1`);
      break;
    case 'sides':
      for (const [s, v] of Object.entries(msg.sides)) {
        sides[s] = v;
        // A KEY- may have been missed: drop keys on a side that is now empty.
        if (!v) for (const [uid, k] of keys) if (k.at === s) removeKey(uid);
      }
      break;
    case 'keyAdd':
      syncSeen?.add(msg.uid);
      addKey(msg.uid, msg.at, false);
      break;
    case 'keysEnd':
      if (syncSeen) for (const uid of [...keys.keys()]) if (!syncSeen.has(uid)) removeKey(uid);
      syncSeen = null;
      break;
    case 'keyRemove':
      removeKey(msg.uid);
      break;
    case 'press': {
      const k = keys.get(msg.uid);
      if (k) k.pressed = msg.down;
      else addKey(msg.uid, null, false);
      const li = document.querySelector(`#keys li[data-uid="${msg.uid}"]`);
      if (li) { li.classList.toggle('pressed', msg.down); return; } // skip a full re-render
      break;
    }
    case 'error':
      log('!!', msg.message);
      activity(`Keyword error: ${msg.message}`, 'remove');
      break;
  }
  render();
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

function applyProfile() {
  if (!transport) return;
  send(...profileCommands(activeProfile().keys));
  activity(`Sent profile "${activeProfile().name}" to the Keyword`);
}

async function connect(t) {
  await disconnect();
  setStatus(`Connecting over ${t.label}…`, 'busy');
  const lines = new LineBuffer(handleLine);
  try {
    await t.open((text) => lines.push(text), () => { log('!!', 'device disconnected'); disconnect(); });
  } catch (e) {
    closeDialog(chooser);
    setStatus(e.name === 'NotFoundError' ? 'Not connected' : `Could not connect: ${e.message}`);
    return;
  }
  transport = t;
  lastRx = Date.now();
  setStatus(`Connected over ${t.label} · waiting for Keyword…`, 'busy');
  const demo = t instanceof DemoTransport;
  document.body.classList.toggle('demo', demo);
  $('#demo-hint').hidden = !demo;
  $('#btn-disconnect').hidden = false;
  activity(`Connected over ${t.label}`);
  render();
  await send(cmd.hello(), cmd.sides());
  syncKeys();
  applyProfile();
  timers = [
    setInterval(() => {
      send(cmd.sides());
      if (Date.now() - lastRx > STALE_MS) setStatus(`Connected over ${t.label} · no reply from the Keyword`, 'stale');
    }, POLL_MS),
    setInterval(syncKeys, KEYS_POLL_MS),
  ];
}

async function disconnect() {
  timers.forEach(clearInterval);
  timers = [];
  const t = transport;
  transport = null;
  if (t) {
    await t.close();
    activity('Disconnected');
  }
  document.body.classList.remove('demo');
  $('#demo-hint').hidden = true;
  $('#btn-disconnect').hidden = true;
  keys.clear();
  sides = {};
  render();
  setStatus('Not connected');
}

// ---- Dialog helpers ----

function closeDialog(d) { if (d.open) d.close(); }

function askText(title, value) {
  const d = $('#prompt');
  $('#prompt-title').textContent = title;
  const input = $('#prompt-input');
  input.value = value || '';
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
const capture = $('#capture');
let assignUid = null;
let draft = { mods: 0, key: 0 };

$('#assign-mods').append(...Hid.MODS.map((m) =>
  el('label', {}, el('input', { type: 'checkbox', value: m.bit }), Hid.modName(m, MAC))));
$('#assign-key').append(el('option', { value: 0, textContent: '(no key, modifiers only)' }),
  ...Hid.KEYS.map((k) => el('option', { value: k.usage, textContent: k.label })));

function showDraft() {
  $('#assign-mods').querySelectorAll('input').forEach((c) => { c.checked = !!(draft.mods & (1 << c.value)); });
  $('#assign-key').value = draft.key;
  $('#capture-combo').textContent = Hid.comboLabel(draft.mods, draft.key, MAC) || 'Press a key or shortcut…';
}
$('#assign-mods').addEventListener('change', () => {
  draft.mods = [...$('#assign-mods').querySelectorAll('input')].reduce((m, c) => m | (c.checked ? 1 << c.value : 0), 0);
  showDraft();
});
$('#assign-key').addEventListener('change', (e) => { draft.key = Number(e.target.value); showDraft(); });

function captureKey(e) {
  const r = Hid.fromEvent(e);
  if (!r) return;
  draft = { mods: r.mods, key: r.usage };
  showDraft();
}
capture.addEventListener('keydown', (e) => { e.preventDefault(); e.stopPropagation(); captureKey(e); });
capture.addEventListener('focus', () => window.desktop?.setCapture(true));
capture.addEventListener('blur', () => window.desktop?.setCapture(false));
window.desktop?.onCaptureKey((k) => { if (k.type === 'keyDown') captureKey(k); });

function openAssign(uid) {
  assignUid = uid;
  const m = activeProfile().keys[uid];
  draft = { mods: m?.mods || 0, key: m?.key || 0 };
  $('#assign-name').textContent = keyName(uid);
  $('#assign-label').value = m?.name || '';
  $('#assign-clear').disabled = !m;
  showDraft();
  assign.showModal();
  capture.focus();
}
function closeAssign() {
  window.desktop?.setCapture(false);
  closeDialog(assign);
}
assign.addEventListener('close', () => window.desktop?.setCapture(false));
$('#assign-cancel').onclick = closeAssign;
$('#assign-save').onclick = () => {
  const uid = assignUid;
  const name = $('#assign-label').value.trim();
  const p = activeProfile();
  if (!draft.mods && !draft.key && !name) delete p.keys[uid];
  else p.keys[uid] = { mods: draft.mods, key: draft.key, ...(name ? { name } : {}) };
  persist();
  if (draft.mods || draft.key) send(cmd.map(uid, draft.mods, draft.key), cmd.save());
  else send(cmd.unmap(uid), cmd.save());
  activity(`${keyName(uid)} → ${comboFor(uid) || 'nothing'}`);
  closeAssign();
  render();
};
$('#assign-clear').onclick = () => {
  delete activeProfile().keys[assignUid];
  persist();
  send(cmd.unmap(assignUid), cmd.save());
  closeAssign();
  render();
};

function forgetKey(uid) {
  delete activeProfile().keys[uid];
  persist();
  render();
}

// ---- Profile buttons ----

function switchTo(id) {
  store.active = id;
  persist();
  render();
  applyProfile();
}
$('#profile-select').onchange = (e) => switchTo(e.target.value);
$('#p-new').onclick = async () => {
  const name = await askText('New profile name', `Profile ${store.profiles.length + 1}`);
  if (!name) return;
  const p = Profiles.newProfile(name);
  store.profiles.push(p);
  switchTo(p.id);
};
$('#p-dup').onclick = async () => {
  const cur = activeProfile();
  const name = await askText('Name for the copy', `${cur.name} copy`);
  if (!name) return;
  const p = Profiles.newProfile(name, JSON.parse(JSON.stringify(cur.keys)));
  store.profiles.push(p);
  switchTo(p.id);
};
$('#p-rename').onclick = async () => {
  const name = await askText('Rename profile', activeProfile().name);
  if (!name) return;
  activeProfile().name = name;
  persist();
  render();
};
$('#p-delete').onclick = () => {
  if (store.profiles.length < 2) return;
  if (!confirm(`Delete profile "${activeProfile().name}"?`)) return;
  store.profiles = store.profiles.filter((p) => p.id !== store.active);
  switchTo(store.profiles[0].id);
};
$('#p-export').onclick = async () => {
  const p = activeProfile();
  const text = Profiles.exportProfile(p);
  if (window.desktop) {
    if (await window.desktop.exportProfile(p.name, text)) activity(`Exported "${p.name}"`);
    return;
  }
  const a = el('a', { href: URL.createObjectURL(new Blob([text], { type: 'application/json' })), download: `${p.name}.keyword.json` });
  a.click();
  URL.revokeObjectURL(a.href);
};
function importText(text) {
  try {
    const p = Profiles.importProfile(text);
    store.profiles.push(p);
    activity(`Imported "${p.name}"`);
    switchTo(p.id);
  } catch (e) {
    activity(`Import failed: ${e.message}`, 'remove');
  }
}
$('#p-import').onclick = async () => {
  if (window.desktop) {
    const text = await window.desktop.importProfile();
    if (text != null) importText(text);
    return;
  }
  const input = el('input', { type: 'file', accept: '.json,application/json' });
  input.onchange = async () => { if (input.files[0]) importText(await input.files[0].text()); };
  input.click();
};
$('#p-apply').onclick = applyProfile;

// ---- Connection buttons ----

$('#btn-usb').onclick = () => {
  if (!('serial' in navigator)) return setStatus('Web Serial is not available here');
  connect(new SerialTransport());
};
$('#btn-ble').onclick = () => {
  if (!('bluetooth' in navigator)) return setStatus('Web Bluetooth is not available here');
  connect(new BleTransport());
};
$('#btn-demo').onclick = () => connect(new DemoTransport());
$('#btn-disconnect').onclick = () => disconnect();

document.querySelectorAll('.board .slot').forEach((slot) => {
  slot.addEventListener('click', () => {
    if (transport instanceof DemoTransport) transport.toggle(slot.dataset.side);
  });
});

storage.load().then((s) => { store = Profiles.normalizeStore(s); render(); });
render();
