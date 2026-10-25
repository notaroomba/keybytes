const test = require('node:test');
const assert = require('node:assert');
const { LineBuffer, parseLine, cmd, profileCommands } = require('../renderer/protocol.js');

test('parses SIDES', () => {
  assert.deepStrictEqual(parseLine('SIDES L=1 R=0 T=0'), {
    type: 'sides',
    sides: { L: true, R: false, T: false },
  });
});

test('SIDES with only some fields', () => {
  assert.deepStrictEqual(parseLine('SIDES R=1').sides, { R: true });
  assert.strictEqual(parseLine('SIDES').type, 'unknown');
});

test('parses HELLO reply', () => {
  assert.deepStrictEqual(parseLine('KEYWORD v=0.1 proto=1'), { type: 'hello', fw: '0.1', proto: 1 });
});

test('accepts 96-bit UIDs', () => {
  const uid = '3B0A4F1C88D2E61F5F1821C1';
  assert.deepStrictEqual(parseLine(`KEY+ uid=${uid.toLowerCase()} at=L`), { type: 'keyAdd', uid, at: 'L' });
  assert.strictEqual(cmd.map(uid, 1, 4), `MAP uid=${uid} mods=01 key=04`);
  assert.strictEqual(parseLine('KEY+ uid=3B0A4F1C88D2 at=L').type, 'unknown');
});

test('parses ERR', () => {
  assert.deepStrictEqual(parseLine('ERR i2c timeout'), { type: 'error', message: 'i2c timeout' });
});

test('line buffer splits chunks and CRLF', () => {
  const lines = [];
  const lb = new LineBuffer((l) => lines.push(l));
  lb.push('SIDES L=1 ');
  lb.push('R=0\r\nKEYW');
  lb.push('ORD v=1 proto=1\n\n');
  assert.deepStrictEqual(lines, ['SIDES L=1 R=0', 'KEYWORD v=1 proto=1']);
});

test('parses key events', () => {
  assert.deepStrictEqual(parseLine('KEY+ uid=5f1821c1 at=R'), { type: 'keyAdd', uid: '5F1821C1', at: 'R' });
  assert.deepStrictEqual(parseLine('KEY+ uid=5F1821C1 at=?'), { type: 'keyAdd', uid: '5F1821C1', at: null });
  assert.deepStrictEqual(parseLine('KEY- uid=5F1821C1'), { type: 'keyRemove', uid: '5F1821C1' });
  assert.deepStrictEqual(parseLine('PRESS uid=5F1821C1 down=1'), { type: 'press', uid: '5F1821C1', down: true });
  assert.deepStrictEqual(parseLine('KEYS END'), { type: 'keysEnd' });
  assert.strictEqual(parseLine('KEY+ uid=xyz').type, 'unknown');
});

test('parses MAP and OK', () => {
  assert.deepStrictEqual(parseLine('MAP uid=5F1821C1 mods=0A key=04'), { type: 'map', uid: '5F1821C1', mods: 10, key: 4 });
  assert.deepStrictEqual(parseLine('OK MAP uid=5F1821C1'), { type: 'ok', what: 'MAP', uid: '5F1821C1' });
});

test('formats commands', () => {
  assert.strictEqual(cmd.map('5F1821C1', 0x0a, 0x04), 'MAP uid=5F1821C1 mods=0A key=04');
  assert.deepStrictEqual(profileCommands({ '5F1821C1': { mods: 1, key: 6 }, '00000001': { mods: 0, key: 0 }, bad: { key: 4 } }),
    ['CLEAR', 'MAP uid=5F1821C1 mods=01 key=06', 'SAVE']);
});

test('media key mappings', () => {
  const { mapCommand } = require('../renderer/protocol.js');
  assert.strictEqual(mapCommand('5F1821C1', { consumer: 0xcd }), 'MAPC uid=5F1821C1 usage=00CD');
  assert.strictEqual(mapCommand('5F1821C1', { mods: 0, key: 0 }), null);
  assert.deepStrictEqual(parseLine('MAPC uid=5F1821C1 usage=00E9'), { type: 'mapConsumer', uid: '5F1821C1', usage: 0xe9 });
  assert.deepStrictEqual(profileCommands({ '5F1821C1': { consumer: 0x192 } }), ['CLEAR', 'MAPC uid=5F1821C1 usage=0192', 'SAVE']);
});
