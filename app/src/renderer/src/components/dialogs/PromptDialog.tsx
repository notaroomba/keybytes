import { useEffect, useId, useState } from 'react';
import { useApp } from '../../store.ts';
import { Button, Dialog, inputClass } from '../ui.tsx';

/** Asks for a line of text (profile names, app names). */
export function PromptDialog() {
  const prompt = useApp((s) => s.prompt);
  const [value, setValue] = useState('');
  const listId = useId();
  useEffect(() => { if (prompt) setValue(prompt.value); }, [prompt]);

  const done = (v: string | null) => {
    prompt?.resolve(v);
    useApp.setState({ prompt: null });
  };

  return (
    <Dialog
      open={!!prompt}
      onClose={() => done(null)}
      title={prompt?.title}
      actions={
        <>
          <Button variant="ghost" onClick={() => done(null)}>Cancel</Button>
          <Button onClick={() => done(value.trim() || null)}>OK</Button>
        </>
      }
    >
      <form onSubmit={(e) => { e.preventDefault(); done(value.trim() || null); }}>
        <input
          autoFocus
          onFocus={(e) => e.target.select()}
          className={inputClass}
          maxLength={80}
          value={value}
          list={listId}
          onChange={(e) => setValue(e.target.value)}
        />
        <datalist id={listId}>{prompt?.suggestions.map((s) => <option key={s} value={s} />)}</datalist>
      </form>
    </Dialog>
  );
}

export function ConfirmDialog() {
  const confirm = useApp((s) => s.confirm);
  const done = (ok: boolean) => {
    confirm?.resolve(ok);
    useApp.setState({ confirm: null });
  };
  return (
    <Dialog
      open={!!confirm}
      onClose={() => done(false)}
      title={confirm?.title}
      actions={
        <>
          <Button variant="ghost" onClick={() => done(false)}>{confirm?.cancel}</Button>
          <Button autoFocus onClick={() => done(true)}>{confirm?.ok}</Button>
        </>
      }
    >
      <p className="whitespace-pre-line text-muted">{confirm?.body}</p>
    </Dialog>
  );
}
