import test from 'node:test';
import assert from 'node:assert';
import { layout, bounds, TILE, GAP, KEYWORD_W } from '../src/renderer/src/lib/layout.ts';

const STEP = TILE + GAP;
const at = (items: ReturnType<typeof layout>, id: string) => items.find((i) => i.id === id);

test('empty board: the Keyword and three dashed slots', () => {
  const items = layout({}, {});
  assert.deepStrictEqual(items.map((i) => i.kind), ['keyword', 'slot', 'slot', 'slot']);
  assert.deepStrictEqual(bounds(items), { x: -STEP, y: -STEP, w: KEYWORD_W + 2 * STEP, h: TILE + STEP });
});

test('chained Keybits go straight out from the one they came through', () => {
  const items = layout({
    TOP: { at: 'T', via: null, depth: null },
    A: { at: null, via: 'TOP', depth: 2 },
    B: { at: null, via: 'TOP', depth: 3 },
    C: { at: null, via: 'TOP', depth: 2 },
  }, {});
  const top = at(items, 'TOP')!;
  assert.deepStrictEqual([at(items, 'A')!.x, at(items, 'A')!.y], [top.x, top.y - STEP]);
  assert.deepStrictEqual([at(items, 'B')!.x, at(items, 'B')!.y], [top.x, top.y - 2 * STEP]);
  assert.deepStrictEqual([at(items, 'C')!.x, at(items, 'C')!.y], [top.x + STEP, top.y - STEP]);  // same hop: spread across
  assert.ok(at(items, 'A')!.estimated && !top.estimated);
});

test('a chained Keybit with an unknown route still shows', () => {
  const items = layout({ X: { at: null, via: 'GONE', depth: 2 } }, {});
  assert.ok(at(items, 'X'));
});
