import { closeDialog, bridge, exportAll, openOta, resetStats, setNotifications, updateSettings } from '../../controller.ts';
import type { Theme } from '../../lib/types.ts';
import { useApp } from '../../store.ts';
import { Button, Dialog, inputClass, Toggle } from '../ui.tsx';

export function SettingsDialog() {
  const open = useApp((s) => s.dialog === 'settings');
  const s = useApp((st) => st.store.settings);
  const about = useApp((st) => st.about);
  return (
    <Dialog open={open} onClose={closeDialog} title="Settings" actions={<Button onClick={closeDialog}>Done</Button>}>
      <label className="block">
        <span className="mb-1 block text-xs text-muted">Theme</span>
        <select className={inputClass} value={s.theme} onChange={(e) => updateSettings({ theme: e.target.value as Theme })}>
          <option value="system">Match system</option>
          <option value="light">Light</option>
          <option value="dark">Dark</option>
        </select>
      </label>
      <div>
        <Toggle checked={s.autoReconnect} onChange={(v) => updateSettings({ autoReconnect: v })}>Reconnect to the last Keyword automatically</Toggle>
        <Toggle checked={s.perApp} onChange={(v) => updateSettings({ perApp: v })}>Switch profiles by the app in front</Toggle>
        <Toggle checked={s.notifications} onChange={(v) => void setNotifications(v)}>Notify when Keybits connect or disconnect</Toggle>
        <Toggle checked={s.closeToTray} onChange={(v) => updateSettings({ closeToTray: v })}>Keep running in the menu bar / tray when closed</Toggle>
        <Toggle checked={s.launchAtLogin} onChange={(v) => updateSettings({ launchAtLogin: v })}>Open at login</Toggle>
      </div>
      <div className="flex flex-wrap gap-1.5">
        <Button size="sm" variant="ghost" onClick={() => void openOta()}>Update Keyword firmware…</Button>
        <Button size="sm" variant="ghost" onClick={() => void exportAll()}>Export all profiles…</Button>
        <Button size="sm" variant="ghost" onClick={resetStats}>Reset press counts</Button>
        {bridge && <Button size="sm" variant="ghost" onClick={() => bridge?.openDataFolder()}>Open data folder</Button>}
      </div>
      {about && <p className="text-xs text-muted">{about}</p>}
    </Dialog>
  );
}
