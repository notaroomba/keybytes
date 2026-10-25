const test = require('node:test');
const assert = require('node:assert');
const T = require('../renderer/templates.js');
const P = require('../renderer/profiles.js');

test('every template builds valid entries on both platforms', () => {
  for (const t of T.TEMPLATES) {
    for (const mac of [true, false]) {
      const actions = t.build(mac);
      assert.ok(actions.length > 0, t.id);
      for (const a of actions) assert.ok(P.isMapped(P.cleanEntry(a)), `${t.id} ${JSON.stringify(a)}`);
    }
  }
});

test('templates fill keys left, top, right', () => {
  const side = { A: 'R', B: 'L', C: 'T' };
  const uids = T.orderUids(['A', 'B', 'C'], (u) => side[u]);
  assert.deepStrictEqual(uids, ['B', 'C', 'A']);
  const media = T.TEMPLATES.find((t) => t.id === 'media');
  const r = T.applyTemplate(media, uids, true);
  assert.strictEqual(r.used, 3);
  assert.strictEqual(r.keys.B.consumer, 0xcd);
  assert.strictEqual(r.total, 6);
});
