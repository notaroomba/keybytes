// Where everything goes on the board view: the Keyword, its three side
// slots, and every Keybit, touching it or chained behind one that is. It all
// sits on a grid of Keybit-sized cells, the Keyword being one cell too.
import type { Side } from './types.ts';

/** Size of a cell and the gap between cells, in unscaled px. */
export const TILE = 96;
export const GAP = 8;
const STEP = TILE + GAP;

/** The other side a Keybit reported on side s could be on, if any. */
export const otherSide = (s: Side): Side | undefined => OTHER[s];

/** One thing to draw, with its top-left corner relative to the Keyword's. */
export interface Placed {
  id: string;
  kind: 'keyword' | 'slot' | 'keybit';
  x: number;
  y: number;
  w: number;
  h: number;
  side?: Side;
  uid?: string;
  /** A slot the Keyword reports something on, before its Keybit is known. */
  present?: boolean;
  /** Placed by a guess (chained Keybits until the firmware says where they are). */
  estimated?: boolean;
  /** Its turn in clockwise quarter turns, when known. */
  rot?: number;
}

/** A Keybit's own sides. */
export type Compass = 'N' | 'E' | 'S' | 'W';

/** What layout() needs to know about a Keybit. */
export interface LayoutKey {
  at: Side | null;
  via: string | null;
  depth: number | null;
  /** v11: its own side facing the Keyword or the Keybit it hangs off. */
  face?: Compass | null;
  /** v11: the Keybit it hangs off (null for one on the Keyword), and that one's side it's on. */
  parent?: string | null;
  pside?: Compass | null;
}

type Cell = [number, number];
const SLOT: Record<Side, Cell> = { L: [-1, 0], T: [0, -1], R: [1, 0] };
/** Straight out from the Keyword for a Keybit on each side. */
const OUT: Record<Side, Cell> = { L: [-1, 0], T: [0, -1], R: [1, 0] };
/**
 * Where else a Keybit reported on a side could be. A Keybit turned 90
 * degrees shows the Keyword the same side of itself on the top as one the
 * usual way round shows it on the left, so the shared bus can't tell those
 * two apart. Right is never mixed up.
 */
const OTHER: Partial<Record<Side, Side>> = { L: 'T', T: 'L' };
const NEXT: Cell[] = [[0, -1], [-1, 0], [1, 0], [0, 1]];
/** A Keybit's own sides as steps, the usual way up. */
const DIR: Record<Compass, Cell> = { N: [0, -1], E: [1, 0], S: [0, 1], W: [-1, 0] };

const key = ([x, y]: Cell) => `${x},${y}`;
/** v turned clockwise by r quarter turns (screen y points down). */
const turn = ([x, y]: Cell, r: number): Cell => (r % 4 === 0 ? [x, y] : turn([-y, x], r - 1));
/** The turn that points a Keybit's side f the way of d, or null. */
const turnFor = (f: Compass, d: Cell): number | null => {
  for (let r = 0; r < 4; r++) { const [x, y] = turn(DIR[f], r); if (x === d[0] && y === d[1]) return r; }
  return null;
};

/** One way to place everything, and how many Keybits it couldn't place exactly. */
function place(
  keys: Record<string, LayoutKey>,
  direct: [string, LayoutKey][],
  bySide: Partial<Record<Side, string>>,
) {
  const cells = new Map<string, string>([[key([0, 0]), 'keyword']]);
  const at: Record<string, Cell> = {};
  const rot: Record<string, number> = {};
  const sideOf: Record<string, Side> = {};
  for (const [s, uid] of Object.entries(bySide) as [Side, string][]) {
    sideOf[uid] = s;
    at[uid] = SLOT[s];
    const f = keys[uid].face;
    const r = f ? turnFor(f, [-OUT[s][0], -OUT[s][1]]) : null;
    if (r !== null) rot[uid] = r;
  }
  // The Keyword's own side slots are only for Keybits touching it, and its
  // bottom has the USB port.
  for (const s of ['L', 'T', 'R'] as Side[]) cells.set(key(SLOT[s]), bySide[s] ?? `slot-${s}`);
  cells.set(key([0, 1]), 'usb');

  const chained = Object.entries(keys)
    .filter(([, k]) => !k.at)
    .sort(([a, ka], [b, kb]) => (ka.depth ?? 99) - (kb.depth ?? 99) || a.localeCompare(b));
  const exact = new Set<string>();
  let misses = 0;

  // First what the Keybits report: the side of its parent each one is on,
  // and its own side facing that parent, which gives its turn as well.
  for (const [uid, k] of chained) {
    const p = k.parent;
    if (!p || !k.pside || !k.face || !at[p] || rot[p] === undefined) continue;
    const d = turn(DIR[k.pside], rot[p]);
    const c: Cell = [at[p][0] + d[0], at[p][1] + d[1]];
    const r = turnFor(k.face, [-d[0], -d[1]]);
    if (cells.has(key(c)) || r === null) { misses++; continue; }
    at[uid] = c;
    rot[uid] = r;
    cells.set(key(c), uid);
    exact.add(uid);
  }

  // Then a guess for the rest: a free cell next to the Keybit one hop in,
  // the one touching the most Keybits (so a block stays a block), then
  // straight out. One whose route is unknown goes in a row under the Keyword.
  const hop: Record<string, Cell> = {};   // `${root uid}:${depth}` -> a cell at that hop
  for (const [uid, k] of chained) if (exact.has(uid) && k.via && k.depth) hop[`${k.via}:${k.depth}`] ??= at[uid];
  const loose: string[] = [];
  for (const [uid, k] of chained) {
    if (exact.has(uid)) continue;
    const rootSide = k.via ? sideOf[k.via] : undefined;
    if (!rootSide || !k.depth) { loose.push(uid); continue; }
    const from = (k.parent && at[k.parent]) || hop[`${k.via}:${k.depth - 1}`] || at[k.via!];
    const out = OUT[rootSide];
    const free = NEXT.map(([dx, dy]): Cell => [from[0] + dx, from[1] + dy]).filter((c) => !cells.has(key(c)));
    if (!free.length) { loose.push(uid); continue; }
    const touching = (c: Cell) => NEXT.filter(([dx, dy]) => {
      const who = cells.get(key([c[0] + dx, c[1] + dy]));
      return who && who !== 'usb' && !who.startsWith('slot-');
    }).length;
    const straight = (c: Cell) => c[0] - from[0] === out[0] && c[1] - from[1] === out[1];
    free.sort((a, b) => touching(b) - touching(a) || Number(straight(b)) - Number(straight(a)));
    at[uid] = free[0];
    cells.set(key(free[0]), uid);
    hop[`${k.via}:${k.depth}`] ??= free[0];
  }
  return { at, rot, exact, loose, chained, misses };
}

/**
 * Lays out the board. A Keybit touching the Keyword goes in its side's slot;
 * when two report the same side, the second goes on the side it could also
 * be on, and if the Keybits chained to them only fit one way round, that way.
 * A chained Keybit on v11 says which side of which Keybit it's on and which
 * way it's turned, so it goes exactly there. Older ones are estimated (dashed).
 */
export function layout(
  rawKeys: Record<string, LayoutKey>,
  sides: Partial<Record<Side, boolean>>,
  /** The side the user put an L/T Keybit on, by UID, oldest choice first. */
  prefer: Record<string, Side> = {},
): Placed[] {
  const keys = resolveShort(rawKeys);
  const direct = Object.entries(keys).filter(([, k]) => k.at).sort(([a], [b]) => a.localeCompare(b));
  // The user's choices first, the latest first of all, so it wins its side
  // over an older one that wants the same.
  const order = Object.keys(prefer);
  direct.sort(([a], [b]) => order.indexOf(b) - order.indexOf(a));

  // Every way the Keybits that could be on the left or the top can be
  // shared out (at most 2^3), the user's way first; keep the first one
  // the chained Keybits fit around exactly.
  const assign = (flips: number) => {
    const bySide: Partial<Record<Side, string>> = {};
    let bit = 0;
    for (const [uid, k] of direct) {
      const options = [k.at!, OTHER[k.at!]].filter(Boolean) as Side[];
      if (options.includes(prefer[uid])) options.unshift(prefer[uid]);
      if (options.length > 1 && (flips >> bit++) & 1) options.reverse();
      const s = options.find((o) => !bySide[o]);
      if (s) bySide[s] = uid;   // no side left: it still shows in the key list
    }
    return bySide;
  };
  const ambiguous = direct.filter(([, k]) => OTHER[k.at!]).length;
  let best = place(keys, direct, assign(0));
  let bySide = assign(0);
  for (let flips = 1; best.misses && flips < 1 << Math.min(ambiguous, 3); flips++) {
    const tryIt = place(keys, direct, assign(flips));
    if (tryIt.misses < best.misses) { best = tryIt; bySide = assign(flips); }
  }
  const { at, rot, exact, loose, chained } = best;

  const px = ([x, y]: Cell) => ({ x: x * STEP, y: y * STEP, w: TILE, h: TILE });
  const out: Placed[] = [{ id: 'keyword', kind: 'keyword', ...px([0, 0]) }];
  for (const s of ['L', 'T', 'R'] as Side[]) {
    const uid = bySide[s];
    out.push(uid ? { id: uid, kind: 'keybit', side: s, uid, rot: rot[uid], ...px(SLOT[s]) }
                 // Something there the Keyword hasn't named yet; not a Keybit that's just drawn elsewhere.
                 : { id: `slot-${s}`, kind: 'slot', side: s, present: !!sides[s] && !direct.some(([, k]) => k.at === s), ...px(SLOT[s]) });
  }
  for (const [uid] of chained) {
    if (at[uid] && !loose.includes(uid)) out.push({ id: uid, kind: 'keybit', uid, rot: rot[uid], estimated: !exact.has(uid), ...px(at[uid]) });
  }
  loose.forEach((uid, i) => out.push({ id: uid, kind: 'keybit', uid, estimated: true, ...px([i - 1, 2]) }));
  return out;
}

/**
 * A chained Keybit is reported with the low 32 bits (8 hex digits) of the
 * Keybit it came through and hangs off. If that one is known by its full
 * UID, point at it, or the chained one has nowhere to go.
 */
export function resolveShort(keys: Record<string, LayoutKey>): Record<string, LayoutKey> {
  const ids = Object.keys(keys);
  const full = (u: string | null | undefined) => {
    if (!u || keys[u]) return u;
    const m = ids.filter((id) => id.length > u.length && id.endsWith(u));
    return m.length === 1 ? m[0] : u;
  };
  const out: Record<string, LayoutKey> = {};
  for (const [id, k] of Object.entries(keys)) out[id] = { ...k, via: full(k.via) ?? null, parent: full(k.parent) ?? k.parent };
  return out;
}

/** The smallest box around everything placed. */
export function bounds(items: Placed[]) {
  const x = Math.min(...items.map((i) => i.x));
  const y = Math.min(...items.map((i) => i.y));
  return { x, y, w: Math.max(...items.map((i) => i.x + i.w)) - x, h: Math.max(...items.map((i) => i.y + i.h)) - y };
}
