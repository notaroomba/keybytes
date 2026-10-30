import test from 'node:test';
import assert from 'node:assert';
import { LineBuffer, parseLine, cmd, profileCommands, mapCommand, entryCommands, OTA, otaBaseCommand, checkAppImage, otaChunks } from '../src/renderer/src/lib/protocol.ts';
import type { Step } from '../src/renderer/src/lib/types.ts';

test('parses SIDES', () => {
  assert.deepStrictEqual(parseLine('SIDES L=1 R=0 T=0'), {
    type: 'sides',
    sides: { L: true, R: false, T: false },
  });
});

test('SIDES with only some fields', () => {
  assert.deepStrictEqual((parseLine('SIDES R=1') as { sides: object }).sides, { R: true });
  assert.strictEqual(parseLine('SIDES').type, 'unknown');
});

test('parses HELLO reply', () => {
  assert.deepStrictEqual(parseLine('KEYWORD v=0.1 proto=1'), { type: 'hello', fw: '0.1', proto: 1, caps: new Set(), kb: null, macroSteps: 16 });
  const h = parseLine('KEYWORD v=0.2 proto=1 caps=batt,hold kb=3');
  assert.ok(h.type === 'hello');
  assert.deepStrictEqual([...h.caps], ['batt', 'hold']);
  assert.strictEqual(h.kb, 3);
  assert.strictEqual((parseLine('KEYWORD v=0.1.0 proto=1 caps=macro kb=13 macro=64') as { macroSteps: number }).macroSteps, 64);
});

test('accepts 96-bit UIDs', () => {
  const uid = '3B0A4F1C88D2E61F5F1821C1';
  assert.deepStrictEqual(parseLine(`KEY+ uid=${uid.toLowerCase()} at=L`), { type: 'keyAdd', uid, at: 'L', v: null });
  assert.strictEqual(cmd.map(uid, 1, 4), `MAP uid=${uid} mods=01 key=04`);
  assert.strictEqual(parseLine('KEY+ uid=3B0A4F1C88D2 at=L').type, 'unknown');
});

test('parses BACKWARDS', () => {
  assert.deepStrictEqual(parseLine('BACKWARDS uid=5F1821C1 sides=NE'), { type: 'backwards', uid: '5F1821C1', sides: 'NE' });
  assert.deepStrictEqual(parseLine('BACKWARDS uid=5F1821C1 sides=?'), { type: 'backwards', uid: '5F1821C1', sides: '?' });
  assert.deepStrictEqual(parseLine('BACKWARDS uid=5F1821C1 sides=-'), { type: 'backwards', uid: '5F1821C1', sides: null });
});

test('parses ERR', () => {
  assert.deepStrictEqual(parseLine('ERR i2c timeout'), { type: 'error', message: 'i2c timeout' });
});

test('line buffer splits chunks and CRLF', () => {
  const lines: string[] = [];
  const lb = new LineBuffer((l) => lines.push(l));
  lb.push('SIDES L=1 ');
  lb.push('R=0\r\nKEYW');
  lb.push('ORD v=1 proto=1\n\n');
  assert.deepStrictEqual(lines, ['SIDES L=1 R=0', 'KEYWORD v=1 proto=1']);
});

test('parses key events', () => {
  assert.deepStrictEqual(parseLine('KEY+ uid=5f1821c1 at=R'), { type: 'keyAdd', uid: '5F1821C1', at: 'R', v: null });
  assert.deepStrictEqual(parseLine('KEY+ uid=5F1821C1 at=?'), { type: 'keyAdd', uid: '5F1821C1', at: null, v: null });
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
  assert.deepStrictEqual(profileCommands({ '5F1821C1': { mods: 1, key: 6 }, '00000001': { mods: 0, key: 0 }, bad: { key: 4 } }, new Set()),
    ['CLEAR', 'MAP uid=5F1821C1 mods=01 key=06', 'SAVE']);
});

test('media key mappings', () => {
  assert.strictEqual(mapCommand('5F1821C1', { consumer: 0xcd }), 'MAPC uid=5F1821C1 usage=00CD');
  assert.strictEqual(mapCommand('5F1821C1', { mods: 0, key: 0 }), null);
  assert.deepStrictEqual(parseLine('MAPC uid=5F1821C1 usage=00E9'), { type: 'mapConsumer', uid: '5F1821C1', usage: 0xe9 });
  assert.deepStrictEqual(profileCommands({ '5F1821C1': { consumer: 0x192 } }), ['CLEAR', 'MAPC uid=5F1821C1 usage=0192', 'SAVE']);
});

test('optional features: battery, macro, hold, flash', () => {
  assert.deepStrictEqual(parseLine('BATT pct=81 chg=1 usb=1'), { type: 'battery', pct: 81, charging: true, usb: true });
  assert.deepStrictEqual(parseLine('MACRO uid=5F1821C1 seq=0006,0119,zz'), { type: 'macro', uid: '5F1821C1', steps: [[0, 6], [1, 0x19]] });
  assert.deepStrictEqual(parseLine('HOLD uid=5F1821C1 mods=02 key=00 ms=250'), { type: 'hold', uid: '5F1821C1', mods: 2, key: 0, ms: 250 });
  assert.deepStrictEqual(parseLine('FLASH uid=5F1821C1 pct=40'), { type: 'flashProgress', uid: '5F1821C1', pct: 40 });
  assert.deepStrictEqual((parseLine('KEY+ uid=5F1821C1 at=L v=2') as { v: number }).v, 2);

  const all = new Set(['macro', 'hold']);
  const e = { macro: [[1, 6], [1, 0x19]] as Step[], hold: { mods: 2, key: 0, ms: 300 } };
  assert.deepStrictEqual(entryCommands('5F1821C1', e, all), ['MACRO uid=5F1821C1 seq=0106,0119', 'HOLD uid=5F1821C1 mods=02 key=00 ms=300']);
  assert.deepStrictEqual(entryCommands('5F1821C1', e, new Set()), [], 'unsupported features are not sent');
  assert.deepStrictEqual(entryCommands('5F1821C1', { host: { type: 'url', value: 'x' } }, all), []);
  assert.strictEqual(cmd.macro('5F1821C1', Array<Step>(20).fill([0, 4])).split(',').length, 16);
  const long = { macro: Array<Step>(70).fill([0, 4]) };
  assert.strictEqual(entryCommands('5F1821C1', long, all, 64)[0].split(',').length, 64);
  assert.strictEqual(entryCommands('5F1821C1', long, all)[0].split(',').length, 16, 'older firmware gets 16');
});

test('line buffer accepts a bare CR', () => {
  const lines: string[] = [];
  const lb = new LineBuffer((l) => lines.push(l));
  lb.push('HELLO\rSIDES?\r');
  lb.push('\nKEYS?\n');
  assert.deepStrictEqual(lines, ['HELLO', 'SIDES?', 'KEYS?']);
});

test('relayed Keybits', () => {
  assert.deepStrictEqual(parseLine('KEY+ uid=D00DF00D at=X via=91C7002DE4B3A75C0A33B7E2 depth=2 v=3'),
    { type: 'keyAdd', uid: 'D00DF00D', at: null, v: 3, via: '91C7002DE4B3A75C0A33B7E2', depth: 2 });
});

test('OTA: stack reply, image checks and chunks', () => {
  assert.deepStrictEqual(parseLine('STACK ble v=1.24.0 fus=2.2.0'), { type: 'stack', stack: 'ble', v: '1.24.0', fus: '2.2.0' });
  assert.deepStrictEqual(parseLine('STACK none fus=2.2.0'), { type: 'stack', stack: 'none', v: null, fus: '2.2.0' });
  assert.deepStrictEqual([...otaBaseCommand(OTA.START, OTA.APP_OFFSET)], [0x02, 0x00, 0x70, 0x00]);
  assert.deepStrictEqual([...otaBaseCommand(OTA.FINISH)], [0x07, 0, 0, 0]);

  const image = (sp: number, reset: number, len = 1000) => {
    const b = new Uint8Array(len);
    new DataView(b.buffer).setUint32(0, sp, true);
    new DataView(b.buffer).setUint32(4, reset, true);
    return b;
  };
  assert.ok(checkAppImage(image(0x20030000, 0x08007119)).ok);
  assert.match((checkAppImage(image(0x20030000, 0x08000119)) as { error: string }).error, /keyword_app\.bin/);
  assert.match((checkAppImage(image(0x12345678, 0x08007119)) as { error: string }).error, /vector table/);
  assert.match((checkAppImage(new Uint8Array(10)) as { error: string }).error, /too small/);

  const chunks = otaChunks(new Uint8Array(500).fill(1), 244);
  assert.deepStrictEqual(chunks.map((c) => c.length), [240, 240, 24]);
  assert.deepStrictEqual([...chunks[2].slice(18)], [1, 1, 0xff, 0xff, 0xff, 0xff]);
});

test('KEY+ with where a v11 Keybit is', () => {
  assert.deepStrictEqual(parseLine('KEY+ uid=1560ABCD at=X via=FFFFFFFF7E8BBD5D1561ABCD depth=2 parent=1561ABCD pside=N face=S'), {
    type: 'keyAdd', uid: '1560ABCD', at: null, v: null, via: 'FFFFFFFF7E8BBD5D1561ABCD', depth: 2,
    face: 'S', parent: '1561ABCD', pside: 'N',
  });
  assert.strictEqual((parseLine('KEY+ uid=FFFFFFFF7E8BBD5D1561ABCD at=L v=11 face=E') as { face: string }).face, 'E');
});
