// Small shared building blocks.
import clsx from 'clsx';
import { useEffect, useRef, type ButtonHTMLAttributes, type ReactNode } from 'react';

type Variant = 'primary' | 'ghost' | 'link' | 'danger';

export function Button({
  variant = 'primary', size = 'md', className, ...props
}: ButtonHTMLAttributes<HTMLButtonElement> & { variant?: Variant; size?: 'sm' | 'md' }) {
  return (
    <button
      type="button"
      className={clsx(
        'inline-flex items-center justify-center gap-1.5 rounded-lg font-medium whitespace-nowrap transition-colors disabled:cursor-default disabled:opacity-45',
        size === 'sm' ? 'h-7 px-2.5 text-xs' : 'h-8 px-3.5',
        variant === 'primary' && 'bg-accent text-accent-fg hover:brightness-110',
        variant === 'ghost' && 'border border-line bg-panel text-fg hover:bg-app',
        variant === 'danger' && 'border border-danger/40 bg-panel text-danger hover:bg-danger/10',
        variant === 'link' && 'h-auto px-0 text-accent hover:underline',
        className,
      )}
      {...props}
    />
  );
}

export function IconButton({ label, className, ...props }: ButtonHTMLAttributes<HTMLButtonElement> & { label: string }) {
  return (
    <button
      type="button"
      aria-label={label}
      title={props.title ?? label}
      className={clsx(
        'inline-flex size-8 items-center justify-center rounded-lg border border-line bg-panel text-fg transition-colors hover:bg-app disabled:cursor-default disabled:opacity-40',
        className,
      )}
      {...props}
    />
  );
}

export function Pill({ children, tone, title }: { children: ReactNode; tone?: 'warn' | 'on'; title?: string }) {
  return (
    <span
      title={title}
      className={clsx(
        'inline-flex items-center gap-1.5 rounded-full border px-2 py-0.5 text-xs',
        tone === 'warn' ? 'border-warn/40 bg-warn-soft text-warn' : tone === 'on' ? 'border-on/40 bg-on-soft text-on' : 'border-line bg-panel text-muted',
      )}
    >
      {children}
    </span>
  );
}

export function Chip({ children, onRemove, removeLabel }: { children: ReactNode; onRemove?: () => void; removeLabel?: string }) {
  return (
    <span className="inline-flex items-center gap-1 rounded-full border border-line bg-app px-2 py-0.5 text-xs">
      {children}
      {onRemove && (
        <button
          type="button"
          aria-label={removeLabel ?? 'Remove'}
          title={removeLabel ?? 'Remove'}
          onClick={(e) => { e.stopPropagation(); onRemove(); }}
          className="-mr-1 rounded-full px-1 text-muted hover:text-danger"
        >
          ×
        </button>
      )}
    </span>
  );
}

export function SectionTitle({ children, action }: { children: ReactNode; action?: ReactNode }) {
  return (
    <div className="mb-2 flex items-center justify-between gap-2">
      <h2 className="text-[11px] font-semibold tracking-wider text-muted uppercase">{children}</h2>
      {action}
    </div>
  );
}

export const inputClass =
  'h-8 w-full rounded-lg border border-line bg-panel px-2.5 text-fg placeholder:text-muted focus:border-accent focus:outline-none';

/** A native modal dialog. Escape and the backdrop close it through onClose. */
export function Dialog({
  open, onClose, title, children, actions, wide,
}: {
  open: boolean;
  onClose: () => void;
  title: ReactNode;
  children: ReactNode;
  actions?: ReactNode;
  wide?: boolean;
}) {
  const ref = useRef<HTMLDialogElement>(null);
  useEffect(() => {
    const d = ref.current;
    if (!d) return;
    if (open && !d.open) d.showModal();
    if (!open && d.open) d.close();
  }, [open]);
  return (
    <dialog
      ref={ref}
      onCancel={(e) => { e.preventDefault(); onClose(); }}
      onMouseDown={(e) => { if (e.target === ref.current) onClose(); }}
      className={clsx(
        'm-auto max-h-[90vh] rounded-xl border border-line bg-panel p-0 text-fg shadow-2xl',
        wide ? 'w-[min(620px,94vw)]' : 'w-[min(440px,92vw)]',
      )}
    >
      {open && (
        <div className="flex max-h-[90vh] flex-col">
          <h2 className="px-5 pt-4 pb-3 text-[15px] font-semibold">{title}</h2>
          <div className="flex-1 space-y-3 overflow-y-auto px-5 pb-1">{children}</div>
          {actions && <div className="flex items-center justify-end gap-2 px-5 pt-3 pb-4">{actions}</div>}
        </div>
      )}
    </dialog>
  );
}

export function Toggle({ checked, onChange, children }: { checked: boolean; onChange: (v: boolean) => void; children: ReactNode }) {
  return (
    <label className="flex cursor-pointer items-center gap-2.5 py-1">
      <input type="checkbox" className="size-4 accent-(--accent)" checked={checked} onChange={(e) => onChange(e.target.checked)} />
      <span>{children}</span>
    </label>
  );
}
