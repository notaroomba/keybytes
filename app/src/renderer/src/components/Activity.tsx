import clsx from 'clsx';
import { useApp } from '../store.ts';
import { Button, SectionTitle } from './ui.tsx';

export function Activity() {
  const items = useApp((s) => s.activity);
  return (
    <div className="flex min-h-40 flex-1 flex-col rounded-xl border border-line bg-panel p-3.5">
      <SectionTitle action={<Button variant="link" size="sm" onClick={() => useApp.setState({ activity: [] })}>Clear</Button>}>
        Activity
      </SectionTitle>
      {items.length ? (
        <ul className="min-h-0 flex-1 space-y-1 overflow-y-auto text-xs">
          {items.map((a) => (
            <li key={a.id} className="flex gap-2">
              <time className="shrink-0 text-muted tabular-nums">{a.time}</time>
              <span className={clsx(a.tone === 'add' && 'text-on', a.tone === 'remove' && 'text-danger')}>{a.text}</span>
            </li>
          ))}
        </ul>
      ) : (
        <p className="text-xs text-muted">Connections, presses that run actions and profile changes show up here.</p>
      )}
    </div>
  );
}
