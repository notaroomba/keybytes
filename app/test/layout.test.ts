import test from 'node:test';
import assert from 'node:assert';
import { layout, bounds, TILE, GAP } from '../src/renderer/src/lib/layout.ts';

const STEP = TILE + GAP;
const at = (items: ReturnType<typeof layout>, id: string) => items.find((i) => i.id === id);
const cell = (items: ReturnType<typeof layout>, id: string) => {
  const p = at(items, id)!;
  return [p.x / STEP, p.y / STEP];
};

test('empty board: the Keyword (one cell) and three dashed slots', () => {
  const items = layout({}, {});
  assert.deepStrictEqual(items.map((i) => i.kind), ['keyword', 'slot', 'slot', 'slot']);
  assert.deepStrictEqual(bounds(items), { x: -STEP, y: -STEP, w: 2 * STEP + TILE, h: STEP + TILE });
});

test('chained Keybits go straight out from the one they came through', () => {
  const items = layout({
    TOP: { at: 'T', via: null, depth: null },
    A: { at: null, via: 'TOP', depth: 2 },
    B: { at: null, via: 'TOP', depth: 3 },
  }, {});
  assert.deepStrictEqual(cell(items, 'TOP'), [0, -1]);
  assert.deepStrictEqual(cell(items, 'A'), [0, -2]);
  assert.deepStrictEqual(cell(items, 'B'), [0, -3]);
  assert.ok(at(items, 'A')!.estimated && !at(items, 'TOP')!.estimated);
});

test('two Keybits both reported on the left: the second goes on top, and a chained one closes the square', () => {
  const items = layout({
    K1: { at: 'L', via: null, depth: null },
    K2: { at: 'L', via: null, depth: null },
    C: { at: null, via: 'K2', depth: 2 },
  }, {});
  assert.deepStrictEqual(cell(items, 'K1'), [-1, 0]);
  assert.deepStrictEqual(cell(items, 'K2'), [0, -1]);
  assert.deepStrictEqual(cell(items, 'C'), [-1, -1]);   // touches both, not straight up
});

test('the side the user picked wins', () => {
  const items = layout({ K1: { at: 'L', via: null, depth: null }, K2: { at: 'L', via: null, depth: null } }, {}, { K2: 'L' });
  assert.deepStrictEqual(cell(items, 'K2'), [-1, 0]);
  assert.deepStrictEqual(cell(items, 'K1'), [0, -1]);
});

test('chained Keybits never share a cell, and an unknown route still shows', () => {
  const items = layout({
    R: { at: 'R', via: null, depth: null },
    A: { at: null, via: 'R', depth: 2 },
    B: { at: null, via: 'R', depth: 2 },
    X: { at: null, via: 'GONE', depth: 2 },
  }, {});
  const cells = items.map((i) => `${i.x},${i.y}`);
  assert.strictEqual(new Set(cells).size, cells.length);
  assert.ok(at(items, 'X'));
});
