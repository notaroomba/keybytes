import clsx from 'clsx';
import { activeProfile } from '../controller.ts';
import { useApp } from '../store.ts';
import { Pill } from './ui.tsx';

export function StatusBar() {
  const status = useApp((s) => s.status);
  const tone = useApp((s) => s.tone);
  const transport = useApp((s) => s.transport);
  const device = useApp((s) => s.device);
  const perApp = useApp((s) => s.store.settings.perApp);
  const frontApp = useApp((s) => s.frontApp);
  const autoSwitched = useApp((s) => s.autoSwitched);
  useApp((s) => s.store.active);
  const b = device.battery;

  return (
    <div className="flex flex-wrap items-center gap-2 px-4 pt-2.5 text-muted">
      <span
        className={clsx(
          'size-2 rounded-full',
          tone === 'connected' ? 'bg-on' : tone === 'busy' ? 'animate-pulse bg-accent' : tone === 'stale' ? 'bg-warn' : 'bg-off',
        )}
      />
      <span className={clsx(tone === 'connected' && 'text-fg')}>{status}</span>
      <span className="ml-auto flex flex-wrap gap-1.5">
        {perApp && frontApp && (
          <Pill title="Per-app profiles are on (Settings)">
            {autoSwitched ? `${frontApp} → ${activeProfile().name}` : `App: ${frontApp}`}
          </Pill>
        )}
        {transport && device.fw && <Pill>Firmware {device.fw}</Pill>}
        {transport && b && (
          <Pill tone={b.pct <= 15 && !b.charging ? 'warn' : b.charging ? 'on' : undefined}>
            <span className="relative inline-block h-2.5 w-5 rounded-[3px] border border-current p-px after:absolute after:top-1/2 after:-right-[3px] after:h-1 after:w-0.5 after:-translate-y-1/2 after:rounded-r-sm after:bg-current">
              <span className="block h-full rounded-[1px] bg-current" style={{ width: `${b.pct}%` }} />
            </span>
            {b.pct}%{b.charging ? ' charging' : b.usb ? ' on USB' : ''}
          </Pill>
        )}
      </span>
    </div>
  );
}
