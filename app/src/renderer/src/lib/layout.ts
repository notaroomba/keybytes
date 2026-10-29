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
}

/** What layout() needs to know about a Keybit. */
export interface LayoutKey {
  at: Side | null;
  via: string | null;
  depth: number | null;
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

const key = ([x, y]: Cell) => `${x},${y}`;

/**
 * Lays out the board. A Keybit touching the Keyword goes in its side's slot;
 * when two report the same side, the second goes on the side it could also
 * be on. A chained one is only known by the Keybit it came through and how
 * many hops out it is, so until the firmware reports where it is, it's
 * estimated: on a free cell next to the Keybit one hop in, picking the cell
 * touching the most Keybits (so a block of Keybits stays a block), then
 * straight out. One whose route is unknown goes in a row under the Keyword.
 */
export function layout(
  keys: Record<string, LayoutKey>,
  sides: Partial<Record<Side, boolean>>,
  /** The side the user put an L/T Keybit on, by UID. */
  prefer: Record<string, Side> = {},
  /** For chained Keybits, which of the possible cells the user picked, by UID. */
  pick: Record<string, number> = {},
): Placed[] {
  const cells = new Map<string, string>([[key([0, 0]), 'keyword']]);
  const at: Record<string, Cell> = {};
  const sideOf: Record<string, Side> = {};

  const direct = Object.entries(keys).filter(([, k]) => k.at).sort(([a], [b]) => a.localeCompare(b));
  const bySide: Partial<Record<Side, string>> = {};
  // The user's choices first, so they win their side.
  direct.sort(([a], [b]) => Number(!!prefer[b]) - Number(!!prefer[a]));
  for (const [uid, k] of direct) {
    const options = [k.at!, OTHER[k.at!]].filter(Boolean) as Side[];
    if (options.includes(prefer[uid])) options.unshift(prefer[uid]);
    const s = options.find((o) => !bySide[o]) ?? null;
    if (!s) continue;  // no side left for it; it still shows in the key list
    bySide[s] = uid;
    sideOf[uid] = s;
    at[uid] = SLOT[s];
  }
  // The Keyword's own side slots are only for Keybits touching it, and its
  // bottom has the USB port.
  for (const s of ['L', 'T', 'R'] as Side[]) cells.set(key(SLOT[s]), bySide[s] ?? `slot-${s}`);
  cells.set(key([0, 1]), 'usb');

  const chained = Object.entries(keys)
    .filter(([, k]) => !k.at)
    .sort(([a, ka], [b, kb]) => (ka.depth ?? 99) - (kb.depth ?? 99) || a.localeCompare(b));
  const hop: Record<string, Cell> = {};   // `${root uid}:${depth}` -> a cell at that hop
  const loose: string[] = [];
  for (const [uid, k] of chained) {
    const rootSide = k.via ? sideOf[k.via] : undefined;
    if (!rootSide || !k.depth) { loose.push(uid); continue; }
    const from = hop[`${k.via}:${k.depth - 1}`] ?? at[k.via!];
    const out = OUT[rootSide];
    const free = NEXT.map(([dx, dy]): Cell => [from[0] + dx, from[1] + dy]).filter((c) => !cells.has(key(c)));
    if (!free.length) { loose.push(uid); continue; }
    const touching = (c: Cell) => NEXT.filter(([dx, dy]) => {
      const who = cells.get(key([c[0] + dx, c[1] + dy]));
      return who && who !== 'usb' && !who.startsWith('slot-');
    }).length;
    const straight = (c: Cell) => c[0] - from[0] === out[0] && c[1] - from[1] === out[1];
    free.sort((a, b) => touching(b) - touching(a) || Number(straight(b)) - Number(straight(a)));
    const c = free[(pick[uid] ?? 0) % free.length];
    at[uid] = c;
    cells.set(key(c), uid);
    hop[`${k.via}:${k.depth}`] ??= c;
  }

  const px = ([x, y]: Cell) => ({ x: x * STEP, y: y * STEP, w: TILE, h: TILE });
  const out: Placed[] = [{ id: 'keyword', kind: 'keyword', ...px([0, 0]) }];
  for (const s of ['L', 'T', 'R'] as Side[]) {
    const uid = bySide[s];
    out.push(uid ? { id: uid, kind: 'keybit', side: s, uid, ...px(SLOT[s]) }
                 // Something there the Keyword hasn't named yet; not a Keybit that's just drawn elsewhere.
                 : { id: `slot-${s}`, kind: 'slot', side: s, present: !!sides[s] && !direct.some(([, k]) => k.at === s), ...px(SLOT[s]) });
  }
  for (const [uid] of chained) if (at[uid]) out.push({ id: uid, kind: 'keybit', uid, estimated: true, ...px(at[uid]) });
  loose.forEach((uid, i) => out.push({ id: uid, kind: 'keybit', uid, estimated: true, ...px([i - 1, 2]) }));
  return out;
}

/** The smallest box around everything placed. */
export function bounds(items: Placed[]) {
  const x = Math.min(...items.map((i) => i.x));
  const y = Math.min(...items.map((i) => i.y));
  return { x, y, w: Math.max(...items.map((i) => i.x + i.w)) - x, h: Math.max(...items.map((i) => i.y + i.h)) - y };
}
