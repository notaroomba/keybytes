const test = require('node:test');
const assert = require('node:assert');
const Hid = require('../renderer/hid.js');

test('usages match the HID table', () => {
  const u = (code) => Hid.KEYS.find((k) => k.code === code).usage;
  assert.strictEqual(u('KeyA'), 0x04);
  assert.strictEqual(u('KeyZ'), 0x1d);
  assert.strictEqual(u('Digit1'), 0x1e);
  assert.strictEqual(u('Digit0'), 0x27);
  assert.strictEqual(u('F1'), 0x3a);
  assert.strictEqual(u('F12'), 0x45);
  assert.strictEqual(u('Numpad1'), 0x59);
  assert.strictEqual(u('Numpad9'), 0x61);
  assert.strictEqual(u('F13'), 0x68);
  assert.strictEqual(u('F24'), 0x73);
  assert.strictEqual(new Set(Hid.KEYS.map((k) => k.usage)).size, Hid.KEYS.length);
});

test('fromEvent and labels', () => {
  assert.deepStrictEqual(Hid.fromEvent({ code: 'KeyC', ctrlKey: true, shiftKey: true }), { mods: 3, usage: 6 });
  assert.deepStrictEqual(Hid.fromEvent({ code: 'ShiftRight', shiftKey: true }), { mods: 0x22, usage: 0 });
  assert.strictEqual(Hid.fromEvent({ code: 'Fn' }), null);
  assert.strictEqual(Hid.comboLabel(0x09, 0x2c, true), 'Ctrl+Cmd+Space');
  assert.strictEqual(Hid.comboLabel(0x09, 0x2c, false), 'Ctrl+Win+Space');
  assert.strictEqual(Hid.comboLabel(0x02, 0, false), 'Shift');
});
