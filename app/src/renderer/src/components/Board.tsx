import clsx from 'clsx';
import { ArrowLeftRight } from 'lucide-react';
import { useEffect, useLayoutEffect, useMemo, useRef, useState } from 'react';
import { actionLabel, activeProfile, demo, entryOf, keyName, openAssign } from '../controller.ts';
import { bounds, layout, otherSide, type Placed } from '../lib/layout.ts';
import type { Side } from '../lib/types.ts';
import { useApp } from '../store.ts';

/** How long a Keybit that left keeps showing while it pops out, in ms. */
const LEAVE_MS = 200;
/** Tallest the board gets before it scales down, in px. */
const MAX_H = 320;
const PAD = 16;
/** Where the user put Keybits that could be on the left or the top, by UID. */
const PREFER_KEY = 'board-side';

function loadPrefer(): Record<string, Side> {
  try { return JSON.parse(localStorage.getItem(PREFER_KEY) || '{}'); } catch { return {}; }
}

/**
 * The Keyword from above: a slot on each side a Keybit can attach to, and
 * every Keybit chained further out. It scales down to fit as the board
 * grows; Keybits pop in and out and the rest slide to their new places.
 */
export function Board() {
  const keys = useApp((s) => s.keys);
  const sides = useApp((s) => s.sides);
  const transport = useApp((s) => s.transport);
  useApp((s) => s.store); // labels follow the active profile
  const isDemo = transport === 'demo';

  const [prefer, setPrefer] = useState(loadPrefer);
  const flip = (uid: string, now: Side) => {
    const to = otherSide(now);
    if (!to) return;
    // Latest choice last: layout() lets it win a clash with an older one.
    const next = { ...prefer };
    delete next[uid];
    next[uid] = to;
    setPrefer(next);
    try { localStorage.setItem(PREFER_KEY, JSON.stringify(next)); } catch { /* only a convenience */ }
  };
  const placed = useMemo(() => layout(keys, sides, prefer), [keys, sides, prefer]);

  // Keep what just left on screen long enough to pop out.
  const [leaving, setLeaving] = useState<Placed[]>([]);
  const last = useRef<Placed[]>(placed);
  useEffect(() => {
    const now = new Set(placed.map((p) => p.id));
    const gone = last.current.filter((p) => p.kind === 'keybit' && !now.has(p.id));
    last.current = placed;
    if (!gone.length) return;
    setLeaving((l) => [...l.filter((p) => !now.has(p.id)), ...gone]);
    // Not cleared when placed changes again: several Keybits often leave in a row.
    setTimeout(() => setLeaving((l) => l.filter((p) => !gone.includes(p))), LEAVE_MS);
  }, [placed]);

  // Fit everything in the panel's width and MAX_H.
  const box = useRef<HTMLDivElement>(null);
  const [width, setWidth] = useState(0);
  useLayoutEffect(() => {
    const el = box.current;
    if (!el) return;
    const ro = new ResizeObserver(() => setWidth(el.clientWidth));
    ro.observe(el);
    setWidth(el.clientWidth);
    return () => ro.disconnect();
  }, []);
  const b = bounds([...placed, ...leaving]);
  const scale = width ? Math.min(1, (width - 2 * PAD) / b.w, MAX_H / b.h) : 1;

  const tile = (p: Placed, gone: boolean) => {
    const style = { left: p.x - b.x, top: p.y - b.y, width: p.w, height: p.h };
    const move = 'absolute motion-safe:transition-[left,top] motion-safe:duration-300 motion-safe:ease-out';

    if (p.kind === 'keyword') {
      return (
        <div key={p.id} style={style} className={clsx(move, 'flex flex-col items-center justify-center overflow-hidden rounded-xl bg-keyword px-1.5 text-center text-keyword-fg shadow-inner')}>
          <span className="text-sm font-semibold tracking-wide">Keyword</span>
          <small className="mt-0.5 line-clamp-1 text-[11px] opacity-70">{activeProfile().name}</small>
        </div>
      );
    }

    const uid = p.uid;
    const present = !!uid || !!p.present;
    const label = !present ? '' : uid ? actionLabel(entryOf(uid)) || keyName(uid) : 'Keybit';
    return (
      <button
        key={p.id}
        type="button"
        style={style}
        title={uid ? `${keyName(uid)} · uid ${uid}${p.estimated ? ' · position estimated' : ''}` : isDemo ? 'Attach a Keybit here' : ''}
        disabled={gone || (!isDemo && !uid) || (isDemo && !p.side)}
        onClick={() => {
          const d = demo();
          if (d && p.side) d.toggle(p.side);
          else if (uid) openAssign(uid);
        }}
        className={clsx(
          move,
          'grid place-items-center overflow-hidden rounded-xl border-2 px-1.5 text-center text-xs font-medium',
          !present ? 'border-dashed border-line text-transparent'
            : p.estimated ? 'border-dashed border-on bg-on-soft/70 text-on'   /* placed by a guess */
            : 'border-solid border-on bg-on-soft text-on',
          uid && keys[uid]?.pressed && 'scale-95 border-accent bg-press text-accent',
          (isDemo || uid) && !gone && 'hover:border-accent',
          p.kind === 'keybit' && (gone ? 'motion-safe:animate-tile-out' : 'motion-safe:animate-tile-in'),
          gone && 'pointer-events-none opacity-0',
        )}
      >
        <span className="line-clamp-2">{label}</span>
        {p.rot !== undefined && present && (
          // A notch on the Keybit's own N side, so its turn shows.
          <span
            aria-hidden
            className={clsx('absolute rounded-full bg-on opacity-60', ['top-1 left-1/2 h-1 w-4 -translate-x-1/2', 'top-1/2 right-1 h-4 w-1 -translate-y-1/2', 'bottom-1 left-1/2 h-1 w-4 -translate-x-1/2', 'top-1/2 left-1 h-4 w-1 -translate-y-1/2'][p.rot])}
          />
        )}
      </button>
    );
  };

  /** A Keybit that could be on the left or the top gets a button to swap it. */
  const flipper = (p: Placed) => {
    const k = p.uid ? keys[p.uid] : undefined;
    if (p.kind !== 'keybit' || !p.side || !k?.at || !otherSide(k.at)) return null;
    const to = otherSide(p.side) === 'T' ? 'top' : 'left';
    return (
      <button
        key={`flip-${p.id}`}
        type="button"
        title={`Turned 90°? Move it to the ${to}`}
        aria-label={`Move to the ${to}`}
        onClick={() => flip(p.uid!, p.side!)}
        style={{ left: p.x - b.x + p.w - 26, top: p.y - b.y + 4 }}
        className="absolute grid size-[22px] place-items-center rounded-md text-muted hover:bg-line hover:text-fg motion-safe:transition-[left,top] motion-safe:duration-300"
      >
        <ArrowLeftRight size={13} />
      </button>
    );
  };

  return (
    <div ref={box} className="flex flex-col items-center gap-2 rounded-xl border border-line bg-panel py-5">
      <div
        className="relative motion-safe:transition-[width,height] motion-safe:duration-300"
        style={{ width: b.w * scale, height: b.h * scale }}
      >
        <div
          className="absolute top-0 left-0 origin-top-left motion-safe:transition-transform motion-safe:duration-300 motion-safe:ease-out"
          style={{ width: b.w, height: b.h, transform: `scale(${scale})` }}
          aria-label="The Keyword and the Keybits on it"
        >
          {placed.map((p) => tile(p, false))}
          {leaving.map((p) => tile(p, true))}
          {placed.map(flipper)}
        </div>
      </div>
      {isDemo && <p className="text-xs text-muted">Demo: click a side to attach or remove a Keybit.</p>}
    </div>
  );
}
