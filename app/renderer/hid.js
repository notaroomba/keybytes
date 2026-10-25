// USB HID keyboard usages (HID Usage Tables, page 0x07) keyed by KeyboardEvent.code.
(function (root) {
  const KEYS = [];
  const add = (code, usage, label) => KEYS.push({ code, usage, label });

  for (let i = 0; i < 26; i++) {
    const c = String.fromCharCode(65 + i);
    add(`Key${c}`, 0x04 + i, c);
  }
  '1234567890'.split('').forEach((d, i) => add(`Digit${d}`, 0x1e + i, d));
  [
    ['Enter', 0x28, 'Enter'], ['Escape', 0x29, 'Esc'], ['Backspace', 0x2a, 'Backspace'],
    ['Tab', 0x2b, 'Tab'], ['Space', 0x2c, 'Space'], ['Minus', 0x2d, '-'], ['Equal', 0x2e, '='],
    ['BracketLeft', 0x2f, '['], ['BracketRight', 0x30, ']'], ['Backslash', 0x31, '\\'],
    ['Semicolon', 0x33, ';'], ['Quote', 0x34, "'"], ['Backquote', 0x35, '`'], ['Comma', 0x36, ','],
    ['Period', 0x37, '.'], ['Slash', 0x38, '/'], ['CapsLock', 0x39, 'Caps Lock'],
  ].forEach((k) => add(...k));
  for (let i = 1; i <= 12; i++) add(`F${i}`, 0x39 + i, `F${i}`);
  [
    ['PrintScreen', 0x46, 'Print Screen'], ['ScrollLock', 0x47, 'Scroll Lock'], ['Pause', 0x48, 'Pause'],
    ['Insert', 0x49, 'Insert'], ['Home', 0x4a, 'Home'], ['PageUp', 0x4b, 'Page Up'],
    ['Delete', 0x4c, 'Delete'], ['End', 0x4d, 'End'], ['PageDown', 0x4e, 'Page Down'],
    ['ArrowRight', 0x4f, '→'], ['ArrowLeft', 0x50, '←'], ['ArrowDown', 0x51, '↓'], ['ArrowUp', 0x52, '↑'],
    ['NumLock', 0x53, 'Num Lock'], ['NumpadDivide', 0x54, 'Num /'], ['NumpadMultiply', 0x55, 'Num *'],
    ['NumpadSubtract', 0x56, 'Num -'], ['NumpadAdd', 0x57, 'Num +'], ['NumpadEnter', 0x58, 'Num Enter'],
  ].forEach((k) => add(...k));
  for (let i = 1; i <= 9; i++) add(`Numpad${i}`, 0x58 + i, `Num ${i}`);
  [
    ['Numpad0', 0x62, 'Num 0'], ['NumpadDecimal', 0x63, 'Num .'], ['IntlBackslash', 0x64, 'Intl \\'],
    ['ContextMenu', 0x65, 'Menu'], ['NumpadEqual', 0x67, 'Num ='],
  ].forEach((k) => add(...k));
  for (let i = 13; i <= 24; i++) add(`F${i}`, 0x5b + i, `F${i}`);
  [
    ['AudioVolumeMute', 0x7f, 'Mute'], ['AudioVolumeUp', 0x80, 'Volume Up'],
    ['AudioVolumeDown', 0x81, 'Volume Down'],
  ].forEach((k) => add(...k));

  const byCode = new Map(KEYS.map((k) => [k.code, k]));
  const byUsage = new Map(KEYS.map((k) => [k.usage, k]));

  // Modifier byte bits, as in the HID boot keyboard report.
  const MODS = [
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

  function modName(m, mac) {
    if (!mac) return m.name.replace('GUI', 'Win');
    return m.name.replace('GUI', 'Cmd').replace('Alt', 'Option');
  }

  // "Ctrl+Shift+A", "Cmd+Space", "Shift" (modifier only), "" when empty.
  function comboLabel(mods, usage, mac) {
    const parts = MODS.filter((m) => mods & (1 << m.bit)).map((m) => modName(m, mac));
    if (usage) parts.push(byUsage.get(usage)?.label || `0x${usage.toString(16).padStart(2, '0')}`);
    return parts.join('+');
  }

  // Turns a key event ({code, ctrlKey, shiftKey, altKey, metaKey}) into {mods, usage}.
  // usage is 0 when only modifiers are held. Returns null for keys HID can't send.
  function fromEvent(e) {
    let mods = (e.ctrlKey ? 1 : 0) | (e.shiftKey ? 2 : 0) | (e.altKey ? 4 : 0) | (e.metaKey ? 8 : 0);
    const mod = modByCode.get(e.code);
    if (mod) return { mods: mods | (1 << mod.bit), usage: 0 };
    const k = byCode.get(e.code);
    if (!k) return null;
    return { mods, usage: k.usage };
  }

  const api = { KEYS, MODS, byUsage, comboLabel, fromEvent, modName };
  if (typeof module !== 'undefined' && module.exports) module.exports = api;
  else root.Hid = api;
})(typeof window !== 'undefined' ? window : globalThis);
