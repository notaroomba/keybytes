// Keymap profiles: { id, name, keys: { <UID>: { mods, key, name? } } }.
// The store is { active: <id>, profiles: [...] } and is saved as JSON by main.js.
(function (root) {
  const FORMAT = 'keyword-profile';

  const newId = () => Math.random().toString(36).slice(2, 10);

  function newProfile(name, keys = {}) {
    return { id: newId(), name: name || 'Profile', keys: { ...keys } };
  }

  function cleanKeys(keys) {
    const out = {};
    if (!keys || typeof keys !== 'object') return out;
    for (const [uid, m] of Object.entries(keys)) {
      if (!/^([0-9a-fA-F]{24}|[0-9a-fA-F]{8})$/.test(uid) || !m || typeof m !== 'object') continue;
      const entry = { mods: (Number(m.mods) || 0) & 0xff, key: (Number(m.key) || 0) & 0xff };
      if (typeof m.name === 'string' && m.name.trim()) entry.name = m.name.trim().slice(0, 40);
      out[uid.toUpperCase()] = entry;
    }
    return out;
  }

  // What gets written by Export.
  function exportProfile(p) {
    return JSON.stringify({ format: FORMAT, version: 1, name: p.name, keys: cleanKeys(p.keys) }, null, 2);
  }

  // Parses an exported file. Throws with a readable message if it isn't one.
  function importProfile(text) {
    let data;
    try { data = JSON.parse(text); } catch { throw new Error('Not a JSON file'); }
    if (!data || data.format !== FORMAT) throw new Error('Not a Keyword profile');
    if (data.version !== 1) throw new Error(`Unsupported profile version ${data.version}`);
    return newProfile(typeof data.name === 'string' && data.name.trim() ? data.name.trim() : 'Imported', cleanKeys(data.keys));
  }

  // Repairs a store loaded from disk so the UI can trust it.
  function normalizeStore(store) {
    const profiles = Array.isArray(store?.profiles)
      ? store.profiles.filter((p) => p && typeof p === 'object').map((p) => ({
          id: typeof p.id === 'string' ? p.id : newId(),
          name: typeof p.name === 'string' && p.name ? p.name : 'Profile',
          keys: cleanKeys(p.keys),
        }))
      : [];
    if (!profiles.length) profiles.push(newProfile('Default'));
    const active = profiles.some((p) => p.id === store?.active) ? store.active : profiles[0].id;
    return { active, profiles };
  }

  const api = { newProfile, exportProfile, importProfile, normalizeStore, cleanKeys };
  if (typeof module !== 'undefined' && module.exports) module.exports = api;
  else root.Profiles = api;
})(typeof window !== 'undefined' ? window : globalThis);
