// Keyword host protocol: newline-terminated ASCII lines, same over USB CDC and BLE.
// See ../PROTOCOL.md for the full description.
(function (root) {
  const SIDES = ['L', 'R', 'T'];

  // Nordic UART Service, used as a plain byte pipe over BLE.
  const NUS = {
    service: '6e400001-b5a3-f393-e0a9-e50e24dcca9e',
    rx: '6e400002-b5a3-f393-e0a9-e50e24dcca9e', // host writes here
    tx: '6e400003-b5a3-f393-e0a9-e50e24dcca9e', // keyword notifies here
  };

  // ST BLE_Ota bootloader on the Keyword, reached with the OTA command. See PROTOCOL.md.
  const OTA = {
    name: 'Keyword OTA',
    service: '0000fe20-cc7a-482a-984a-7f2ed5b3e58f',
    base: '0000fe22-8e22-4541-9d4c-21edae82ed19',    // write: [cmd, offset >> 16, >> 8, offset]
    confirm: '0000fe23-8e22-4541-9d4c-21edae82ed19', // indicate: 0x01 once the image is accepted
    raw: '0000fe24-8e22-4541-9d4c-21edae82ed19',     // write without response: image bytes, in order
    START: 0x02, FINISH: 0x07, CANCEL: 0x08,
    FLASH_BASE: 0x08000000,
    APP_OFFSET: 0x7000, // the app lives at 0x08007000, behind the bootloader
    MAX_SIZE: 0x08100000 - 0x08007000,
  };
  const otaBaseCommand = (op, offset = 0) => new Uint8Array([op, (offset >> 16) & 0xff, (offset >> 8) & 0xff, offset & 0xff]);

  // Sanity checks an app image before sending it: the right file, linked for 0x08007000.
  function checkAppImage(bytes) {
    if (!bytes || bytes.length < 64) return { ok: false, error: 'The file is too small to be a Keyword firmware image.' };
    if (bytes.length > OTA.MAX_SIZE) return { ok: false, error: 'The file is too big for the Keyword.' };
    const word = (i) => (bytes[i] | (bytes[i + 1] << 8) | (bytes[i + 2] << 16) | (bytes[i + 3] << 24)) >>> 0;
    const sp = word(0), reset = word(4);
    if (sp < 0x20000000 || sp > 0x20040000 || !(reset & 1)) {
      return { ok: false, error: "This doesn't look like a Keyword firmware image (no vector table at the start)." };
    }
    const appStart = OTA.FLASH_BASE + OTA.APP_OFFSET;
    if (reset < appStart || reset >= appStart + bytes.length) {
      return { ok: false, error: reset < appStart
        ? 'This image starts at 0x08000000, so it is the full image for USB DFU. Pick keyword_app.bin instead.'
        : 'This image is not linked for 0x08007000.' };
    }
    return { ok: true, size: bytes.length };
  }

  // Splits an image into OTA writes. The bootloader programs flash 8 bytes at a time,
  // so chunks are multiples of 8 and the tail is padded with 0xFF (erased flash).
  function otaChunks(bytes, size) {
    const step = Math.max(8, size - (size % 8));
    const padded = new Uint8Array(Math.ceil(bytes.length / 8) * 8).fill(0xff);
    padded.set(bytes);
    const out = [];
    for (let i = 0; i < padded.length; i += step) out.push(padded.subarray(i, i + step));
    return out;
  }

  const MAX_MACRO_STEPS = 16;
  const DEFAULT_HOLD_MS = 200;

  // Splits a byte/char stream into lines ending in \n, \r\n or a bare \r. Handles partial chunks.
  class LineBuffer {
    constructor(onLine) {
      this.buf = '';
      this.onLine = onLine;
    }
    push(text) {
      this.buf += text;
      let i;
      while ((i = this.buf.search(/[\r\n]/)) >= 0) {
        const line = this.buf.slice(0, i).trim();
        this.buf = this.buf.slice(i + 1); // a \r\n leaves an empty line, which is skipped
        if (line) this.onLine(line);
      }
      if (this.buf.length > 4096) this.buf = ''; // garbage with no newline
    }
  }

  // Parses "KEY=val KEY=val" into an object.
  function parseFields(parts) {
    const out = {};
    for (const p of parts) {
      const eq = p.indexOf('=');
      if (eq > 0) out[p.slice(0, eq)] = p.slice(eq + 1);
    }
    return out;
  }

  const isUid = (u) => typeof u === 'string' && /^([0-9a-fA-F]{24}|[0-9a-fA-F]{8})$/.test(u);
  const hex2 = (n) => (n & 0xff).toString(16).toUpperCase().padStart(2, '0');
  const hex4 = (n) => (n & 0xffff).toString(16).toUpperCase().padStart(4, '0');
  const int = (s, radix = 10) => { const n = parseInt(s, radix); return Number.isFinite(n) ? n : 0; };

  // Host -> Keyword commands.
  const cmd = {
    hello: () => 'HELLO',
    sides: () => 'SIDES?',
    keys: () => 'KEYS?',
    battery: () => 'BATT?',
    map: (uid, mods, key) => `MAP uid=${uid} mods=${hex2(mods)} key=${hex2(key)}`,
    mapConsumer: (uid, usage) => `MAPC uid=${uid} usage=${hex4(usage)}`,
    macro: (uid, steps) => `MACRO uid=${uid} seq=${steps.slice(0, MAX_MACRO_STEPS).map(([m, k]) => hex2(m) + hex2(k)).join(',')}`,
    hold: (uid, mods, key, ms) => `HOLD uid=${uid} mods=${hex2(mods)} key=${hex2(key)} ms=${Math.round(ms || DEFAULT_HOLD_MS)}`,
    mapQuery: (uid) => `MAP? uid=${uid}`,
    unmap: (uid) => `UNMAP uid=${uid}`,
    flash: (uid) => `FLASH uid=${uid}`,
    ota: () => 'OTA',
    dfu: () => 'DFU',
    stack: () => 'STACK?',
    clear: () => 'CLEAR',
    save: () => 'SAVE',
  };

  const hasHold = (m) => !!(m.hold && (m.hold.mods || m.hold.key));

  // One profile entry -> the commands that program it into the Keyword.
  // Host actions (run by the app) and empty entries need nothing.
  // caps is the set from HELLO; commands it doesn't list are left out.
  function entryCommands(uid, m, caps) {
    const out = [];
    if (m.host) return out;
    if (m.macro?.length) {
      if (caps.has('macro')) out.push(cmd.macro(uid, m.macro));
    } else if (m.consumer) out.push(cmd.mapConsumer(uid, m.consumer));
    else if (m.mods || m.key) out.push(cmd.map(uid, m.mods || 0, m.key || 0));
    if (hasHold(m) && caps.has('hold')) out.push(cmd.hold(uid, m.hold.mods || 0, m.hold.key || 0, m.hold.ms));
    return out;
  }

  // Back-compat helper: the main MAP/MAPC command for an entry, or null.
  function mapCommand(uid, m) {
    if (m.consumer) return cmd.mapConsumer(uid, m.consumer);
    if (m.mods || m.key) return cmd.map(uid, m.mods || 0, m.key || 0);
    return null;
  }

  // Commands that make the Keyword match a profile exactly.
  function profileCommands(keys, caps = new Set()) {
    const out = [cmd.clear()];
    for (const [uid, m] of Object.entries(keys)) {
      if (isUid(uid)) out.push(...entryCommands(uid.toUpperCase(), m, caps));
    }
    out.push(cmd.save());
    return out;
  }

  // Returns {type, ...} or {type: 'unknown', line}.
  function parseLine(line) {
    const parts = line.trim().split(/\s+/);
    const name = parts[0];
    const f = parseFields(parts.slice(1));
    const uid = isUid(f.uid) ? f.uid.toUpperCase() : null;
    switch (name) {
      case 'KEYWORD':
        return {
          type: 'hello', fw: f.v || '?', proto: int(f.proto),
          caps: new Set((f.caps || '').split(',').filter(Boolean)),
          kb: f.kb != null ? int(f.kb) : null,
        };
      case 'SIDES': {
        const sides = {};
        for (const s of SIDES) if (s in f) sides[s] = f[s] === '1';
        if (!Object.keys(sides).length) break;
        return { type: 'sides', sides };
      }
      case 'KEY+':
        if (!uid) break;
        return {
          type: 'keyAdd', uid, at: SIDES.includes(f.at) ? f.at : null, v: f.v != null ? int(f.v) : null,
          // Keybits relayed through another Keybit: at=X, via=<uid of the next Keybit in>, depth=<hops>.
          ...(f.via ? { via: isUid(f.via) ? f.via.toUpperCase() : null, depth: int(f.depth) || null } : {}),
        };
      case 'KEY-':
        if (!uid) break;
        return { type: 'keyRemove', uid };
      case 'KEYS':
        if (parts[1] === 'END') return { type: 'keysEnd' };
        break;
      case 'PRESS':
        if (!uid) break;
        return { type: 'press', uid, down: f.down === '1' };
      case 'MAP':
        if (!uid) break;
        return { type: 'map', uid, mods: int(f.mods, 16), key: int(f.key, 16) };
      case 'MAPC':
        if (!uid) break;
        return { type: 'mapConsumer', uid, usage: int(f.usage, 16) };
      case 'MACRO':
        if (!uid) break;
        return { type: 'macro', uid, steps: (f.seq || '').split(',').filter((s) => /^[0-9a-fA-F]{4}$/.test(s)).map((s) => [int(s.slice(0, 2), 16), int(s.slice(2), 16)]) };
      case 'HOLD':
        if (!uid) break;
        return { type: 'hold', uid, mods: int(f.mods, 16), key: int(f.key, 16), ms: int(f.ms) || DEFAULT_HOLD_MS };
      case 'BATT':
        if (f.pct == null) break;
        return { type: 'battery', pct: Math.max(0, Math.min(100, int(f.pct))), charging: f.chg === '1', usb: f.usb === '1' };
      case 'FLASH':
        if (!uid) break;
        return { type: 'flashProgress', uid, pct: Math.max(0, Math.min(100, int(f.pct))) };
      case 'STACK':
        // STACK ble v=1.24.0 fus=2.2.0 | STACK none fus=x.y.z | STACK unknown
        return { type: 'stack', stack: parts[1] || 'unknown', v: f.v || null, fus: f.fus || null };
      case 'OK':
        return { type: 'ok', what: parts[1] || '', ...parseFields(parts.slice(2)) };
      case 'ERR':
        return { type: 'error', message: parts.slice(1).join(' ') };
    }
    return { type: 'unknown', line };
  }

  const api = {
    SIDES, NUS, MAX_MACRO_STEPS, DEFAULT_HOLD_MS, LineBuffer, parseLine, cmd,
    mapCommand, entryCommands, profileCommands, isUid, hasHold,
    OTA, otaBaseCommand, checkAppImage, otaChunks,
  };
  if (typeof module !== 'undefined' && module.exports) module.exports = api;
  else root.KeywordProtocol = api;
})(typeof window !== 'undefined' ? window : globalThis);
