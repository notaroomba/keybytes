import clsx from 'clsx';
import { ChevronRight } from 'lucide-react';
import { useEffect, useRef, useState, type ReactNode } from 'react';
import type { CapturedKey } from '../../../../shared/bridge';
import {
  bridge, closeAssign, entryOf, holdLabel, keyName, MAC, onKeyPress, runHost, saveEntry, setCapture, toast, unassign,
} from '../../controller.ts';
import * as Hid from '../../lib/hid.ts';
import { cleanEntry, isMapped } from '../../lib/profiles.ts';
import { DEFAULT_HOLD_MS, MAX_MACRO_STEPS } from '../../lib/protocol.ts';
import type { Entry, HostActionType, Hold, Step } from '../../lib/types.ts';
import { useApp } from '../../store.ts';
import { Button, Chip, Dialog, inputClass } from '../ui.tsx';

type Tab = 'shortcut' | 'macro' | 'host';
type Target = 'shortcut' | 'macro' | 'hold';

interface Draft {
  mods: number;
  key: number;
  consumer: number;
  macro: Step[];
  host: { type: HostActionType; value: string };
  hold: Hold;
  name: string;
}

function draftFrom(m: Entry | undefined): Draft {
  return {
    mods: m?.mods || 0,
    key: m?.key || 0,
    consumer: m?.consumer || 0,
    macro: (m?.macro || []).map((s): Step => [s[0], s[1]]),
    host: m?.host ? { ...m.host } : { type: 'url', value: '' },
    hold: m?.hold ? { ...m.hold } : { mods: 0, key: 0, ms: DEFAULT_HOLD_MS },
    name: m?.name || '',
  };
}

/** The entry the dialog describes, for the current tab. */
function toEntry(d: Draft, tab: Tab): Entry {
  let e: Entry;
  if (tab === 'host') e = { host: { type: d.host.type, value: d.host.type === 'next-profile' ? '' : d.host.value.trim() } };
  else if (tab === 'macro') e = d.macro.length ? { macro: d.macro } : { mods: 0, key: 0 };
  else e = d.consumer ? { mods: 0, key: 0, consumer: d.consumer } : { mods: d.mods, key: d.key };
  if (tab !== 'host' && (d.hold.mods || d.hold.key)) e.hold = { ...d.hold };
  if (d.name.trim()) e.name = d.name.trim();
  return cleanEntry(e)!;
}

export function AssignDialog() {
  const uid = useApp((s) => s.assignUid);
  return uid ? <AssignForm key={uid} uid={uid} /> : null;
}

function AssignForm({ uid }: { uid: string }) {
  const profiles = useApp((s) => s.store.profiles);
  const caps = useApp((s) => s.device.caps);
  const fw = useApp((s) => s.device.fw);
  const seenApps = useApp((s) => s.seenApps);
  const original = entryOf(uid);
  const [draft, setDraft] = useState<Draft>(() => draftFrom(original));
  const [tab, setTab] = useState<Tab>(original?.host ? 'host' : original?.macro?.length ? 'macro' : 'shortcut');
  const [query, setQuery] = useState('');
  const [macroText, setMacroText] = useState('');
  const [holdOpen, setHoldOpen] = useState(!!(original?.hold && (original.hold.mods || original.hold.key)));
  const [target, setTarget] = useState<Target | null>(null);
  const [blink, setBlink] = useState(0);
  const update = (patch: Partial<Draft>) => setDraft((d) => ({ ...d, ...patch }));

  // Pressing the Keybit itself flashes its name, so you can tell which one you're editing.
  useEffect(() => onKeyPress((u) => { if (u === uid) setBlink((n) => n + 1); }), [uid]);

  // Turns a captured key into the draft for whichever box is listening.
  const captured = useRef<(e: Hid.KeyLike) => void>(() => {});
  captured.current = (e) => {
    const r = Hid.fromEvent(e);
    if (!r || !target) return;
    if (target === 'shortcut') {
      if (r.consumer !== undefined) update({ mods: 0, key: 0, consumer: r.consumer });
      else update({ mods: r.mods, key: r.usage, consumer: 0 });
    } else if (target === 'macro') {
      if (r.consumer !== undefined || !r.usage) return; // wait for a real key; media keys can't go in a macro
      if (draft.macro.length >= MAX_MACRO_STEPS) return toast(`Macros hold up to ${MAX_MACRO_STEPS} steps`);
      update({ macro: [...draft.macro, [r.mods, r.usage]] });
    } else if (target === 'hold') {
      if (r.consumer !== undefined) return;
      update({ hold: { ...draft.hold, mods: r.mods, key: r.usage } });
    }
  };
  // Electron hands us every key (even Cmd+Q) while a capture box is focused.
  useEffect(() => bridge?.onCaptureKey((k: CapturedKey) => { if (k.type === 'keyDown') captured.current(k); }), []);
  useEffect(() => () => setCapture(false), []);

  const captureProps = (t: Target) => ({
    tabIndex: 0,
    onFocus: () => { setTarget(t); setCapture(true); },
    onBlur: () => { setTarget(null); setCapture(false); },
    onKeyDown: (e: React.KeyboardEvent) => { e.preventDefault(); e.stopPropagation(); captured.current(e); },
  });

  const can = (c: string) => caps.includes(c);
  const shortcutLabel = draft.consumer ? Hid.consumerLabel(draft.consumer) : Hid.comboLabel(draft.mods, draft.key, MAC);
  const results = query.trim() ? Hid.search(query).slice(0, 14) : [];
  const pick = (r: Hid.SearchResult) => {
    if (r.kind === 'c') update({ mods: 0, key: 0, consumer: r.usage });
    else update({ key: r.usage, consumer: 0 });
    setQuery('');
  };
  const needsValue = ['url', 'app', 'cmd'].includes(draft.host.type);

  const save = () => {
    const e = toEntry(draft, tab);
    if (e.host && ['url', 'app', 'cmd'].includes(e.host.type) && !e.host.value) {
      toast('Fill in what to open or run first', { error: true });
      return;
    }
    saveEntry(uid, e);
    closeAssign();
  };

  const addText = () => {
    const { steps, skipped } = Hid.textToSteps(macroText);
    const room = MAX_MACRO_STEPS - draft.macro.length;
    update({ macro: [...draft.macro, ...steps.slice(0, room)] });
    if (steps.length > room) toast(`Only ${room} more step${room === 1 ? '' : 's'} fit; the rest was cut`);
    if (skipped.length) toast(`Skipped characters with no US key: ${[...new Set(skipped)].join(' ')}`);
    setMacroText('');
  };

  const hl = holdLabel(draft.hold);

  return (
    <Dialog
      open
      wide
      onClose={closeAssign}
      title={<>Assign <span key={blink} className={clsx(blink > 0 && 'animate-blink')}>{keyName(uid)}</span></>}
      actions={
        <>
          <Button variant="ghost" disabled={!isMapped(original) && !original?.name} onClick={() => { unassign(uid); closeAssign(); }}>
            Unassign
          </Button>
          {tab === 'host' && <Button variant="ghost" onClick={() => { const e = toEntry(draft, tab); if (e.host) void runHost(e.host, uid); }}>Test</Button>}
          <span className="flex-1" />
          <Button variant="ghost" onClick={closeAssign}>Cancel</Button>
          <Button onClick={save}>Save</Button>
        </>
      }
    >
      <div role="tablist" className="flex gap-1 rounded-lg bg-app p-1">
        {([['shortcut', 'Key or shortcut'], ['macro', 'Macro'], ['host', 'Computer action']] as const).map(([t, label]) => (
          <button
            key={t}
            type="button"
            role="tab"
            aria-selected={tab === t}
            onClick={() => setTab(t)}
            className={clsx('flex-1 rounded-md py-1.5 text-sm', tab === t ? 'bg-panel font-medium shadow-sm' : 'text-muted hover:text-fg')}
          >
            {label}
          </button>
        ))}
      </div>

      {tab === 'shortcut' && (
        <div className="space-y-2.5">
          <CaptureBox listening={target === 'shortcut'} {...captureProps('shortcut')} autoFocus>
            {shortcutLabel || 'Click here and press a key or shortcut…'}
          </CaptureBox>
          <input
            type="search"
            className={inputClass}
            placeholder="Search keys: volume, f13, page down…"
            aria-label="Search keys"
            value={query}
            onChange={(e) => setQuery(e.target.value)}
            onKeyDown={(e) => { if (e.key === 'Enter') { e.preventDefault(); const r = Hid.search(query)[0]; if (r) pick(r); } }}
          />
          {results.length > 0 && (
            <div className="flex flex-wrap gap-1">
              {results.map((r) => (
                <button key={`${r.kind}${r.usage}`} type="button" onClick={() => pick(r)} className="rounded-full border border-line px-2 py-0.5 text-xs hover:border-accent hover:text-accent">
                  {r.label}{r.kind === 'c' ? ' ♪' : ''}
                </button>
              ))}
            </div>
          )}
          <div className="flex flex-wrap gap-x-3 gap-y-1">
            {Hid.MODS.map((m) => (
              <label key={m.bit} className={clsx('flex items-center gap-1 text-xs', draft.consumer && 'opacity-50')}>
                <input
                  type="checkbox"
                  disabled={!!draft.consumer}
                  checked={!!(draft.mods & (1 << m.bit))}
                  onChange={(e) => update({ mods: e.target.checked ? draft.mods | (1 << m.bit) : draft.mods & ~(1 << m.bit) })}
                />
                {Hid.modName(m, MAC)}
              </label>
            ))}
          </div>
          <select
            aria-label="Key"
            className={inputClass}
            value={draft.consumer ? `c:${draft.consumer}` : `k:${draft.key}`}
            onChange={(e) => {
              const [kind, usage] = e.target.value.split(':');
              if (kind === 'c') update({ mods: 0, key: 0, consumer: Number(usage) });
              else update({ key: Number(usage), consumer: 0 });
            }}
          >
            <option value="k:0">(no key, modifiers only)</option>
            <optgroup label="Keys">{Hid.KEYS.map((k) => <option key={k.usage} value={`k:${k.usage}`}>{k.label}</option>)}</optgroup>
            <optgroup label="Media">{Hid.CONSUMER.map((c) => <option key={c.usage} value={`c:${c.usage}`}>{c.label}</option>)}</optgroup>
          </select>
        </div>
      )}

      {tab === 'macro' && (
        <div className="space-y-2.5">
          <p className="text-muted">Keys are tapped in order. Click the box and press keys to record, or type text to add it.</p>
          <CaptureBox listening={target === 'macro'} {...captureProps('macro')} autoFocus>
            {draft.macro.length ? (
              <span className="flex flex-wrap gap-1">
                {draft.macro.map((st, i) => (
                  <Chip key={i} onRemove={() => update({ macro: draft.macro.filter((_, j) => j !== i) })}>{Hid.stepLabel(st, MAC)}</Chip>
                ))}
              </span>
            ) : 'Click here and press keys to record…'}
          </CaptureBox>
          <form className="flex gap-1.5" onSubmit={(e) => { e.preventDefault(); addText(); }}>
            <input className={inputClass} placeholder="Text to type, e.g. your email" value={macroText} onChange={(e) => setMacroText(e.target.value)} />
            <Button type="submit" variant="ghost" size="sm" className="h-8">Add text</Button>
            <Button variant="ghost" size="sm" className="h-8" onClick={() => update({ macro: [] })}>Clear</Button>
          </form>
          <p className="text-xs text-muted">
            {draft.macro.length}/{MAX_MACRO_STEPS} steps.
            {fw && !can('macro') && " This Keyword's firmware doesn't support macros yet, so it won't be sent."}
          </p>
        </div>
      )}

      {tab === 'host' && (
        <div className="space-y-2.5">
          <p className="text-muted">Runs on this computer while Keyword Monitor is open. The Keyword sends no key for it.</p>
          <select
            aria-label="Action"
            className={inputClass}
            value={draft.host.type}
            onChange={(e) => {
              const type = e.target.value as HostActionType;
              update({ host: { type, value: type === 'profile' ? profiles[0].id : '' } });
            }}
          >
            <option value="url">Open a website or link</option>
            <option value="app">Open an app</option>
            <option value="cmd">Run a command</option>
            <option value="next-profile">Switch to the next profile</option>
            <option value="profile">Switch to a profile</option>
          </select>
          {needsValue && (
            <>
              <input
                aria-label="Value"
                autoFocus
                className={inputClass}
                list="app-names"
                value={draft.host.value}
                placeholder={{ url: 'https://example.com or spotify:…', app: MAC ? 'App name, e.g. Spotify' : 'App or program, e.g. notepad', cmd: 'Shell command, e.g. open ~/Downloads' }[draft.host.type as 'url']}
                onChange={(e) => update({ host: { ...draft.host, value: e.target.value } })}
              />
              {draft.host.type === 'app' && <datalist id="app-names">{[...seenApps].sort().map((a) => <option key={a} value={a} />)}</datalist>}
            </>
          )}
          {draft.host.type === 'profile' && (
            <select aria-label="Profile" className={inputClass} value={draft.host.value} onChange={(e) => update({ host: { ...draft.host, value: e.target.value } })}>
              {profiles.map((p) => <option key={p.id} value={p.id}>{p.name}</option>)}
            </select>
          )}
        </div>
      )}

      {tab !== 'host' && (
        <details open={holdOpen} onToggle={(e) => setHoldOpen(e.currentTarget.open)} className="group rounded-lg border border-line px-3 py-2">
          <summary className="flex cursor-pointer list-none items-center gap-1 select-none">
            <ChevronRight size={14} className="transition-transform group-open:rotate-90" />
            When held {hl && <span className="text-muted">· {hl}</span>}
          </summary>
          <div className="mt-2 space-y-2">
            <CaptureBox small listening={target === 'hold'} {...captureProps('hold')}>
              {hl || 'Click here and press what to send while held…'}
            </CaptureBox>
            <div className="flex items-center gap-3">
              <label className="flex items-center gap-1.5 text-xs">
                Hold for
                <input
                  type="number"
                  min={50}
                  max={2000}
                  step={10}
                  value={draft.hold.ms}
                  onChange={(e) => update({ hold: { ...draft.hold, ms: Number(e.target.value) } })}
                  onBlur={() => update({ hold: { ...draft.hold, ms: Math.min(2000, Math.max(50, draft.hold.ms || DEFAULT_HOLD_MS)) } })}
                  className={clsx(inputClass, 'h-7 w-20')}
                />
                ms
              </label>
              <Button size="sm" variant="ghost" onClick={() => update({ hold: { mods: 0, key: 0, ms: draft.hold.ms } })}>No hold action</Button>
            </div>
            <p className="text-xs text-muted">
              {fw && !can('hold')
                ? "This Keyword's firmware doesn't support hold actions yet, so it won't be sent."
                : 'A quick tap sends the main action; holding longer than this sends the hold action.'}
            </p>
          </div>
        </details>
      )}

      <label className="block">
        <span className="mb-1 block text-xs text-muted">Name</span>
        <input className={inputClass} maxLength={40} placeholder="optional, e.g. Mute mic" value={draft.name} onChange={(e) => update({ name: e.target.value })} />
      </label>
    </Dialog>
  );
}

function CaptureBox({
  listening, small, children, autoFocus, ...props
}: {
  listening: boolean;
  small?: boolean;
  children: ReactNode;
  autoFocus?: boolean;
} & React.HTMLAttributes<HTMLDivElement>) {
  const ref = useRef<HTMLDivElement>(null);
  // React only autofocuses form fields. Wait a tick so the dialog has opened first.
  useEffect(() => {
    if (!autoFocus) return;
    const t = setTimeout(() => ref.current?.focus(), 0);
    return () => clearTimeout(t);
  }, [autoFocus]);
  return (
    <div
      ref={ref}
      {...props}
      role="textbox"
      aria-label="Press keys to record"
      className={clsx(
        'flex w-full cursor-text items-center justify-center rounded-lg border-2 border-dashed px-3 text-center font-mono transition-colors focus:outline-none',
        small ? 'min-h-10 text-sm' : 'min-h-16 text-base',
        listening ? 'border-accent bg-press text-accent' : 'border-line text-muted hover:border-accent/60',
      )}
    >
      {children}
    </div>
  );
}
