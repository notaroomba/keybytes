import { contextBridge, ipcRenderer, type IpcRendererEvent } from 'electron';
import type { DesktopBridge, DevicePicker } from '../shared/bridge';

// Subscribes to a channel and returns a function that unsubscribes.
function on<T extends unknown[]>(channel: string, fn: (...args: T) => void): () => void {
  const listener = (_e: IpcRendererEvent, ...args: unknown[]) => fn(...(args as T));
  ipcRenderer.on(channel, listener);
  return () => ipcRenderer.off(channel, listener);
}

const devicePicker: DevicePicker = {
  onList: (fn) => on('chooser:list', fn),
  pick: (id) => ipcRenderer.send('chooser:pick', id || ''),
};

const desktop: DesktopBridge = {
  loadStore: () => ipcRenderer.invoke('store:load'),
  saveStore: (store) => ipcRenderer.invoke('store:save', store),
  saveFile: (name, text) => ipcRenderer.invoke('file:save', name, text),
  openFiles: () => ipcRenderer.invoke('file:open'),
  setCapture: (enabled) => ipcRenderer.send('capture:set', enabled),
  onCaptureKey: (fn) => on('capture:key', fn),
  runHost: (action) => ipcRenderer.invoke('host:run', action),
  appInfo: () => ipcRenderer.invoke('app:info'),
  openDataFolder: () => ipcRenderer.send('open:data'),
  applySettings: (s) => ipcRenderer.send('settings:apply', s),
  updateState: (s) => ipcRenderer.send('state:update', s),
  onMenu: (fn) => on('menu', fn),
  onFrontApp: (fn) => on('app:front', fn),
};

contextBridge.exposeInMainWorld('devicePicker', devicePicker);
contextBridge.exposeInMainWorld('desktop', desktop);
