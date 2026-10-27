import { Bluetooth, Moon, Redo2, Settings, SquareTerminal, Sun, Undo2, Unplug, Usb } from 'lucide-react';
import {
  connectBle, connectDemo, connectUsb, disconnect, MOD_KEY, openDialog, redo, toggleTheme, undo,
} from '../controller.ts';
import { useApp } from '../store.ts';
import { Button, IconButton } from './ui.tsx';

export function Header() {
  const transport = useApp((s) => s.transport);
  const history = useApp((s) => s.history);
  const theme = useApp((s) => s.store.settings.theme);
  const consoleOpen = useApp((s) => s.consoleOpen);
  const dark = typeof document !== 'undefined' && document.documentElement.dataset.theme === 'dark';

  return (
    <header className="flex flex-wrap items-center gap-2 border-b border-line bg-panel px-4 py-2.5">
      <h1 className="mr-auto flex items-center gap-2 text-[15px] font-semibold">
        <span className="grid size-6 place-items-center rounded-md bg-keyword text-keyword-fg">
          <span className="size-1.5 rounded-full bg-current" />
        </span>
        Keyword Monitor
      </h1>
      <Button onClick={connectUsb} title={`Connect over USB (${MOD_KEY}+U)`}>
        <Usb size={15} /> USB
      </Button>
      <Button onClick={connectBle} title={`Connect over Bluetooth (${MOD_KEY}+B)`}>
        <Bluetooth size={15} /> Bluetooth
      </Button>
      <Button variant="ghost" onClick={connectDemo} title="Try the app with a pretend Keyword">
        Demo
      </Button>
      {transport && (
        <Button variant="ghost" onClick={() => void disconnect()}>
          <Unplug size={15} /> Disconnect
        </Button>
      )}
      <span className="mx-1 h-6 w-px bg-line" />
      <IconButton
        label={history.canUndo ? `Undo ${history.undoLabel} (${MOD_KEY}+Z)` : 'Nothing to undo'}
        disabled={!history.canUndo}
        onClick={undo}
      >
        <Undo2 size={15} />
      </IconButton>
      <IconButton label={history.canRedo ? `Redo ${history.redoLabel}` : 'Nothing to redo'} disabled={!history.canRedo} onClick={redo}>
        <Redo2 size={15} />
      </IconButton>
      <IconButton label={`Switch to ${dark ? 'light' : 'dark'} theme (${MOD_KEY}+Shift+L)`} onClick={toggleTheme} data-theme-setting={theme}>
        {dark ? <Sun size={15} /> : <Moon size={15} />}
      </IconButton>
      <IconButton
        label={`Console (${MOD_KEY}+Shift+K)`}
        aria-pressed={consoleOpen}
        className={consoleOpen ? 'border-accent text-accent' : ''}
        onClick={() => useApp.setState({ consoleOpen: !consoleOpen })}
      >
        <SquareTerminal size={15} />
      </IconButton>
      <IconButton label="Settings" onClick={() => openDialog('settings')}>
        <Settings size={15} />
      </IconButton>
    </header>
  );
}
