import { actionLabel, closeDialog, MAC, applyLibraryTemplate } from '../../controller.ts';
import { TEMPLATES } from '../../lib/templates.ts';
import { useApp } from '../../store.ts';
import { Button, Dialog } from '../ui.tsx';

export function LibraryDialog() {
  const open = useApp((s) => s.dialog === 'library');
  return (
    <Dialog
      open={open}
      onClose={closeDialog}
      wide
      title="Profile library"
      actions={<Button variant="ghost" onClick={closeDialog}>Close</Button>}
    >
      <p className="text-muted">Ready-made layouts. Actions go to your Keybits in order: left, top edge, right.</p>
      <ul className="divide-y divide-line">
        {TEMPLATES.map((t) => (
          <li key={t.id} className="flex items-start gap-3 py-3">
            <div className="min-w-0 flex-1">
              <b>{t.name}</b>
              <p className="text-muted">{t.description}</p>
              <p className="mt-1 truncate font-mono text-xs text-muted">
                {t.build(MAC).map((a) => a.name || actionLabel(a)).join(' · ')}
              </p>
            </div>
            <div className="flex shrink-0 gap-1.5">
              <Button size="sm" onClick={() => applyLibraryTemplate(t, true)}>New profile</Button>
              <Button size="sm" variant="ghost" title="Overwrite these keys in the current profile" onClick={() => applyLibraryTemplate(t, false)}>
                Apply here
              </Button>
            </div>
          </li>
        ))}
      </ul>
    </Dialog>
  );
}
