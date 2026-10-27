import test from 'node:test';
import assert from 'node:assert';
import * as P from '../src/renderer/src/lib/profiles.ts';

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

test('consumer entries survive a round trip', () => {
  const p = P.importProfile(P.exportProfile(P.newProfile('M', { '5F1821C1': { consumer: 0xcd, mods: 3 } })));
  assert.deepStrictEqual(p.keys, { '5F1821C1': { mods: 0, key: 0, consumer: 0xcd } });
});

test('entries keep macro, hold and host actions', () => {
  const keys = P.cleanKeys({
    '5F1821C1': { macro: [[1, 6], [1, 0x19, 9], ['x', 4]], hold: { mods: 2, ms: 5 } },
    '0A33B7E2': { host: { type: 'url', value: 'https://a.b' }, name: '  Docs ' },
    '00000001': { host: { type: 'evil', value: 'x' }, key: 4 },
  });
  assert.deepStrictEqual(keys['5F1821C1'], { macro: [[1, 6], [0, 4]], hold: { mods: 2, key: 0, ms: 50 } });
  assert.deepStrictEqual(keys['0A33B7E2'], { host: { type: 'url', value: 'https://a.b' }, name: 'Docs' });
  assert.deepStrictEqual(keys['00000001'], { mods: 0, key: 4 });
  assert.strictEqual(P.isMapped({ mods: 0, key: 0 }), false);
  assert.strictEqual(P.isMapped(keys['0A33B7E2']), true);
});

test('library export imports as several profiles', () => {
  const a = P.newProfile('A', { '5F1821C1': { key: 4 } }, ['Figma']);
  const b = P.newProfile('B');
  const out = P.importProfiles(P.exportLibrary([a, b]));
  assert.deepStrictEqual(out.map((p) => [p.name, p.apps]), [['A', ['Figma']], ['B', []]]);
  assert.throws(() => P.importProfiles('{"format":"keyword-profile-library","version":1}'), /no profiles/);
});

test('settings are kept and repaired', () => {
  const s = P.normalizeStore({ settings: { theme: 'neon', perApp: true, autoReconnect: 'yes', lastConnection: { kind: 'ble', id: 'x' } }, stats: { A: 3, B: -1, C: 'x' } });
  assert.strictEqual(s.settings.theme, 'system');
  assert.strictEqual(s.settings.perApp, true);
  assert.strictEqual(s.settings.autoReconnect, true);
  assert.deepStrictEqual(s.settings.lastConnection, { kind: 'ble', id: 'x' });
  assert.deepStrictEqual(s.stats, { A: 3 });
});

test('per-app lookup is case-insensitive', () => {
  const ps = [P.newProfile('Zoom', {}, ['zoom.us']), P.newProfile('Code', {}, ['Code'])];
  assert.strictEqual(P.profileForApp(ps, 'ZOOM.US')?.name, 'Zoom');
  assert.strictEqual(P.profileForApp(ps, 'Safari'), null);
});

test('history undoes and redoes', () => {
  const h = new P.History<{ n: number }>(2);
  let state = { n: 1 };
  h.record(state, 'one'); state = { n: 2 };
  h.record(state, 'two'); state = { n: 3 };
  h.record(state, 'three'); state = { n: 4 }; // oldest entry drops (limit 2)
  let r = h.undo(state)!; assert.deepStrictEqual([r.state, r.label], [{ n: 3 }, 'three']); state = r.state;
  r = h.undo(state)!; assert.deepStrictEqual(r.state, { n: 2 }); state = r.state;
  assert.strictEqual(h.undo(state), null);
  r = h.redo(state)!; assert.deepStrictEqual(r.state, { n: 3 }); state = r.state;
  h.record(state, 'new');
  assert.strictEqual(h.canRedo, false);
});
