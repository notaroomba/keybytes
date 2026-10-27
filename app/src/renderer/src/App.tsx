import clsx from 'clsx';
import { Activity } from './components/Activity.tsx';
import { Board } from './components/Board.tsx';
import { Console } from './components/Console.tsx';
import { AssignDialog } from './components/dialogs/AssignDialog.tsx';
import { ChooserDialog } from './components/dialogs/ChooserDialog.tsx';
import { ConfirmDialog, PromptDialog } from './components/dialogs/PromptDialog.tsx';
import { LibraryDialog } from './components/dialogs/LibraryDialog.tsx';
import { OtaDialog } from './components/dialogs/OtaDialog.tsx';
import { SettingsDialog } from './components/dialogs/SettingsDialog.tsx';
import { Header } from './components/Header.tsx';
import { KeyList } from './components/KeyList.tsx';
import { ProfilePanel } from './components/ProfilePanel.tsx';
import { StatusBar } from './components/StatusBar.tsx';
import { Toasts } from './components/Toasts.tsx';
import { importTexts } from './controller.ts';
import { useApp } from './store.ts';

export default function App() {
  const dropping = useApp((s) => s.dropping);

  // Drag a profile file onto the window to import it.
  const onDrop = async (e: React.DragEvent) => {
    e.preventDefault();
    useApp.setState({ dropping: false });
    const files = [...e.dataTransfer.files].filter((f) => /\.json$/i.test(f.name));
    if (files.length) await importTexts(await Promise.all(files.map(async (f) => ({ name: f.name, text: await f.text() }))));
  };

  return (
    <div
      className={clsx('flex h-full flex-col', dropping && 'outline-2 -outline-offset-4 outline-dashed outline-accent')}
      onDragOver={(e) => { e.preventDefault(); if (!dropping) useApp.setState({ dropping: true }); }}
      onDragLeave={(e) => { if (!e.relatedTarget) useApp.setState({ dropping: false }); }}
      onDrop={onDrop}
    >
      <Header />
      <StatusBar />
      <main className="grid min-h-0 flex-1 grid-cols-1 gap-4 overflow-y-auto p-4 md:grid-cols-[minmax(0,1fr)_300px] md:overflow-hidden">
        <section className="flex min-h-0 flex-col gap-4 md:overflow-y-auto">
          <Board />
          <KeyList />
        </section>
        <aside className="flex min-h-0 flex-col gap-4 md:overflow-y-auto">
          <ProfilePanel />
          <Activity />
        </aside>
      </main>
      <Console />
      <Toasts />
      <ChooserDialog />
      <AssignDialog />
      <LibraryDialog />
      <SettingsDialog />
      <OtaDialog />
      <PromptDialog />
      <ConfirmDialog />
    </div>
  );
}
