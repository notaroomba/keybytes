// USB HID keyboard usages (HID Usage Tables, page 0x07) keyed by KeyboardEvent.code.
import type { Step } from './types.ts';

export interface HidKey {
  code: string;
  usage: number;
  label: string;
}

export const KEYS: HidKey[] = [];
const add = (code: string, usage: number, label: string) => KEYS.push({ code, usage, label });

for (let i = 0; i < 26; i++) {
  const c = String.fromCharCode(65 + i);
  add(`Key${c}`, 0x04 + i, c);
}
'1234567890'.split('').forEach((d, i) => add(`Digit${d}`, 0x1e + i, d));
([
  ['Enter', 0x28, 'Enter'], ['Escape', 0x29, 'Esc'], ['Backspace', 0x2a, 'Backspace'],
  ['Tab', 0x2b, 'Tab'], ['Space', 0x2c, 'Space'], ['Minus', 0x2d, '-'], ['Equal', 0x2e, '='],
  ['BracketLeft', 0x2f, '['], ['BracketRight', 0x30, ']'], ['Backslash', 0x31, '\\'],
  ['Semicolon', 0x33, ';'], ['Quote', 0x34, "'"], ['Backquote', 0x35, '`'], ['Comma', 0x36, ','],
  ['Period', 0x37, '.'], ['Slash', 0x38, '/'], ['CapsLock', 0x39, 'Caps Lock'],
] as const).forEach(([code, usage, label]) => add(code, usage, label));
for (let i = 1; i <= 12; i++) add(`F${i}`, 0x39 + i, `F${i}`);
([
  ['PrintScreen', 0x46, 'Print Screen'], ['ScrollLock', 0x47, 'Scroll Lock'], ['Pause', 0x48, 'Pause'],
  ['Insert', 0x49, 'Insert'], ['Home', 0x4a, 'Home'], ['PageUp', 0x4b, 'Page Up'],
  ['Delete', 0x4c, 'Delete'], ['End', 0x4d, 'End'], ['PageDown', 0x4e, 'Page Down'],
  ['ArrowRight', 0x4f, '→'], ['ArrowLeft', 0x50, '←'], ['ArrowDown', 0x51, '↓'], ['ArrowUp', 0x52, '↑'],
  ['NumLock', 0x53, 'Num Lock'], ['NumpadDivide', 0x54, 'Num /'], ['NumpadMultiply', 0x55, 'Num *'],
  ['NumpadSubtract', 0x56, 'Num -'], ['NumpadAdd', 0x57, 'Num +'], ['NumpadEnter', 0x58, 'Num Enter'],
] as const).forEach(([code, usage, label]) => add(code, usage, label));
for (let i = 1; i <= 9; i++) add(`Numpad${i}`, 0x58 + i, `Num ${i}`);
([
  ['Numpad0', 0x62, 'Num 0'], ['NumpadDecimal', 0x63, 'Num .'], ['IntlBackslash', 0x64, 'Intl \\'],
  ['ContextMenu', 0x65, 'Menu'], ['NumpadEqual', 0x67, 'Num ='],
] as const).forEach(([code, usage, label]) => add(code, usage, label));
for (let i = 13; i <= 24; i++) add(`F${i}`, 0x5b + i, `F${i}`);

/** Media keys go through the Consumer page (0x0C), sent with MAPC. Keyed by KeyboardEvent.code where there is one. */
export const CONSUMER: HidKey[] = ([
  ['MediaPlayPause', 0xcd, 'Play/Pause'], ['MediaTrackNext', 0xb5, 'Next Track'],
  ['MediaTrackPrevious', 0xb6, 'Previous Track'], ['MediaStop', 0xb7, 'Stop'],
  ['AudioVolumeMute', 0xe2, 'Mute'], ['AudioVolumeUp', 0xe9, 'Volume Up'],
  ['AudioVolumeDown', 0xea, 'Volume Down'], ['', 0x6f, 'Brightness Up'],
  ['', 0x70, 'Brightness Down'], ['LaunchApp2', 0x192, 'Calculator'],
  ['LaunchMail', 0x18a, 'Mail'], ['BrowserSearch', 0x221, 'Search'],
  ['BrowserHome', 0x223, 'Browser Home'],
] as const).map(([code, usage, label]) => ({ code, usage, label }));
const consumerByCode = new Map(CONSUMER.filter((c) => c.code).map((c) => [c.code, c]));
const consumerByUsage = new Map(CONSUMER.map((c) => [c.usage, c]));

const byCode = new Map(KEYS.map((k) => [k.code, k]));
export const byUsage = new Map(KEYS.map((k) => [k.usage, k]));

export interface Modifier {
  bit: number;
  code: string;
  name: string;
}

/** Modifier byte bits, as in the HID boot keyboard report. */
export const MODS: Modifier[] = [
  { bit: 0, code: 'ControlLeft', name: 'Ctrl' },
  { bit: 1, code: 'ShiftLeft', name: 'Shift' },
  { bit: 2, code: 'AltLeft', name: 'Alt' },
  { bit: 3, code: 'MetaLeft', name: 'GUI' },
  { bit: 4, code: 'ControlRight', name: 'Right Ctrl' },
  { bit: 5, code: 'ShiftRight', name: 'Right Shift' },
  { bit: 6, code: 'AltRight', name: 'Right Alt' },
  { bit: 7, code: 'MetaRight', name: 'Right GUI' },
];
const modByCode = new Map(MODS.map((m) => [m.code, m]));

export function modName(m: Modifier, mac: boolean): string {
  if (!mac) return m.name.replace('GUI', 'Win');
  return m.name.replace('GUI', 'Cmd').replace('Alt', 'Option');
}

export function consumerLabel(usage: number): string {
  return consumerByUsage.get(usage)?.label || `Media 0x${usage.toString(16).padStart(4, '0')}`;
}

/** "Ctrl+Shift+A", "Cmd+Space", "Shift" (modifier only), "" when empty. */
export function comboLabel(mods: number, usage: number, mac: boolean): string {
  const parts = MODS.filter((m) => mods & (1 << m.bit)).map((m) => modName(m, mac));
  if (usage) parts.push(byUsage.get(usage)?.label || `0x${usage.toString(16).padStart(2, '0')}`);
  return parts.join('+');
}

export interface KeyLike {
  code: string;
  ctrlKey?: boolean;
  shiftKey?: boolean;
  altKey?: boolean;
  metaKey?: boolean;
}
export type Captured = { mods: number; usage: number; consumer?: undefined } | { consumer: number; mods?: undefined; usage?: undefined };

/**
 * Turns a key event into {mods, usage}, or {consumer} for a media key. usage is 0 when only
 * modifiers are held. Returns null for keys HID can't send.
 */
export function fromEvent(e: KeyLike): Captured | null {
  const media = consumerByCode.get(e.code);
  if (media) return { consumer: media.usage };
  const mods = (e.ctrlKey ? 1 : 0) | (e.shiftKey ? 2 : 0) | (e.altKey ? 4 : 0) | (e.metaKey ? 8 : 0);
  const mod = modByCode.get(e.code);
  if (mod) return { mods: mods | (1 << mod.bit), usage: 0 };
  const k = byCode.get(e.code);
  if (!k) return null;
  return { mods, usage: k.usage };
}

// Characters on a US layout -> [mods, usage] taps, for typing text with a macro.
const SHIFTED: Record<string, string> = {
  '!': '1', '@': '2', '#': '3', $: '4', '%': '5', '^': '6', '&': '7', '*': '8', '(': '9', ')': '0',
  _: '-', '+': '=', '{': '[', '}': ']', '|': '\\', ':': ';', '"': "'", '~': '`', '<': ',', '>': '.', '?': '/',
};
const PLAIN: Record<string, number> = {
  '-': 0x2d, '=': 0x2e, '[': 0x2f, ']': 0x30, '\\': 0x31, ';': 0x33, "'": 0x34, '`': 0x35,
  ',': 0x36, '.': 0x37, '/': 0x38, ' ': 0x2c, '\n': 0x28, '\t': 0x2b,
};
function charToStep(ch: string): Step | null {
  if (/[a-z]/.test(ch)) return [0, 0x04 + ch.charCodeAt(0) - 97];
  if (/[A-Z]/.test(ch)) return [2, 0x04 + ch.charCodeAt(0) - 65];
  if (/[1-9]/.test(ch)) return [0, 0x1e + Number(ch) - 1];
  if (ch === '0') return [0, 0x27];
  if (ch in PLAIN) return [0, PLAIN[ch]];
  if (ch in SHIFTED) {
    const base = charToStep(SHIFTED[ch]);
    return base && [2, base[1]];
  }
  return null;
}

/** Returns the taps that type text, and the characters with no US-layout key. */
export function textToSteps(text: string): { steps: Step[]; skipped: string[] } {
  const steps: Step[] = [];
  const skipped: string[] = [];
  for (const ch of text) {
    const st = charToStep(ch);
    if (st) steps.push(st);
    else skipped.push(ch);
  }
  return { steps, skipped };
}

const charByStep = new Map<string, string>();
for (let c = 32; c < 127; c++) {
  const ch = String.fromCharCode(c);
  const st = charToStep(ch);
  if (st && !charByStep.has(st.join(':'))) charByStep.set(st.join(':'), ch);
}
const stepChar = (st: Step) => charByStep.get(st.join(':')) ?? null;

export const stepLabel = ([mods, key]: Step, mac: boolean) => comboLabel(mods, key, mac);

/** Label for a whole sequence. Runs of printable characters read as text: '"Hi", Cmd+C'. */
export function macroLabel(steps: Step[], mac: boolean): string {
  const out: string[] = [];
  let text = '';
  const flush = () => {
    if (text) out.push(`"${text}"`);
    text = '';
  };
  for (const st of steps) {
    const ch = stepChar(st);
    if (ch != null) text += ch;
    else {
      flush();
      out.push(stepLabel(st, mac));
    }
  }
  flush();
  return out.join(', ');
}

export interface SearchResult {
  kind: 'k' | 'c';
  usage: number;
  label: string;
}

/** Search over keys and media keys for the Assign dialog, best match first. */
export function search(query: string): SearchResult[] {
  const q = query.trim().toLowerCase();
  const all = [
    ...KEYS.map((k) => ({ kind: 'k' as const, usage: k.usage, label: k.label, hay: `${k.label} ${k.code}`.toLowerCase() })),
    ...CONSUMER.map((c) => ({ kind: 'c' as const, usage: c.usage, label: c.label, hay: `${c.label} ${c.code} media`.toLowerCase() })),
  ];
  const strip = ({ kind, usage, label }: SearchResult): SearchResult => ({ kind, usage, label });
  if (!q) return all.map(strip);
  const scored: [number, SearchResult][] = [];
  for (const r of all) {
    const label = r.label.toLowerCase();
    let score = -1;
    if (label === q) score = 0;
    else if (label.startsWith(q)) score = 1;
    else if (r.hay.includes(q)) score = 2;
    else if (q.split(/\s+/).every((w) => r.hay.includes(w))) score = 3;
    if (score >= 0) scored.push([score, strip(r)]);
  }
  return scored.sort((a, b) => a[0] - b[0]).map(([, r]) => r);
}
