import clsx from 'clsx';
import { dismissToast, undo } from '../controller.ts';
import { useApp } from '../store.ts';

export function Toasts() {
  const toasts = useApp((s) => s.toasts);
  return (
    <div className="pointer-events-none fixed bottom-12 left-1/2 z-50 flex -translate-x-1/2 flex-col items-center gap-2" aria-live="polite">
      {toasts.map((t) => (
        <div
          key={t.id}
          className={clsx(
            'pointer-events-auto flex max-w-[90vw] items-center gap-3 rounded-lg px-3.5 py-2 text-sm shadow-lg transition-all duration-300',
            t.error ? 'bg-danger text-white' : 'bg-keyword text-keyword-fg',
            t.leaving ? 'translate-y-2 opacity-0' : 'opacity-100',
          )}
        >
          <span>{t.text}</span>
          {t.undo && (
            <button
              type="button"
              className="font-semibold text-[#9cc0ff] hover:underline"
              onClick={() => { undo(); dismissToast(t.id); }}
            >
              Undo
            </button>
          )}
        </div>
      ))}
    </div>
  );
}
