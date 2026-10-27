import clsx from 'clsx';
import { actionLabel, activeProfile, demo, entryOf, keyName, openAssign } from '../controller.ts';
import type { Side } from '../lib/types.ts';
import { useApp } from '../store.ts';

/** The Keyword from above, with a slot on each side a Keybit can attach to. */
export function Board() {
  const keys = useApp((s) => s.keys);
  const sides = useApp((s) => s.sides);
  const transport = useApp((s) => s.transport);
  useApp((s) => s.store); // labels follow the active profile
  const isDemo = transport === 'demo';

  const slot = (side: Side, className: string) => {
    const uid = Object.entries(keys).find(([, k]) => k.at === side)?.[0];
    const present = !!uid || !!sides[side];
    const label = !present ? '' : uid ? actionLabel(entryOf(uid)) || keyName(uid) : 'Keybit';
    return (
      <button
        type="button"
        title={uid ? `${keyName(uid)} · uid ${uid}` : isDemo ? 'Attach a Keybit here' : ''}
        disabled={!isDemo && !uid}
        onClick={() => {
          const d = demo();
          if (d) d.toggle(side);
          else if (uid) openAssign(uid);
        }}
        className={clsx(
          'grid place-items-center overflow-hidden rounded-xl border-2 px-1.5 text-center text-xs font-medium transition-all',
          present ? 'border-solid border-on bg-on-soft text-on' : 'border-dashed border-line text-transparent',
          uid && keys[uid].pressed && 'scale-95 border-accent bg-press text-accent',
          (isDemo || uid) && 'hover:border-accent',
          className,
        )}
      >
        <span className="line-clamp-2">{label}</span>
      </button>
    );
  };

  return (
    <div className="flex flex-col items-center gap-2 rounded-xl border border-line bg-panel py-5">
      <div className="grid grid-cols-[96px_200px_96px] grid-rows-[96px_96px] gap-2" aria-label="The Keyword and its sides">
        {slot('T', 'col-start-2 row-start-1 mx-auto w-24')}
        {slot('L', 'col-start-1 row-start-2')}
        <div className="col-start-2 row-start-2 flex flex-col items-center justify-center rounded-xl bg-keyword text-keyword-fg shadow-inner">
          <span className="font-semibold tracking-wide">Keyword</span>
          <small className="mt-0.5 text-[11px] opacity-70">{activeProfile().name}</small>
        </div>
        {slot('R', 'col-start-3 row-start-2')}
      </div>
      {isDemo && <p className="text-xs text-muted">Demo: click a side to attach or remove a Keybit.</p>}
    </div>
  );
}
