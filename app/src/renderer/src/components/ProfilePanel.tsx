import { Copy, Download, FolderOpen, Library, Pencil, Plus, Send, Trash2 } from 'lucide-react';
import {
  activeProfile, addAppFlow, applyProfile, deleteProfile, duplicateProfile, exportCurrent, importFlow, MOD_KEY,
  newProfileFlow, openDialog, removeApp, renameProfile, switchTo,
} from '../controller.ts';
import { useApp } from '../store.ts';
import { Button, Chip, inputClass, SectionTitle } from './ui.tsx';

export function ProfilePanel() {
  const profiles = useApp((s) => s.store.profiles);
  const active = useApp((s) => s.store.active);
  const perApp = useApp((s) => s.store.settings.perApp);
  const connected = useApp((s) => !!s.transport && !!s.device.fw);
  const p = activeProfile();

  return (
    <div className="rounded-xl border border-line bg-panel p-3.5">
      <SectionTitle>Profile</SectionTitle>
      <select
        aria-label="Active profile"
        className={inputClass}
        value={active}
        onChange={(e) => switchTo(e.target.value)}
      >
        {profiles.map((x, i) => (
          <option key={x.id} value={x.id} title={i < 9 ? `${MOD_KEY}+${i + 1}` : ''}>{x.name}</option>
        ))}
      </select>
      <div className="mt-2 grid grid-cols-4 gap-1.5">
        <Button size="sm" variant="ghost" title="New profile" onClick={() => void newProfileFlow()}><Plus size={13} /> New</Button>
        <Button size="sm" variant="ghost" title="Duplicate" onClick={() => void duplicateProfile()}><Copy size={13} /></Button>
        <Button size="sm" variant="ghost" title="Rename" onClick={() => void renameProfile()}><Pencil size={13} /></Button>
        <Button size="sm" variant="ghost" title="Delete" disabled={profiles.length < 2} onClick={deleteProfile}><Trash2 size={13} /></Button>
      </div>
      <div className="mt-1.5 grid grid-cols-3 gap-1.5">
        <Button size="sm" variant="ghost" onClick={() => openDialog('library')}><Library size={13} /> Library</Button>
        <Button size="sm" variant="ghost" onClick={() => void importFlow()}><FolderOpen size={13} /> Import</Button>
        <Button size="sm" variant="ghost" onClick={() => void exportCurrent()}><Download size={13} /> Export</Button>
      </div>
      <Button className="mt-2 w-full" size="sm" disabled={!connected} onClick={() => applyProfile(false)}>
        <Send size={13} /> Send to Keyword
      </Button>

      <div className="mt-3 border-t border-line pt-3">
        <div className="mb-1.5 flex items-center justify-between">
          <b className="text-xs">Use automatically in</b>
          <Button variant="link" size="sm" onClick={() => void addAppFlow()}>+ Add app</Button>
        </div>
        <div className="flex flex-wrap gap-1">
          {p.apps.map((a) => <Chip key={a} onRemove={() => removeApp(a)} removeLabel={`Stop using in ${a}`}>{a}</Chip>)}
        </div>
        <p className="mt-1 text-xs text-muted">
          {!perApp
            ? 'Turn on "Switch profiles by the app in front" in Settings to use this.'
            : p.apps.length
              ? ''
              : 'Not tied to any app. Other profiles switch back to this one when you leave their app.'}
        </p>
      </div>
    </div>
  );
}
