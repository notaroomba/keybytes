const test = require('node:test');
const assert = require('node:assert');
const P = require('../renderer/profiles.js');

test('export then import round-trips', () => {
  const p = P.newProfile('Gaming', { '5f1821c1': { mods: 2, key: 4, name: 'Jump' } });
  const back = P.importProfile(P.exportProfile(p));
  assert.strictEqual(back.name, 'Gaming');
  assert.deepStrictEqual(back.keys, { '5F1821C1': { mods: 2, key: 4, name: 'Jump' } });
  assert.notStrictEqual(back.id, p.id);
});

test('import rejects other files', () => {
  assert.throws(() => P.importProfile('nope'), /JSON/);
  assert.throws(() => P.importProfile('{"a":1}'), /Not a Keyword profile/);
  assert.throws(() => P.importProfile('{"format":"keyword-profile","version":9}'), /version/);
});

test('import drops bad keys', () => {
  const p = P.importProfile(JSON.stringify({ format: 'keyword-profile', version: 1, keys: { xyz: { key: 4 }, '0000000A': { key: 999, mods: 'x' } } }));
  assert.deepStrictEqual(p.keys, { '0000000A': { mods: 0, key: 999 & 0xff } });
  assert.strictEqual(p.name, 'Imported');
});

test('normalizeStore fills in a default', () => {
  const s = P.normalizeStore(null);
  assert.strictEqual(s.profiles.length, 1);
  assert.strictEqual(s.active, s.profiles[0].id);
  const s2 = P.normalizeStore({ active: 'zz', profiles: [{ id: 'a', name: 'A', keys: {} }] });
  assert.strictEqual(s2.active, 'a');
});
