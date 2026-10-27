import type { HostAction } from '../../../shared/bridge';

export type { HostAction, HostActionType } from '../../../shared/bridge';

/** A side of the Keyword a Keybit can sit on: left, right or top edge. */
export type Side = 'L' | 'R' | 'T';

/** One tap in a macro: [modifier byte, HID key usage]. */
export type Step = [number, number];

export interface Hold {
  mods: number;
  key: number;
  ms: number;
}

/**
 * What one Keybit does. One main action (a key or shortcut, a media key, a macro or a
 * computer action), plus an optional label and hold action.
 */
export interface Entry {
  mods?: number;
  key?: number;
  consumer?: number;
  macro?: Step[];
  host?: HostAction;
  hold?: Hold;
  name?: string;
}

export interface Profile {
  id: string;
  name: string;
  apps: string[];
  keys: Record<string, Entry>;
}

export type Theme = 'system' | 'light' | 'dark';

export type LastConnection =
  | { kind: 'serial'; usbVendorId: number | null; usbProductId: number | null }
  | { kind: 'ble'; id: string; name: string };

export interface Settings {
  theme: Theme;
  autoReconnect: boolean;
  perApp: boolean;
  notifications: boolean;
  closeToTray: boolean;
  launchAtLogin: boolean;
  lastConnection: LastConnection | null;
}

export interface Store {
  active: string;
  profiles: Profile[];
  settings: Settings;
  stats: Record<string, number>;
}
