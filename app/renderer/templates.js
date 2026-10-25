// Ready-made profiles. Each template is a list of actions that get handed out to
// Keybits in order (left, top edge, right, then the rest), so a 3-key setup takes
// the first three.
(function (root) {
  // Modifier bits: 1 Ctrl, 2 Shift, 4 Alt, 8 GUI (Cmd/Win).
  const C = 1, S = 2, A = 4, G = 8;
  const k = (key, mods = 0, name) => ({ mods, key, ...(name ? { name } : {}) });
  const media = (consumer, name) => ({ consumer, mods: 0, key: 0, name });
  // HID usages used below.
  const U = {
    A: 0x04, C: 0x06, D: 0x07, E: 0x08, L: 0x0f, P: 0x13, Q: 0x14, R: 0x15, S: 0x16, T: 0x17, V: 0x19, W: 0x1a, X: 0x1b, Y: 0x1c, Z: 0x1d,
    N3: 0x20, N4: 0x21, N5: 0x22, Space: 0x2c, Slash: 0x38, Backquote: 0x35, BracketL: 0x2f, BracketR: 0x30,
    F5: 0x3e, F11: 0x44, PrintScreen: 0x46, Left: 0x50, Right: 0x4f, Up: 0x52, F13: 0x68,
  };

  // Each build(mac) returns the action list for that platform.
  const TEMPLATES = [
    {
      id: 'media', name: 'Media controls', description: 'Play/pause, skip and volume.',
      build: () => [media(0xcd, 'Play/Pause'), media(0xb6, 'Previous'), media(0xb5, 'Next'),
        media(0xe2, 'Mute'), media(0xea, 'Volume down'), media(0xe9, 'Volume up')],
    },
    {
      id: 'editing', name: 'Editing', description: 'Copy, paste, undo and friends.',
      build: (mac) => { const M = mac ? G : C; return [k(U.C, M, 'Copy'), k(U.V, M, 'Paste'), k(U.Z, M, 'Undo'),
        mac ? k(U.Z, G | S, 'Redo') : k(U.Y, C, 'Redo'), k(U.X, M, 'Cut'), k(U.A, M, 'Select all')]; },
    },
    {
      id: 'meetings', name: 'Meetings', description: 'Mute and camera for Zoom and Google Meet.', apps: ['zoom.us', 'Zoom'],
      build: (mac) => mac
        ? [k(U.A, G | S, 'Zoom mute'), k(U.V, G | S, 'Zoom video'), k(U.D, G, 'Meet mute'), k(U.E, G, 'Meet camera')]
        : [k(U.A, A, 'Zoom mute'), k(U.V, A, 'Zoom video'), k(U.D, C, 'Meet mute'), k(U.E, C, 'Meet camera')],
    },
    {
      id: 'stream', name: 'Streaming (F13+)', description: 'F13 to F18, which no app uses by default. Bind them as hotkeys in OBS.',
      build: () => [0, 1, 2, 3, 4, 5].map((i) => k(U.F13 + i, 0, `Scene ${i + 1}`)),
    },
    {
      id: 'screens', name: 'Screenshots', description: 'Whole screen, area and the capture toolbar.',
      build: (mac) => mac
        ? [k(U.N3, G | S, 'Full screen'), k(U.N4, G | S, 'Area'), k(U.N5, G | S, 'Capture menu')]
        : [k(U.PrintScreen, 0, 'Full screen'), k(U.S, G | S, 'Snip area')],
    },
    {
      id: 'browser', name: 'Browser', description: 'Tabs, history and reload.',
      build: (mac) => { const M = mac ? G : C; return [k(U.T, M, 'New tab'), k(U.W, M, 'Close tab'), k(U.T, M | S, 'Reopen tab'),
        mac ? k(U.BracketL, G, 'Back') : k(U.Left, A, 'Back'), mac ? k(U.BracketR, G, 'Forward') : k(U.Right, A, 'Forward'), k(U.R, M, 'Reload')]; },
    },
    {
      id: 'code', name: 'VS Code', description: 'Command palette, quick open, terminal and comments.', apps: ['Code', 'Visual Studio Code'],
      build: (mac) => { const M = mac ? G : C; return [k(U.P, M | S, 'Commands'), k(U.P, M, 'Quick open'), k(U.Backquote, C, 'Terminal'),
        k(U.Slash, M, 'Comment'), k(U.F5, 0, 'Run/debug')]; },
    },
    {
      id: 'system', name: 'System', description: 'Search, lock screen and overview.',
      build: (mac) => mac
        ? [k(U.Space, G, 'Spotlight'), k(U.Q, C | G, 'Lock screen'), k(U.Up, C, 'Mission Control')]
        : [k(0, G, 'Start menu'), k(U.L, G, 'Lock screen'), k(U.D, G, 'Show desktop')],
    },
  ];

  // Order Keybits are handed actions in.
  const SIDE_ORDER = { L: 0, T: 1, R: 2 };
  function orderUids(uids, sideOf) {
    return [...uids].sort((a, b) => (SIDE_ORDER[sideOf(a)] ?? 9) - (SIDE_ORDER[sideOf(b)] ?? 9) || a.localeCompare(b));
  }

  // Template -> { keys, apps, used, extra } for the given Keybits.
  function applyTemplate(t, uids, mac) {
    const actions = t.build(mac);
    const keys = {};
    uids.slice(0, actions.length).forEach((uid, i) => { keys[uid] = actions[i]; });
    return { keys, apps: t.apps || [], used: Math.min(uids.length, actions.length), total: actions.length };
  }

  const api = { TEMPLATES, applyTemplate, orderUids };
  if (typeof module !== 'undefined' && module.exports) module.exports = api;
  else root.Templates = api;
})(typeof window !== 'undefined' ? window : globalThis);
