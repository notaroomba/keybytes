import clsx from 'clsx';
import { closeDialog, hasOtaImage, otaCancel, otaDfu, otaGo, pickOtaFile } from '../../controller.ts';
import { OTA } from '../../lib/protocol.ts';
import { useApp } from '../../store.ts';
import { Button, Dialog } from '../ui.tsx';

export function OtaDialog() {
  const open = useApp((s) => s.dialog === 'ota');
  const ota = useApp((s) => s.ota);
  const connected = useApp((s) => !!s.transport);

  const goLabel = {
    pick: connected ? 'Start update' : `Find ${OTA.name}`,
    find: ota.hasDevice ? 'Start update' : `Find ${OTA.name}`,
    sending: 'Sending…',
    done: 'Done',
    waiting: 'Restarting…',
  }[ota.stage];
  const goDisabled = ota.stage === 'sending' || ota.stage === 'waiting' || (ota.stage !== 'done' && !hasOtaImage());

  return (
    <Dialog
      open={open}
      onClose={closeDialog}
      title="Update Keyword firmware"
      actions={
        <>
          {ota.showDfu && <Button variant="ghost" onClick={() => void otaDfu()}>Restart into USB DFU</Button>}
          <span className="flex-1" />
          <Button variant="ghost" onClick={otaCancel}>{ota.stage === 'sending' ? 'Cancel' : 'Close'}</Button>
          <Button disabled={goDisabled} onClick={() => void otaGo()}>{goLabel}</Button>
        </>
      }
    >
      <p className="text-muted">
        Sends a new firmware image to the Keyword over Bluetooth, even when you're connected over USB. Pick{' '}
        <code className="font-mono text-xs">keyword_app.bin</code> from <code className="font-mono text-xs">firmware/release</code>.
      </p>
      {ota.stack && <p className="text-xs text-muted">{ota.stack}</p>}
      <label className="block">
        <span className="mb-1 block text-xs text-muted">Firmware image</span>
        <input
          type="file"
          accept=".bin"
          disabled={ota.busy}
          onChange={(e) => void pickOtaFile(e.target.files?.[0])}
          className="block w-full text-sm file:mr-3 file:rounded-lg file:border file:border-line file:bg-app file:px-3 file:py-1.5 file:text-fg"
        />
      </label>
      {ota.fileName && <p className="text-xs text-muted">{ota.fileName} · {(ota.fileSize / 1024).toFixed(1)} KB</p>}
      {ota.pct != null && (
        <div className="h-2 overflow-hidden rounded-full bg-line" role="progressbar" aria-valuenow={ota.pct} aria-valuemin={0} aria-valuemax={100}>
          <div className="h-full rounded-full bg-accent transition-[width] duration-200" style={{ width: `${ota.pct}%` }} />
        </div>
      )}
      {ota.status && <p aria-live="polite" className={clsx('text-sm', ota.error && 'text-danger')}>{ota.status}</p>}
    </Dialog>
  );
}
