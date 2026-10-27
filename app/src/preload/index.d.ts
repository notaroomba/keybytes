import type { DesktopBridge, DevicePicker } from '../shared/bridge';

declare global {
  interface Window {
    desktop?: DesktopBridge;
    devicePicker?: DevicePicker;
  }
}
