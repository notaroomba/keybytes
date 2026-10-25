// Keymap profiles and the app's saved state.
//
// store = { active, profiles: [profile], settings, stats }
// profile = { id, name, apps: [app name], keys: { <UID>: entry } }
// entry: one main action, plus an optional name and hold action:
//   { mods, key }                    keyboard key or shortcut
//   { consumer }                     media key (HID consumer page)
//   { macro: [[mods, key], ...] }    sequence of taps
//   { host: { type, value } }        run by the app: url | app | cmd | next-profile | profile
//   name: 'Mute mic'                 optional label
//   hold: { mods, key, ms }          optional action while held (firmware tap/hold)
(function (root) {
  const FORMAT = 'keyword-profile';
  const LIBRARY_FORMAT = 'keyword-profile-library';
  const HOST_TYPES = ['url', 'app', 'cmd', 'next-profile', 'profile'];
  const MAX_MACRO_STEPS = 16;

  const DEFAULT_SETTINGS = {
    theme: 'system',         // system | light | dark
    autoReconnect: true,
    perApp: false,           // switch profiles by frontmost app
    notifications: false,    // OS notification on connect/disconnect
    closeToTray: false,
    launchAtLogin: false,
    lastConnection: null,    // { kind: 'serial', usbVendorId, usbProductId } | { kind: 'ble', id, name }
  };

  const newId = () => Math.random().toString(36).slice(2, 10);
  const byte = (n) => (Number(n) || 0) & 0xff;

  function newProfile(name, keys = {}, apps = []) {
    return { id: newId(), name: name || 'Profile', apps: [...apps], keys: JSON.parse(JSON.stringify(keys)) };
  }

  function cleanEntry(m) {
    if (!m || typeof m !== 'object') return null;
    const entry = {};
    if (m.host && HOST_TYPES.includes(m.host.type)) {
      entry.host = { type: m.host.type, value: typeof m.host.value === 'string' ? m.host.value.slice(0, 1000) : '' };
    } else if (Array.isArray(m.macro) && m.macro.length) {
      entry.macro = m.macro.slice(0, MAX_MACRO_STEPS)
        .filter((s) => Array.isArray(s) && s.length === 2)
        .map(([mods, key]) => [byte(mods), byte(key)]);
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

  function cleanKeys(keys) {
    const out = {};
    if (!keys || typeof keys !== 'object') return out;
    for (const [uid, m] of Object.entries(keys)) {
      if (!/^([0-9a-fA-F]{24}|[0-9a-fA-F]{8})$/.test(uid)) continue;
      const entry = cleanEntry(m);
      if (entry) out[uid.toUpperCase()] = entry;
    }
    return out;
  }

  const cleanApps = (apps) => (Array.isArray(apps) ? [...new Set(apps.filter((a) => typeof a === 'string' && a.trim()).map((a) => a.trim().slice(0, 80)))] : []);

  // Does an entry do anything when pressed?
  const isMapped = (m) => !!(m && (m.host || m.macro?.length || m.consumer || m.mods || m.key));

  // What gets written by Export.
  function exportProfile(p) {
    return JSON.stringify({ format: FORMAT, version: 1, name: p.name, apps: cleanApps(p.apps), keys: cleanKeys(p.keys) }, null, 2);
  }

  // Every profile in one file.
  function exportLibrary(profiles) {
    return JSON.stringify({
      format: LIBRARY_FORMAT, version: 1,
      profiles: profiles.map((p) => ({ name: p.name, apps: cleanApps(p.apps), keys: cleanKeys(p.keys) })),
    }, null, 2);
  }

  // Parses an exported profile or library. Throws with a readable message if it's neither.
  // Always returns a list of new profiles.
  function importProfiles(text) {
    let data;
    try { data = JSON.parse(text); } catch { throw new Error('Not a JSON file'); }
    const one = (d, fallback) => newProfile(typeof d.name === 'string' && d.name.trim() ? d.name.trim() : fallback, cleanKeys(d.keys), cleanApps(d.apps));
    if (data?.format === LIBRARY_FORMAT) {
      if (data.version !== 1) throw new Error(`Unsupported library version ${data.version}`);
      if (!Array.isArray(data.profiles)) throw new Error('Library has no profiles');
      return data.profiles.filter((d) => d && typeof d === 'object').map((d) => one(d, 'Imported'));
    }
    if (!data || data.format !== FORMAT) throw new Error('Not a Keyword profile');
    if (data.version !== 1) throw new Error(`Unsupported profile version ${data.version}`);
    return [one(data, 'Imported')];
  }

  function importProfile(text) {
    return importProfiles(text)[0];
  }

  // Repairs a store loaded from disk so the UI can trust it.
  function normalizeStore(store) {
    const profiles = Array.isArray(store?.profiles)
      ? store.profiles.filter((p) => p && typeof p === 'object').map((p) => ({
          id: typeof p.id === 'string' ? p.id : newId(),
          name: typeof p.name === 'string' && p.name ? p.name : 'Profile',
          apps: cleanApps(p.apps),
          keys: cleanKeys(p.keys),
        }))
      : [];
    if (!profiles.length) profiles.push(newProfile('Default'));
    const active = profiles.some((p) => p.id === store?.active) ? store.active : profiles[0].id;
    const settings = { ...DEFAULT_SETTINGS };
    for (const k of Object.keys(DEFAULT_SETTINGS)) {
      const v = store?.settings?.[k];
      if (v !== undefined && (typeof v === typeof DEFAULT_SETTINGS[k] || DEFAULT_SETTINGS[k] === null)) settings[k] = v;
    }
    if (!['system', 'light', 'dark'].includes(settings.theme)) settings.theme = 'system';
    const stats = {};
    for (const [uid, n] of Object.entries(store?.stats || {})) if (Number.isFinite(n) && n > 0) stats[uid] = Math.floor(n);
    return { active, profiles, settings, stats };
  }

  // Which profile should be active for a frontmost app, or null for "no rule".
  function profileForApp(profiles, appName) {
    if (!appName) return null;
    const a = appName.toLowerCase();
    return profiles.find((p) => p.apps.some((x) => x.toLowerCase() === a)) || null;
  }

  // Undo/redo over JSON snapshots of the parts the user edits.
  class History {
    constructor(limit = 100) {
      this.limit = limit;
      this.undoStack = [];
      this.redoStack = [];
    }
    // Call before a change, with the state as it is now.
    record(snapshot, label) {
      this.undoStack.push({ snapshot: JSON.stringify(snapshot), label });
      if (this.undoStack.length > this.limit) this.undoStack.shift();
      this.redoStack = [];
    }
    get canUndo() { return this.undoStack.length > 0; }
    get canRedo() { return this.redoStack.length > 0; }
    get undoLabel() { return this.undoStack.at(-1)?.label || ''; }
    get redoLabel() { return this.redoStack.at(-1)?.label || ''; }
    // Both take the current state and return the one to restore, or null.
    undo(current) {
      const e = this.undoStack.pop();
      if (!e) return null;
      this.redoStack.push({ snapshot: JSON.stringify(current), label: e.label });
      return { state: JSON.parse(e.snapshot), label: e.label };
    }
    redo(current) {
      const e = this.redoStack.pop();
      if (!e) return null;
      this.undoStack.push({ snapshot: JSON.stringify(current), label: e.label });
      return { state: JSON.parse(e.snapshot), label: e.label };
    }
  }

  const api = {
    DEFAULT_SETTINGS, HOST_TYPES, newProfile, cleanEntry, cleanKeys, isMapped, exportProfile, exportLibrary,
    importProfile, importProfiles, normalizeStore, profileForApp, History,
  };
  if (typeof module !== 'undefined' && module.exports) module.exports = api;
  else root.Profiles = api;
})(typeof window !== 'undefined' ? window : globalThis);
