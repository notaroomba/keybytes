import { resolve } from 'node:path';
import { defineConfig } from 'electron-vite';
import react from '@vitejs/plugin-react';
import tailwindcss from '@tailwindcss/vite';
import type { Plugin } from 'vite';

// On macOS, an Electron started from a terminal is treated as part of the terminal for
// privacy checks, so the first Bluetooth scan is charged to Terminal / iTerm / VS Code.
// They have no Bluetooth usage string, and macOS kills the app. scripts/mac-electron.sh
// starts Electron through `open` instead, so Electron.app itself is responsible.
if (process.platform === 'darwin' && !process.env.KEYWORD_DIRECT_START && !process.env.ELECTRON_EXEC_PATH) {
  process.env.ELECTRON_EXEC_PATH = resolve(import.meta.dirname, 'scripts/mac-electron.sh');
}

// A strict Content Security Policy for the packaged page. Left out in development, where
// React's fast refresh needs an inline script.
const csp = (): Plugin => ({
  name: 'keyword-csp',
  apply: 'build',
  transformIndexHtml: () => [{
    tag: 'meta',
    attrs: { 'http-equiv': 'Content-Security-Policy', content: "default-src 'self'; script-src 'self'; style-src 'self' 'unsafe-inline'; img-src 'self' data:" },
    injectTo: 'head-prepend',
  }],
});

export default defineConfig({
  main: {
    build: { rollupOptions: { input: { index: resolve(import.meta.dirname, 'src/main/index.ts') } } },
  },
  preload: {
    build: {
      rollupOptions: {
        input: { index: resolve(import.meta.dirname, 'src/preload/index.ts') },
        // Sandboxed preloads must be CommonJS.
        output: { format: 'cjs', entryFileNames: '[name].cjs' },
      },
    },
  },
  renderer: {
    root: resolve(import.meta.dirname, 'src/renderer'),
    build: { minify: true, rollupOptions: { input: { index: resolve(import.meta.dirname, 'src/renderer/index.html') } } },
    plugins: [react(), tailwindcss(), csp()],
  },
});
