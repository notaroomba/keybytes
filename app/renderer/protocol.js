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
  };
  if (typeof module !== 'undefined' && module.exports) module.exports = api;
  else root.KeywordProtocol = api;
})(typeof window !== 'undefined' ? window : globalThis);
