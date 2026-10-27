// The API the preload script exposes to the page as window.desktop and window.devicePicker.

export type HostActionType = 'url' | 'app' | 'cmd' | 'next-profile' | 'profile';
export interface HostAction {
  type: HostActionType;
  value: string;
}

export interface PickerDevice {
  id: string;
  name: string;
  detail: string;
  likely: boolean;
}
export interface PickerList {
  kind: 'serial' | 'bluetooth';
  devices: PickerDevice[];
}

export interface CapturedKey {
  type: 'keyDown' | 'keyUp' | string;
  code: string;
  ctrlKey: boolean;
  shiftKey: boolean;
  altKey: boolean;
  metaKey: boolean;
}

export interface TrayState {
  status: string;
  profiles: { id: string; name: string }[];
  active: string | null;
  battery: number | null;
}

export interface MainSettings {
  theme: 'system' | 'light' | 'dark';
  closeToTray: boolean;
  launchAtLogin: boolean;
  perApp: boolean;
}

export type MenuCommand =
  | 'undo' | 'redo' | 'settings' | 'import' | 'export' | 'export-all' | 'connect-usb' | 'connect-ble'
  | 'disconnect' | 'profile' | 'next-profile' | 'prev-profile' | 'new-profile' | 'library' | 'apply'
  | 'toggle-theme' | 'find' | 'update-firmware' | 'console';

export interface DesktopBridge {
  loadStore(): Promise<unknown>;
  saveStore(store: unknown): Promise<void>;
  saveFile(name: string, text: string): Promise<boolean>;
  openFiles(): Promise<{ name: string; text: string }[]>;
  setCapture(on: boolean): void;
  onCaptureKey(fn: (k: CapturedKey) => void): () => void;
  runHost(action: HostAction): Promise<{ ok: boolean; error?: string }>;
  appInfo(): Promise<{ version: string; platform: string }>;
  openDataFolder(): void;
  applySettings(s: MainSettings): void;
  updateState(s: TrayState): void;
  onMenu(fn: (cmd: MenuCommand, arg?: string) => void): () => void;
  onFrontApp(fn: (name: string) => void): () => void;
}

export interface DevicePicker {
  onList(fn: (list: PickerList) => void): () => void;
  pick(id: string): void;
}
