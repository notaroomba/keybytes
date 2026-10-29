// Where everything goes on the board view: the Keyword, its three side
// slots, and every Keybit, touching it or chained behind one that is.
import type { Side } from './types.ts';

/** Size of a Keybit tile and the gap between tiles, in unscaled px. */
export const TILE = 96;
export const GAP = 8;
const STEP = TILE + GAP;
/** The Keyword itself is two tiles wide. */
export const KEYWORD_W = 2 * TILE + GAP;

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

const SLOT_AT: Record<Side, [number, number]> = {
  T: [(KEYWORD_W - TILE) / 2, -STEP],
  L: [-STEP, 0],
  R: [KEYWORD_W + GAP, 0],
};
/** Away from the Keyword, and across, for a Keybit on each side. */
const OUT: Record<Side, [number, number]> = { T: [0, -1], L: [-1, 0], R: [1, 0] };

/**
 * Lays out the board. A Keybit touching the Keyword goes in its side's slot.
 * A chained one is only known by the Keybit it was relayed through and how
 * many hops out it is, so until the firmware reports which side of its
 * parent it sits on, it's estimated: straight out from that Keybit, away
 * from the Keyword, one tile per hop, with Keybits at the same hop spread
 * across. One whose route is unknown goes in a row under the Keyword.
 */
export function layout(keys: Record<string, LayoutKey>, sides: Partial<Record<Side, boolean>>): Placed[] {
  const out: Placed[] = [{ id: 'keyword', kind: 'keyword', x: 0, y: 0, w: KEYWORD_W, h: TILE }];
  const bySide: Partial<Record<Side, string>> = {};
  for (const [uid, k] of Object.entries(keys)) if (k.at && !bySide[k.at]) bySide[k.at] = uid;

  for (const side of ['L', 'T', 'R'] as Side[]) {
    const [x, y] = SLOT_AT[side];
    const uid = bySide[side];
    out.push(uid ? { id: uid, kind: 'keybit', x, y, w: TILE, h: TILE, side, uid }
                 : { id: `slot-${side}`, kind: 'slot', x, y, w: TILE, h: TILE, side, present: !!sides[side] });
  }

  const chained = Object.entries(keys)
    .filter(([, k]) => !k.at)
    .sort(([a, ka], [b, kb]) => (ka.depth ?? 99) - (kb.depth ?? 99) || a.localeCompare(b));
  const used = new Map<string, number>();
  let loose = 0;
  for (const [uid, k] of chained) {
    const root = k.via ? keys[k.via] : undefined;
    if (!root?.at || !k.depth) {
      out.push({ id: uid, kind: 'keybit', uid, estimated: true, w: TILE, h: TILE, x: loose++ * STEP, y: STEP + GAP });
      continue;
    }
    const [ox, oy] = OUT[root.at];
    const [bx, by] = SLOT_AT[root.at];
    const hop = k.depth - 1;
    // Same parent and hop: 0, +1, -1, +2, ... tiles across.
    const key = `${k.via}:${hop}`;
    const n = used.get(key) ?? 0;
    used.set(key, n + 1);
    const across = n === 0 ? 0 : n % 2 ? (n + 1) / 2 : -n / 2;
    out.push({
      id: uid, kind: 'keybit', uid, estimated: true, w: TILE, h: TILE,
      x: bx + ox * STEP * hop + (oy ? across * STEP : 0),
      y: by + oy * STEP * hop + (ox ? across * STEP : 0),
    });
  }
  return out;
}

/** The smallest box around everything placed. */
export function bounds(items: Placed[]) {
  const x = Math.min(...items.map((i) => i.x));
  const y = Math.min(...items.map((i) => i.y));
  return { x, y, w: Math.max(...items.map((i) => i.x + i.w)) - x, h: Math.max(...items.map((i) => i.y + i.h)) - y };
}
