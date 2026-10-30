// Everything the UI shows lives in one zustand store. controller.ts changes it.
import { create } from 'zustand';
import type { PickerList } from '../../shared/bridge';
import { normalizeStore } from './lib/profiles.ts';
import { MAX_MACRO_STEPS } from './lib/protocol.ts';
import type { TransportKind } from './lib/transports.ts';
import type { Compass } from './lib/layout.ts';
import type { Side, Store } from './lib/types.ts';

export interface KeyState {
  at: Side | null;
  pressed: boolean;
  v: number | null;
  flashing: number | null;
  via: string | null;
  depth: number | null;
  // v11 Keybits say where they are: their own side facing the Keyword or the
  // Keybit they hang off (parent, null for one on the Keyword), and which
  // side of that Keybit they're on. Null until known.
  face: Compass | null;
  parent: string | null;
  pside: Compass | null;
  // Its own sides (N/E/S/W) where another Keybit is on backwards, '?' when only
  // known to be one of them, null when none.
  backwards: string | null;
}

export interface Battery {
  pct: number;
  charging: boolean;
  usb: boolean;
}

export interface DeviceInfo {
  fw: string | null;
  caps: string[];
  kb: number | null;
  macroSteps: number; // taps per macro the connected Keyword takes
  battery: Battery | null;
}

export type Tone = 'idle' | 'busy' | 'connected' | 'stale';
export type ActivityTone = 'add' | 'remove' | undefined;

export interface ActivityItem { id: number; time: string; text: string; tone?: ActivityTone }
export interface LogLine { id: number; time: string; dir: '->' | '<-' | '!!'; text: string }
export interface Toast { id: number; text: string; error?: boolean; undo?: boolean; leaving?: boolean }

export type OtaStage = 'pick' | 'find' | 'waiting' | 'sending' | 'done';
export interface OtaState {
  busy: boolean;
  stage: OtaStage;
  fileName: string;
  fileSize: number;
  status: string;
  error: boolean;
  pct: number | null;
  stack: string;
  showDfu: boolean;
  /** A Keyword already in its bootloader, picked from the normal Bluetooth chooser. */
  hasDevice: boolean;
}

export type DialogName = 'library' | 'settings' | 'ota' | null;

export interface PromptRequest {
  title: string;
  value: string;
  suggestions: string[];
  resolve: (v: string | null) => void;
}
export interface ConfirmRequest {
  title: string;
  body: string;
  ok: string;
  cancel: string;
  resolve: (ok: boolean) => void;
}

export interface History {
  canUndo: boolean;
  canRedo: boolean;
  undoLabel: string;
  redoLabel: string;
}

export interface AppState {
  store: Store;
  history: History;
  status: string;
  tone: Tone;
  transport: TransportKind | null;
  reconnecting: boolean;
  device: DeviceInfo;
  keys: Record<string, KeyState>;
  sides: Partial<Record<Side, boolean>>;
  frontApp: string | null;
  autoSwitched: boolean;
  seenApps: string[];
  activity: ActivityItem[];
  log: LogLine[];
  toasts: Toast[];
  highlight: string | null;
  chooser: PickerList | null;
  assignUid: string | null;
  dialog: DialogName;
  consoleOpen: boolean;
  prompt: PromptRequest | null;
  confirm: ConfirmRequest | null;
  ota: OtaState;
  filter: string;
  about: string;
  dropping: boolean;
}

export const emptyDevice = (): DeviceInfo => ({ fw: null, caps: [], kb: null, macroSteps: MAX_MACRO_STEPS, battery: null });

export const initialOta = (): OtaState => ({
  busy: false, stage: 'pick', fileName: '', fileSize: 0, status: '', error: false, pct: null, stack: '', showDfu: false, hasDevice: false,
});

export const useApp = create<AppState>(() => ({
  store: normalizeStore(null),
  history: { canUndo: false, canRedo: false, undoLabel: '', redoLabel: '' },
  status: 'Not connected',
  tone: 'idle',
  transport: null,
  reconnecting: false,
  device: emptyDevice(),
  keys: {},
  sides: {},
  frontApp: null,
  autoSwitched: false,
  seenApps: [],
  activity: [],
  log: [],
  toasts: [],
  highlight: null,
  chooser: null,
  assignUid: null,
  dialog: null,
  consoleOpen: false,
  prompt: null,
  confirm: null,
  ota: initialOta(),
  filter: '',
  about: '',
  dropping: false,
}));

export const activeProfileOf = (s: AppState) => s.store.profiles.find((p) => p.id === s.store.active) || s.store.profiles[0];
