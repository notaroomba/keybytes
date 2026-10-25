const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('devicePicker', {
  onList: (fn) => ipcRenderer.on('chooser:list', (_e, data) => fn(data)),
  pick: (id) => ipcRenderer.send('chooser:pick', id || ''),
});

contextBridge.exposeInMainWorld('desktop', {
  loadStore: () => ipcRenderer.invoke('store:load'),
  saveStore: (store) => ipcRenderer.invoke('store:save', store),
  saveFile: (name, text) => ipcRenderer.invoke('file:save', name, text),
  openFiles: () => ipcRenderer.invoke('file:open'),
  setCapture: (on) => ipcRenderer.send('capture:set', on),
  onCaptureKey: (fn) => ipcRenderer.on('capture:key', (_e, k) => fn(k)),
  runHost: (action) => ipcRenderer.invoke('host:run', action),
  appInfo: () => ipcRenderer.invoke('app:info'),
  openDataFolder: () => ipcRenderer.send('open:data'),
  applySettings: (s) => ipcRenderer.send('settings:apply', s),
  updateState: (s) => ipcRenderer.send('state:update', s),
  onMenu: (fn) => ipcRenderer.on('menu', (_e, cmd, arg) => fn(cmd, arg)),
  onFrontApp: (fn) => ipcRenderer.on('app:front', (_e, name) => fn(name)),
});
