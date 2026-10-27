import test from 'node:test';
import assert from 'node:assert';
import * as Hid from '../src/renderer/src/lib/hid.ts';

test('usages match the HID table', () => {
  const u = (code: string) => Hid.KEYS.find((k) => k.code === code)!.usage;
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

test('media keys', () => {
  assert.deepStrictEqual(Hid.fromEvent({ code: 'MediaPlayPause' }), { consumer: 0xcd });
  assert.strictEqual(Hid.consumerLabel(0xe9), 'Volume Up');
});

test('text to macro steps', () => {
  assert.deepStrictEqual(Hid.textToSteps('aB1!').steps, [[0, 4], [2, 5], [0, 0x1e], [2, 0x1e]]);
  assert.deepStrictEqual(Hid.textToSteps('é').skipped, ['é']);
  assert.strictEqual(Hid.macroLabel([...Hid.textToSteps('Hi').steps, [8, 0x06]], true), '"Hi", Cmd+C');
});

test('search finds keys and media keys', () => {
  assert.strictEqual(Hid.search('f13')[0].label, 'F13');
  assert.deepStrictEqual(Hid.search('play')[0], { kind: 'c', usage: 0xcd, label: 'Play/Pause' });
  assert.strictEqual(Hid.search('zzzz').length, 0);
});
