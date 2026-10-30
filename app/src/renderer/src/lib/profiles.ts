// Keymap profiles and the app's saved state.
import type { Entry, HostActionType, Profile, Settings, Step, Store } from './types.ts';
import { MAX_MACRO_STEPS } from './protocol.ts';

const FORMAT = 'keyword-profile';
const LIBRARY_FORMAT = 'keyword-profile-library';
export const HOST_TYPES: HostActionType[] = ['url', 'app', 'cmd', 'next-profile', 'profile'];

export const DEFAULT_SETTINGS: Settings = {
  theme: 'system',
  autoReconnect: true,
  perApp: false, // switch profiles by the app in front
  notifications: false, // OS notification on connect/disconnect
  closeToTray: false,
  launchAtLogin: false,
  lastConnection: null,
};

const newId = () => Math.random().toString(36).slice(2, 10);
const byte = (n: unknown) => (Number(n) || 0) & 0xff;
// Loaded files are untrusted, so the cleaners take anything.
// eslint-disable-next-line @typescript-eslint/no-explicit-any
type Loose = any;

export function newProfile(name: string, keys: Record<string, Entry> = {}, apps: string[] = []): Profile {
  return { id: newId(), name: name || 'Profile', apps: [...apps], keys: structuredClone(keys) };
}

export function cleanEntry(m: Loose): Entry | null {
  if (!m || typeof m !== 'object') return null;
  const entry: Entry = {};
  if (m.host && HOST_TYPES.includes(m.host.type)) {
    entry.host = { type: m.host.type, value: typeof m.host.value === 'string' ? m.host.value.slice(0, 1000) : '' };
  } else if (Array.isArray(m.macro) && m.macro.length) {
    entry.macro = m.macro.slice(0, MAX_MACRO_STEPS)
      .filter((s: unknown) => Array.isArray(s) && s.length === 2)
      .map(([mods, key]: [unknown, unknown]): Step => [byte(mods), byte(key)]);
  } else {
    const consumer = (Number(m.consumer) || 0) & 0xffff;
    if (consumer) Object.assign(entry, { mods: 0, key: 0, consumer });
    else Object.assign(entry, { mods: byte(m.mods), key: byte(m.key) });
  }
  if (m.hold && (m.hold.mods || m.hold.key)) {
    entry.hold = { mods: byte(m.hold.mods), key: byte(m.hold.key), ms: Math.min(2000, Math.max(50, Number(m.hold.ms) || 200)) };
  }
  if (typeof m.name === 'string' && m.name.trim()) entry.name = m.name.trim().slice(0, 40);
  return entry;
}

export function cleanKeys(keys: Loose): Record<string, Entry> {
  const out: Record<string, Entry> = {};
  if (!keys || typeof keys !== 'object') return out;
  for (const [uid, m] of Object.entries(keys)) {
    if (!/^([0-9a-fA-F]{24}|[0-9a-fA-F]{8})$/.test(uid)) continue;
    const entry = cleanEntry(m);
    if (entry) out[uid.toUpperCase()] = entry;
  }
  return out;
}

const cleanApps = (apps: Loose): string[] =>
  Array.isArray(apps)
    ? [...new Set(apps.filter((a): a is string => typeof a === 'string' && !!a.trim()).map((a) => a.trim().slice(0, 80)))]
    : [];

/** Does an entry do anything when pressed? */
export const isMapped = (m: Entry | null | undefined) => !!(m && (m.host || m.macro?.length || m.consumer || m.mods || m.key));

/** What gets written by Export. */
export function exportProfile(p: Profile): string {
  return JSON.stringify({ format: FORMAT, version: 1, name: p.name, apps: cleanApps(p.apps), keys: cleanKeys(p.keys) }, null, 2);
}

/** Every profile in one file. */
export function exportLibrary(profiles: Profile[]): string {
  return JSON.stringify({
    format: LIBRARY_FORMAT,
    version: 1,
    profiles: profiles.map((p) => ({ name: p.name, apps: cleanApps(p.apps), keys: cleanKeys(p.keys) })),
  }, null, 2);
}

/** Parses an exported profile or library into new profiles. Throws with a readable message if it's neither. */
export function importProfiles(text: string): Profile[] {
  let data: Loose;
  try {
    data = JSON.parse(text);
  } catch {
    throw new Error('Not a JSON file');
  }
  const one = (d: Loose, fallback: string) =>
    newProfile(typeof d.name === 'string' && d.name.trim() ? d.name.trim() : fallback, cleanKeys(d.keys), cleanApps(d.apps));
  if (data?.format === LIBRARY_FORMAT) {
    if (data.version !== 1) throw new Error(`Unsupported library version ${data.version}`);
    if (!Array.isArray(data.profiles)) throw new Error('Library has no profiles');
    return data.profiles.filter((d: unknown) => d && typeof d === 'object').map((d: Loose) => one(d, 'Imported'));
  }
  if (!data || data.format !== FORMAT) throw new Error('Not a Keyword profile');
  if (data.version !== 1) throw new Error(`Unsupported profile version ${data.version}`);
  return [one(data, 'Imported')];
}

export const importProfile = (text: string) => importProfiles(text)[0];

/** Repairs a store loaded from disk so the UI can trust it. */
export function normalizeStore(store: Loose): Store {
  const profiles: Profile[] = Array.isArray(store?.profiles)
    ? store.profiles.filter((p: unknown) => p && typeof p === 'object').map((p: Loose) => ({
        id: typeof p.id === 'string' ? p.id : newId(),
        name: typeof p.name === 'string' && p.name ? p.name : 'Profile',
        apps: cleanApps(p.apps),
        keys: cleanKeys(p.keys),
      }))
    : [];
  if (!profiles.length) profiles.push(newProfile('Default'));
  const active = profiles.some((p) => p.id === store?.active) ? store.active : profiles[0].id;
  const settings: Settings = { ...DEFAULT_SETTINGS };
  for (const k of Object.keys(DEFAULT_SETTINGS) as (keyof Settings)[]) {
    const v = store?.settings?.[k];
    if (v !== undefined && (typeof v === typeof DEFAULT_SETTINGS[k] || DEFAULT_SETTINGS[k] === null)) {
      (settings as Loose)[k] = v;
    }
  }
  if (!['system', 'light', 'dark'].includes(settings.theme)) settings.theme = 'system';
  const stats: Record<string, number> = {};
  for (const [uid, n] of Object.entries(store?.stats || {})) {
    if (typeof n === 'number' && Number.isFinite(n) && n > 0) stats[uid] = Math.floor(n);
  }
  return { active, profiles, settings, stats };
}

/** Which profile should be active for the app in front, or null for "no rule". */
export function profileForApp(profiles: Profile[], appName: string | null): Profile | null {
  if (!appName) return null;
  const a = appName.toLowerCase();
  return profiles.find((p) => p.apps.some((x) => x.toLowerCase() === a)) || null;
}

/** Undo/redo over JSON snapshots of the parts the user edits. */
export class History<T> {
  private undoStack: { snapshot: string; label: string }[] = [];
  private redoStack: { snapshot: string; label: string }[] = [];
  private readonly limit: number;
  constructor(limit = 100) {
    this.limit = limit;
  }
  /** Call before a change, with the state as it is now. */
  record(snapshot: T, label: string) {
    this.undoStack.push({ snapshot: JSON.stringify(snapshot), label });
    if (this.undoStack.length > this.limit) this.undoStack.shift();
    this.redoStack = [];
  }
  get canUndo() { return this.undoStack.length > 0; }
  get canRedo() { return this.redoStack.length > 0; }
  get undoLabel() { return this.undoStack.at(-1)?.label || ''; }
  get redoLabel() { return this.redoStack.at(-1)?.label || ''; }
  /** Takes the current state and returns the one to restore, or null. */
  undo(current: T): { state: T; label: string } | null {
    const e = this.undoStack.pop();
    if (!e) return null;
    this.redoStack.push({ snapshot: JSON.stringify(current), label: e.label });
    return { state: JSON.parse(e.snapshot), label: e.label };
  }
  redo(current: T): { state: T; label: string } | null {
    const e = this.redoStack.pop();
    if (!e) return null;
    this.undoStack.push({ snapshot: JSON.stringify(current), label: e.label });
    return { state: JSON.parse(e.snapshot), label: e.label };
  }
}
