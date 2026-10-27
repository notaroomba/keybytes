import clsx from 'clsx';
import { Search } from 'lucide-react';
import {
  actionLabel, activeProfile, copyUid, demo, entryOf, flashKey, forgetKey, holdLabel, isOutdated, keyName, moveKey,
  openAssign, SIDE_NAMES,
} from '../controller.ts';
import { isMapped } from '../lib/profiles.ts';
import { useApp } from '../store.ts';
import { Button, inputClass, SectionTitle } from './ui.tsx';

const ORDER: Record<string, number> = { L: 0, T: 1, R: 2 };

export function KeyList() {
  const keys = useApp((s) => s.keys);
  const filter = useApp((s) => s.filter);
  const transport = useApp((s) => s.transport);
  useApp((s) => s.store);
  const p = activeProfile();

  // Connected first (left, top, right), then keys set up in this profile that are offline.
  const online = Object.entries(keys).sort((a, b) => (ORDER[a[1].at ?? ''] ?? 9) - (ORDER[b[1].at ?? ''] ?? 9)).map(([u]) => u);
  const offline = Object.keys(p.keys).filter((u) => !keys[u]).sort();
  const q = filter.trim().toLowerCase();
  const matches = (uid: string) =>
    !q || [keyName(uid), actionLabel(entryOf(uid)), uid, holdLabel(entryOf(uid)?.hold)].join(' ').toLowerCase().includes(q);
  const shown = [...online, ...offline].filter(matches);
  const any = online.length + offline.length > 0;

  return (
    <div>
      <SectionTitle
        action={
          <label className="relative w-48">
            <Search size={13} className="pointer-events-none absolute top-1/2 left-2.5 -translate-y-1/2 text-muted" />
            <input
              id="key-filter"
              type="search"
              placeholder="Find a key…"
              aria-label="Find a key"
              value={filter}
              onChange={(e) => useApp.setState({ filter: e.target.value })}
              className={clsx(inputClass, 'h-7 pl-7 text-xs')}
            />
          </label>
        }
      >
        Keys
      </SectionTitle>
      {shown.length ? (
        <ul className="space-y-2">
          {shown.map((uid) => <KeyCard key={uid} uid={uid} />)}
        </ul>
      ) : (
        <p className="text-muted">
          {q && any
            ? `No keys match "${q}".`
            : transport
              ? "No Keybits connected. Attach one to the Keyword's left, right or top edge."
              : 'Connect to the Keyword, or try Demo, to see its Keybits.'}
        </p>
      )}
    </div>
  );
}

function KeyCard({ uid }: { uid: string }) {
  const k = useApp((s) => s.keys[uid]);
  const keys = useApp((s) => s.keys);
  const kb = useApp((s) => s.device.kb);
  const canFlash = useApp((s) => s.device.caps.includes('flash'));
  const presses = useApp((s) => s.store.stats[uid] || 0);
  const highlight = useApp((s) => s.highlight === uid);
  const isDemo = useApp((s) => s.transport === 'demo');
  const m = entryOf(uid);
  const combo = actionLabel(m);
  const hold = holdLabel(m?.hold);
  const outdated = isOutdated(k, kb);
  const where = !k
    ? 'Not connected'
    : k.at
      ? SIDE_NAMES[k.at][0].toUpperCase() + SIDE_NAMES[k.at].slice(1)
      : k.via
        ? `Chained behind ${keyName(k.via)}${k.depth && k.depth > 1 ? ` (${k.depth} hops)` : ''}`
        : 'Connected';
  const canMove = !k && m && Object.keys(keys).some((u) => !isMapped(entryOf(u)));

  return (
    <li
      onDoubleClick={(e) => { if (!(e.target as HTMLElement).closest('button')) openAssign(uid); }}
      className={clsx(
        'flex flex-wrap items-center gap-3 rounded-xl border bg-panel px-3.5 py-2.5 transition-colors',
        k?.pressed ? 'border-accent bg-press' : 'border-line',
        !k && 'opacity-60',
        highlight && 'animate-pop',
      )}
    >
      <div className="min-w-0 flex-1">
        <div className="flex flex-wrap items-center gap-1.5 font-semibold">
          {keyName(uid)}
          {k?.v != null && (
            <Badge tone={outdated ? 'warn' : undefined} title={outdated ? 'Firmware update available' : 'Keybit firmware'}>v{k.v}</Badge>
          )}
          {m?.host && <Badge title="Runs on this computer while the app is open">computer</Badge>}
          {!!m?.macro?.length && <Badge>macro</Badge>}
        </div>
        <div className="mt-0.5 text-xs text-muted">
          {where} ·{' '}
          <button type="button" className="font-mono hover:text-accent" title={`${uid}\nClick to copy`} onClick={() => void copyUid(uid)}>
            …{uid.slice(-8)}
          </button>
          {presses > 0 && ` · ${presses.toLocaleString()} press${presses === 1 ? '' : 'es'}`}
          {k?.flashing != null && <span className="text-accent"> · Flashing {k.flashing}%</span>}
        </div>
      </div>
      <div className="flex min-w-0 flex-col items-end">
        <span className={clsx('max-w-56 truncate rounded-md px-2 py-0.5 font-mono text-xs', combo ? 'bg-app text-fg' : 'text-muted italic')} title={combo}>
          {combo || 'not assigned'}
        </span>
        {hold && <span className="mt-0.5 text-[11px] text-muted">hold: {hold}</span>}
      </div>
      <div className="flex flex-wrap gap-1.5">
        <Button size="sm" variant="ghost" onClick={() => openAssign(uid)}>Assign…</Button>
        {k && isDemo && (
          <Button size="sm" variant="ghost" title="Simulate a press" onClick={() => demo()?.tap(uid)}>Tap</Button>
        )}
        {k && !k.via && canFlash && k.flashing == null && (
          <Button
            size="sm"
            variant="ghost"
            className={outdated ? 'border-warn text-warn' : ''}
            title={outdated ? `Keybit firmware v${k.v}, Keyword has v${kb}` : 'Reflash this Keybit'}
            onClick={() => flashKey(uid)}
          >
            {outdated ? 'Update' : 'Reflash'}
          </Button>
        )}
        {canMove && (
          <Button size="sm" variant="ghost" title="Give this setup to a connected Keybit, e.g. after swapping one out" onClick={() => void moveKey(uid)}>
            Move to…
          </Button>
        )}
        {!k && <Button size="sm" variant="ghost" onClick={() => forgetKey(uid)}>Forget</Button>}
      </div>
    </li>
  );
}

function Badge({ children, tone, title }: { children: React.ReactNode; tone?: 'warn'; title?: string }) {
  return (
    <span
      title={title}
      className={clsx(
        'rounded-full border px-1.5 text-[10px] leading-4 font-medium',
        tone === 'warn' ? 'border-warn/50 bg-warn-soft text-warn' : 'border-line text-muted',
      )}
    >
      {children}
    </span>
  );
}
