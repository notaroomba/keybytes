// Keyword host protocol: newline-terminated ASCII lines, same over USB CDC and BLE.
// See ../README.md for the full description.
(function (root) {
  const SIDES = ['L', 'R', 'T'];

  // Nordic UART Service, used as a plain byte pipe over BLE.
  const NUS = {
    service: '6e400001-b5a3-f393-e0a9-e50e24dcca9e',
    rx: '6e400002-b5a3-f393-e0a9-e50e24dcca9e', // host writes here
    tx: '6e400003-b5a3-f393-e0a9-e50e24dcca9e', // keyword notifies here
  };

  // Splits a byte/char stream into lines. Handles \r\n and partial chunks.
  class LineBuffer {
    constructor(onLine) {
      this.buf = '';
      this.onLine = onLine;
    }
    push(text) {
      this.buf += text;
      let i;
      while ((i = this.buf.indexOf('\n')) >= 0) {
        const line = this.buf.slice(0, i).replace(/\r$/, '').trim();
        this.buf = this.buf.slice(i + 1);
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

  // One profile entry -> the MAP/MAPC command for it, or null if it maps nothing.
  function mapCommand(uid, m) {
    if (m.consumer) return cmd.mapConsumer(uid, m.consumer);
    if (m.mods || m.key) return cmd.map(uid, m.mods || 0, m.key || 0);
    return null;
  }

  // Host -> Keyword commands.
  const cmd = {
    hello: () => 'HELLO',
    sides: () => 'SIDES?',
    keys: () => 'KEYS?',
    map: (uid, mods, key) => `MAP uid=${uid} mods=${hex2(mods)} key=${hex2(key)}`,
    mapConsumer: (uid, usage) => `MAPC uid=${uid} usage=${hex4(usage)}`,
    mapQuery: (uid) => `MAP? uid=${uid}`,
    unmap: (uid) => `UNMAP uid=${uid}`,
    clear: () => 'CLEAR',
    save: () => 'SAVE',
  };

  // Commands that make the Keyword match a profile exactly.
  function profileCommands(keys) {
    const out = [cmd.clear()];
    for (const [uid, m] of Object.entries(keys)) {
      const c = isUid(uid) && mapCommand(uid.toUpperCase(), m);
      if (c) out.push(c);
    }
    out.push(cmd.save());
    return out;
  }

  // Returns {type, ...} or {type: 'unknown', line}.
  function parseLine(line) {
    const parts = line.trim().split(/\s+/);
    const cmd = parts[0];
    const f = parseFields(parts.slice(1));
    switch (cmd) {
      case 'KEYWORD':
        return { type: 'hello', fw: f.v || '?', proto: Number(f.proto || 0) };
      case 'SIDES': {
        const sides = {};
        for (const s of SIDES) if (s in f) sides[s] = f[s] === '1';
        if (!Object.keys(sides).length) break;
        return { type: 'sides', sides };
      }
      case 'KEY+':
        if (!isUid(f.uid)) break;
        return { type: 'keyAdd', uid: f.uid.toUpperCase(), at: SIDES.includes(f.at) ? f.at : null };
      case 'KEY-':
        if (!isUid(f.uid)) break;
        return { type: 'keyRemove', uid: f.uid.toUpperCase() };
      case 'KEYS':
        if (parts[1] === 'END') return { type: 'keysEnd' };
        break;
      case 'PRESS':
        if (!isUid(f.uid)) break;
        return { type: 'press', uid: f.uid.toUpperCase(), down: f.down === '1' };
      case 'MAP':
        if (!isUid(f.uid)) break;
        return { type: 'map', uid: f.uid.toUpperCase(), mods: parseInt(f.mods || '0', 16) || 0, key: parseInt(f.key || '0', 16) || 0 };
      case 'MAPC':
        if (!isUid(f.uid)) break;
        return { type: 'mapConsumer', uid: f.uid.toUpperCase(), usage: parseInt(f.usage || '0', 16) || 0 };
      case 'OK':
        return { type: 'ok', what: parts[1] || '', ...parseFields(parts.slice(2)) };
      case 'ERR':
        return { type: 'error', message: parts.slice(1).join(' ') };
    }
    return { type: 'unknown', line };
  }

  const api = { SIDES, NUS, LineBuffer, parseLine, cmd, mapCommand, profileCommands, isUid };
  if (typeof module !== 'undefined' && module.exports) module.exports = api;
  else root.KeywordProtocol = api;
})(typeof window !== 'undefined' ? window : globalThis);
