// Keyword host protocol: newline-terminated ASCII lines, the same over USB CDC and BLE.
// See PROTOCOL.md for the full description.
import type { Entry, Side, Step } from './types.ts';
import type { Compass } from './layout.ts';

export const SIDES: readonly Side[] = ['L', 'R', 'T'];

/** Nordic UART Service, used as a plain byte pipe over BLE. */
export const NUS = {
  service: '6e400001-b5a3-f393-e0a9-e50e24dcca9e',
  rx: '6e400002-b5a3-f393-e0a9-e50e24dcca9e', // host writes here
  tx: '6e400003-b5a3-f393-e0a9-e50e24dcca9e', // keyword notifies here
} as const;

/** ST BLE_Ota bootloader on the Keyword, reached with the OTA command. */
export const OTA = {
  name: 'Keyword OTA',
  service: '0000fe20-cc7a-482a-984a-7f2ed5b3e58f',
  base: '0000fe22-8e22-4541-9d4c-21edae82ed19', // write: [cmd, offset >> 16, >> 8, offset]
  confirm: '0000fe23-8e22-4541-9d4c-21edae82ed19', // indicate: 0x01 once the image is accepted
  raw: '0000fe24-8e22-4541-9d4c-21edae82ed19', // write without response: image bytes, in order
  START: 0x02,
  FINISH: 0x07,
  CANCEL: 0x08,
  FLASH_BASE: 0x08000000,
  APP_OFFSET: 0x7000, // the app lives at 0x08007000, behind the bootloader
  MAX_SIZE: 0x08100000 - 0x08007000,
} as const;

export const MAX_MACRO_STEPS = 16;
export const DEFAULT_HOLD_MS = 200;

export const otaBaseCommand = (op: number, offset = 0) =>
  new Uint8Array([op, (offset >> 16) & 0xff, (offset >> 8) & 0xff, offset & 0xff]);

/** Sanity checks an app image before sending it: the right file, linked for 0x08007000. */
export function checkAppImage(bytes: Uint8Array | null): { ok: true; size: number } | { ok: false; error: string } {
  if (!bytes || bytes.length < 64) return { ok: false, error: 'The file is too small to be a Keyword firmware image.' };
  if (bytes.length > OTA.MAX_SIZE) return { ok: false, error: 'The file is too big for the Keyword.' };
  const word = (i: number) => (bytes[i] | (bytes[i + 1] << 8) | (bytes[i + 2] << 16) | (bytes[i + 3] << 24)) >>> 0;
  const sp = word(0);
  const reset = word(4);
  if (sp < 0x20000000 || sp > 0x20040000 || !(reset & 1)) {
    return { ok: false, error: "This doesn't look like a Keyword firmware image (no vector table at the start)." };
  }
  const appStart = OTA.FLASH_BASE + OTA.APP_OFFSET;
  if (reset < appStart || reset >= appStart + bytes.length) {
    return {
      ok: false,
      error: reset < appStart
        ? 'This image starts at 0x08000000, so it is the full image for USB DFU. Pick keyword_app.bin instead.'
        : 'This image is not linked for 0x08007000.',
    };
  }
  return { ok: true, size: bytes.length };
}

/**
 * Splits an image into OTA writes. The bootloader programs flash 8 bytes at a time,
 * so chunks are multiples of 8 and the tail is padded with 0xFF (erased flash).
 */
export function otaChunks(bytes: Uint8Array, size: number): Uint8Array<ArrayBuffer>[] {
  const step = Math.max(8, size - (size % 8));
  const padded = new Uint8Array(Math.ceil(bytes.length / 8) * 8).fill(0xff);
  padded.set(bytes);
  const out: Uint8Array<ArrayBuffer>[] = [];
  for (let i = 0; i < padded.length; i += step) out.push(padded.subarray(i, i + step));
  return out;
}

/** Splits a stream into lines ending in \n, \r\n or a bare \r. Handles partial chunks. */
export class LineBuffer {
  private buf = '';
  private readonly onLine: (line: string) => void;
  constructor(onLine: (line: string) => void) {
    this.onLine = onLine;
  }
  push(text: string) {
    this.buf += text;
    let i: number;
    while ((i = this.buf.search(/[\r\n]/)) >= 0) {
      const line = this.buf.slice(0, i).trim();
      this.buf = this.buf.slice(i + 1); // a \r\n leaves an empty line, which is skipped
      if (line) this.onLine(line);
    }
    if (this.buf.length > 4096) this.buf = ''; // garbage with no newline
  }
}

/** Parses "KEY=val KEY=val" into an object. */
function parseFields(parts: string[]): Record<string, string> {
  const out: Record<string, string> = {};
  for (const p of parts) {
    const eq = p.indexOf('=');
    if (eq > 0) out[p.slice(0, eq)] = p.slice(eq + 1);
  }
  return out;
}

export const isUid = (u: unknown): u is string => typeof u === 'string' && /^([0-9a-fA-F]{24}|[0-9a-fA-F]{8})$/.test(u);
const hex2 = (n: number) => (n & 0xff).toString(16).toUpperCase().padStart(2, '0');
const hex4 = (n: number) => (n & 0xffff).toString(16).toUpperCase().padStart(4, '0');
const int = (s: string | undefined, radix = 10) => {
  const n = parseInt(s ?? '', radix);
  return Number.isFinite(n) ? n : 0;
};

/** Host -> Keyword commands. */
export const cmd = {
  hello: () => 'HELLO',
  sides: () => 'SIDES?',
  keys: () => 'KEYS?',
  battery: () => 'BATT?',
  map: (uid: string, mods: number, key: number) => `MAP uid=${uid} mods=${hex2(mods)} key=${hex2(key)}`,
  mapConsumer: (uid: string, usage: number) => `MAPC uid=${uid} usage=${hex4(usage)}`,
  macro: (uid: string, steps: Step[]) =>
    `MACRO uid=${uid} seq=${steps.slice(0, MAX_MACRO_STEPS).map(([m, k]) => hex2(m) + hex2(k)).join(',')}`,
  hold: (uid: string, mods: number, key: number, ms?: number) =>
    `HOLD uid=${uid} mods=${hex2(mods)} key=${hex2(key)} ms=${Math.round(ms || DEFAULT_HOLD_MS)}`,
  mapQuery: (uid: string) => `MAP? uid=${uid}`,
  unmap: (uid: string) => `UNMAP uid=${uid}`,
  flash: (uid: string) => `FLASH uid=${uid}`,
  ota: () => 'OTA',
  dfu: () => 'DFU',
  stack: () => 'STACK?',
  clear: () => 'CLEAR',
  save: () => 'SAVE',
};

export const hasHold = (m: Entry) => !!(m.hold && (m.hold.mods || m.hold.key));

/**
 * One profile entry -> the commands that program it into the Keyword.
 * Computer actions (run by the app) and empty entries need nothing.
 * caps is the set from HELLO; commands it doesn't list are left out.
 */
export function entryCommands(uid: string, m: Entry, caps: ReadonlySet<string>): string[] {
  const out: string[] = [];
  if (m.host) return out;
  if (m.macro?.length) {
    if (caps.has('macro')) out.push(cmd.macro(uid, m.macro));
  } else if (m.consumer) out.push(cmd.mapConsumer(uid, m.consumer));
  else if (m.mods || m.key) out.push(cmd.map(uid, m.mods || 0, m.key || 0));
  if (m.hold && hasHold(m) && caps.has('hold')) out.push(cmd.hold(uid, m.hold.mods || 0, m.hold.key || 0, m.hold.ms));
  return out;
}

/** The main MAP/MAPC command for an entry, or null. */
export function mapCommand(uid: string, m: Entry): string | null {
  if (m.consumer) return cmd.mapConsumer(uid, m.consumer);
  if (m.mods || m.key) return cmd.map(uid, m.mods || 0, m.key || 0);
  return null;
}

/** Commands that make the Keyword match a profile exactly. */
export function profileCommands(keys: Record<string, Entry>, caps: ReadonlySet<string> = new Set()): string[] {
  const out = [cmd.clear()];
  for (const [uid, m] of Object.entries(keys)) {
    if (isUid(uid)) out.push(...entryCommands(uid.toUpperCase(), m, caps));
  }
  out.push(cmd.save());
  return out;
}

export type Message =
  | { type: 'hello'; fw: string; proto: number; caps: Set<string>; kb: number | null }
  | { type: 'sides'; sides: Partial<Record<Side, boolean>> }
  | {
      type: 'keyAdd'; uid: string; at: Side | null; v: number | null; via?: string | null; depth?: number | null;
      face?: Compass | null; parent?: string | null; pside?: Compass | null;
    }
  | { type: 'keyRemove'; uid: string }
  | { type: 'keysEnd' }
  | { type: 'press'; uid: string; down: boolean }
  | { type: 'backwards'; uid: string; sides: string | null }
  | { type: 'map'; uid: string; mods: number; key: number }
  | { type: 'mapConsumer'; uid: string; usage: number }
  | { type: 'macro'; uid: string; steps: Step[] }
  | { type: 'hold'; uid: string; mods: number; key: number; ms: number }
  | { type: 'battery'; pct: number; charging: boolean; usb: boolean }
  | { type: 'flashProgress'; uid: string; pct: number }
  | { type: 'stack'; stack: string; v: string | null; fus: string | null }
  | { type: 'ok'; what: string; uid?: string; [field: string]: string | undefined }
  | { type: 'error'; message: string }
  | { type: 'unknown'; line: string };

const isSide = (s: string | undefined): s is Side => SIDES.includes(s as Side);
const isCompass = (s: string | undefined): s is Compass => s === 'N' || s === 'E' || s === 'S' || s === 'W';

/** Parses one line from the Keyword. Anything unrecognised comes back as {type: 'unknown'}. */
export function parseLine(line: string): Message {
  const parts = line.trim().split(/\s+/);
  const name = parts[0];
  const f = parseFields(parts.slice(1));
  const uid = isUid(f.uid) ? f.uid.toUpperCase() : null;
  switch (name) {
    case 'KEYWORD':
      return {
        type: 'hello',
        fw: f.v || '?',
        proto: int(f.proto),
        caps: new Set((f.caps || '').split(',').filter(Boolean)),
        kb: f.kb != null ? int(f.kb) : null,
      };
    case 'SIDES': {
      const sides: Partial<Record<Side, boolean>> = {};
      for (const s of SIDES) if (s in f) sides[s] = f[s] === '1';
      if (!Object.keys(sides).length) break;
      return { type: 'sides', sides };
    }
    case 'KEY+':
      if (!uid) break;
      return {
        type: 'keyAdd',
        uid,
        at: isSide(f.at) ? f.at : null,
        v: f.v != null ? int(f.v) : null,
        // Keybits relayed through another Keybit: at=X, via=<uid of the next Keybit in>, depth=<hops>.
        ...(f.via ? { via: isUid(f.via) ? f.via.toUpperCase() : null, depth: int(f.depth) || null } : {}),
        // v11: its own side facing the Keyword or its parent, and for a chained one
        // the parent (low 32 bits of its UID) and that Keybit's side it's on.
        ...(isCompass(f.face) ? { face: f.face } : {}),
        ...(f.parent && isUid(f.parent) && isCompass(f.pside) ? { parent: f.parent.toUpperCase(), pside: f.pside } : {}),
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
    case 'BACKWARDS':
      // BACKWARDS uid=<uid> sides=<NESW letters | ? | ->
      if (!uid) break;
      return { type: 'backwards', uid, sides: !f.sides || f.sides === '-' ? null : f.sides };
    case 'MAP':
      if (!uid) break;
      return { type: 'map', uid, mods: int(f.mods, 16), key: int(f.key, 16) };
    case 'MAPC':
      if (!uid) break;
      return { type: 'mapConsumer', uid, usage: int(f.usage, 16) };
    case 'MACRO':
      if (!uid) break;
      return {
        type: 'macro',
        uid,
        steps: (f.seq || '').split(',').filter((s) => /^[0-9a-fA-F]{4}$/.test(s))
          .map((s): Step => [int(s.slice(0, 2), 16), int(s.slice(2), 16)]),
      };
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
