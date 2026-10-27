import clsx from 'clsx';
import { Loader2 } from 'lucide-react';
import { pickDevice } from '../../controller.ts';
import { useApp } from '../../store.ts';
import { Button, Dialog } from '../ui.tsx';

/** Electron has no device picker for Web Serial or Web Bluetooth, so main forwards the list here. */
export function ChooserDialog() {
  const chooser = useApp((s) => s.chooser);
  const serial = chooser?.kind === 'serial';
  return (
    <Dialog
      open={!!chooser}
      onClose={() => pickDevice('')}
      title={serial ? 'Choose the Keyword serial port' : 'Choose the Keyword'}
      actions={<Button variant="ghost" onClick={() => pickDevice('')}>Cancel</Button>}
    >
      {chooser?.devices.length ? (
        <ul className="space-y-1.5">
          {chooser.devices.map((d) => (
            <li key={d.id}>
              <button
                type="button"
                onClick={() => pickDevice(d.id)}
                className={clsx(
                  'flex w-full flex-col items-start rounded-lg border px-3 py-2 text-left hover:border-accent hover:bg-press',
                  d.likely ? 'border-accent/60' : 'border-line',
                )}
              >
                <span className="font-medium">
                  {d.name}
                  {d.likely && <span className="ml-2 text-xs font-normal text-accent">Keyword</span>}
                </span>
                <small className="text-muted">{d.detail}</small>
              </button>
            </li>
          ))}
        </ul>
      ) : (
        <p className="flex items-center gap-2 text-muted">
          {serial ? 'No serial ports found. Is the Keyword plugged in?' : <><Loader2 size={14} className="animate-spin" /> Searching for Bluetooth devices…</>}
        </p>
      )}
      {!serial && chooser && (
        <p className="text-xs text-muted">A Keyword waiting for a firmware update shows up as "Keyword OTA".</p>
      )}
    </Dialog>
  );
}
