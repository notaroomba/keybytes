import clsx from 'clsx';
import { ChevronDown, ChevronUp, Copy, Trash2 } from 'lucide-react';
import { useEffect, useLayoutEffect, useRef, useState } from 'react';
import { MOD_KEY, send, toast } from '../controller.ts';
import { useApp } from '../store.ts';
import { Button, IconButton, inputClass } from './ui.tsx';

// Handy commands for poking at the firmware. Anything typed is sent as-is.
const QUICK = ['HELLO', 'KEYS?', 'SIDES?', 'BATT?', 'STACK?', 'DOCK?', 'BOOTCFG?'];

/** The raw line log, with a box to send commands by hand. */
export function Console() {
  const open = useApp((s) => s.consoleOpen);
  const log = useApp((s) => s.log);
  const connected = useApp((s) => !!s.transport);
  const [text, setText] = useState('');
  const [hidePolls, setHidePolls] = useState(true);
  const [history, setHistory] = useState<string[]>([]);
  const [pos, setPos] = useState(-1);
  const box = useRef<HTMLDivElement>(null);
  const input = useRef<HTMLInputElement>(null);
  const stick = useRef(true);

  // The app polls SIDES?, KEYS? and BATT? on a timer; hiding them makes the rest readable.
  const isPoll = (t: string) => /^(SIDES\??|KEYS\?|KEYS END|BATT\??)( |$)/.test(t);
  const lines = hidePolls ? log.filter((l) => !isPoll(l.text)) : log;

  useLayoutEffect(() => {
    if (open && stick.current && box.current) box.current.scrollTop = box.current.scrollHeight;
  }, [lines.length, open]);
  useEffect(() => { if (open) input.current?.focus(); }, [open]);

  const submit = (line: string) => {
    const l = line.trim();
    if (!l) return;
    void send(l);
    setHistory((h) => [...h.filter((x) => x !== l), l].slice(-50));
    setPos(-1);
    setText('');
  };

  return (
    <section className={clsx('border-t border-line bg-panel', open ? 'flex h-64 flex-col' : '')}>
      <button
        type="button"
        onClick={() => useApp.setState({ consoleOpen: !open })}
        className="flex w-full items-center gap-2 px-4 py-1.5 text-xs text-muted hover:text-fg"
        title={`${MOD_KEY}+Shift+K`}
      >
        {open ? <ChevronDown size={14} /> : <ChevronUp size={14} />}
        Console
        <span className="ml-auto tabular-nums">{log.length} lines</span>
      </button>
      {open && (
        <>
          <div
            ref={box}
            onScroll={(e) => {
              const el = e.currentTarget;
              stick.current = el.scrollHeight - el.scrollTop - el.clientHeight < 24;
            }}
            className="min-h-0 flex-1 overflow-y-auto px-4 font-mono text-[11.5px] leading-[1.55] select-text"
          >
            {lines.map((l) => (
              <div key={l.id} className="flex gap-2 whitespace-pre-wrap">
                <span className="shrink-0 text-muted tabular-nums">{l.time}</span>
                <span className={clsx('shrink-0', l.dir === '->' ? 'text-accent' : l.dir === '!!' ? 'text-danger' : 'text-on')}>{l.dir}</span>
                <span className={clsx(l.dir === '!!' && 'text-danger')}>{l.text}</span>
              </div>
            ))}
          </div>
          <div className="flex flex-wrap items-center gap-1.5 border-t border-line px-4 py-2">
            <form
              className="flex min-w-60 flex-1 gap-1.5"
              onSubmit={(e) => { e.preventDefault(); submit(text); }}
            >
              <input
                ref={input}
                value={text}
                disabled={!connected}
                placeholder={connected ? 'Send a command, e.g. DOCK?' : 'Connect to send commands'}
                onChange={(e) => setText(e.target.value)}
                onKeyDown={(e) => {
                  if (e.key === 'ArrowUp' && history.length) {
                    e.preventDefault();
                    const i = pos < 0 ? history.length - 1 : Math.max(0, pos - 1);
                    setPos(i);
                    setText(history[i]);
                  } else if (e.key === 'ArrowDown' && pos >= 0) {
                    e.preventDefault();
                    const i = pos + 1;
                    setPos(i >= history.length ? -1 : i);
                    setText(i >= history.length ? '' : history[i]);
                  }
                }}
                className={clsx(inputClass, 'h-7 font-mono text-xs')}
                spellCheck={false}
                autoCapitalize="characters"
              />
              <Button size="sm" type="submit" disabled={!connected || !text.trim()}>Send</Button>
            </form>
            <div className="flex flex-wrap gap-1">
              {QUICK.map((q) => (
                <Button key={q} size="sm" variant="ghost" className="font-mono" disabled={!connected} onClick={() => submit(q)}>{q}</Button>
              ))}
            </div>
            <label className="flex items-center gap-1 text-xs text-muted">
              <input type="checkbox" checked={hidePolls} onChange={(e) => setHidePolls(e.target.checked)} /> Hide polling
            </label>
            <IconButton
              label="Copy the log"
              className="size-7"
              onClick={async () => {
                await navigator.clipboard.writeText(log.map((l) => `${l.time} ${l.dir} ${l.text}`).join('\n'));
                toast('Log copied');
              }}
            >
              <Copy size={13} />
            </IconButton>
            <IconButton label="Clear the log" className="size-7" onClick={() => useApp.setState({ log: [] })}>
              <Trash2 size={13} />
            </IconButton>
          </div>
        </>
      )}
    </section>
  );
}
