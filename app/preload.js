const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('devicePicker', {
  onList: (fn) => ipcRenderer.on('chooser:list', (_e, data) => fn(data)),
  pick: (id) => ipcRenderer.send('chooser:pick', id || ''),
});

contextBridge.exposeInMainWorld('desktop', {
  loadStore: () => ipcRenderer.invoke('store:load'),
  saveStore: (store) => ipcRenderer.invoke('store:save', store),
  exportProfile: (name, text) => ipcRenderer.invoke('profile:export', name, text),
  importProfile: () => ipcRenderer.invoke('profile:import'),
  setCapture: (on) => ipcRenderer.send('capture:set', on),
  onCaptureKey: (fn) => ipcRenderer.on('capture:key', (_e, k) => fn(k)),
});
