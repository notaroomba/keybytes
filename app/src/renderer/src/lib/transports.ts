// Ways to reach the Keyword. Each one carries the same line protocol.
import { NUS, OTA, SIDES, otaBaseCommand } from './protocol.ts';
import type { LastConnection, Side } from './types.ts';

export type TransportKind = 'serial' | 'ble' | 'demo';

export interface Transport {
  readonly label: string;
  readonly kind: TransportKind;
  open(onText: (text: string) => void, onClose: () => void): Promise<void>;
  write(text: string): Promise<void>;
  close(): Promise<void>;
  identity(): LastConnection | null;
}

/** Thrown when the picked Bluetooth device is a Keyword sitting in its update bootloader. */
export class OtaModeError extends Error {
  readonly device: BluetoothDevice;
  constructor(device: BluetoothDevice) {
    super('in update mode');
    this.name = 'OtaModeError';
    this.device = device;
  }
}

export class SerialTransport implements Transport {
  readonly label = 'USB serial';
  readonly kind = 'serial';
  private port: SerialPort | null;
  private writer: WritableStreamDefaultWriter<Uint8Array> | null = null;
  private reader: ReadableStreamDefaultReader<Uint8Array> | null = null;
  private closed = false;

  /** An already-granted port skips the picker. */
  constructor(port?: SerialPort | null) {
    this.port = port ?? null;
  }

  async open(onText: (text: string) => void, onClose: () => void) {
    const port = (this.port ??= await navigator.serial.requestPort());
    await port.open({ baudRate: 115200 }); // CDC ignores the baud rate
    // The Keyword only pushes events once DTR is set.
    try { await port.setSignals({ dataTerminalReady: true }); } catch { /* not every port has DTR */ }
    this.writer = port.writable!.getWriter();
    this.closed = false;
    void (async () => {
      const decoder = new TextDecoder();
      try {
        while (!this.closed && port.readable) {
          this.reader = port.readable.getReader();
          try {
            for (;;) {
              const { value, done } = await this.reader.read();
              if (done) break;
              onText(decoder.decode(value, { stream: true }));
            }
          } finally {
            this.reader.releaseLock();
          }
        }
      } catch (e) {
        console.warn('serial:', (e as Error).message);
      }
      if (!this.closed) onClose();
    })();
  }

  identity(): LastConnection {
    const i = this.port?.getInfo?.() ?? {};
    return { kind: 'serial', usbVendorId: i.usbVendorId ?? null, usbProductId: i.usbProductId ?? null };
  }

  async write(text: string) {
    await this.writer!.write(new TextEncoder().encode(text));
  }

  async close() {
    this.closed = true;
    try { await this.reader?.cancel(); } catch { /* already gone */ }
    try { this.writer?.releaseLock(); } catch { /* already gone */ }
    try { await this.port?.close(); } catch { /* already gone */ }
  }
}

export class BleTransport implements Transport {
  readonly label = 'Bluetooth';
  readonly kind = 'ble';
  device: BluetoothDevice | null;
  private rx: BluetoothRemoteGATTCharacteristic | null = null;
  private closed = false;
  private onDisconnect = () => {};

  constructor(device?: BluetoothDevice | null) {
    this.device = device ?? null;
  }

  async open(onText: (text: string) => void, onClose: () => void) {
    if (!this.device) {
      this.device = await navigator.bluetooth.requestDevice({
        // NUS's 128-bit UUID isn't advertised, so match on the name.
        filters: [{ namePrefix: 'Keyword' }],
        optionalServices: [NUS.service, OTA.service],
      });
      // Picked a Keyword sitting in its update bootloader: hand it to the update dialog.
      if (this.device.name === OTA.name) throw new OtaModeError(this.device);
    }
    const device = this.device;
    this.onDisconnect = () => { if (!this.closed) onClose(); };
    device.addEventListener('gattserverdisconnected', this.onDisconnect);
    const server = await device.gatt!.connect();
    const svc = await server.getPrimaryService(NUS.service);
    this.rx = await svc.getCharacteristic(NUS.rx);
    const tx = await svc.getCharacteristic(NUS.tx);
    const decoder = new TextDecoder();
    tx.addEventListener('characteristicvaluechanged', (e) => {
      const value = (e.target as BluetoothRemoteGATTCharacteristic).value!;
      onText(decoder.decode(value, { stream: true }));
    });
    await tx.startNotifications();
    this.closed = false;
  }

  identity(): LastConnection {
    return { kind: 'ble', id: this.device!.id, name: this.device!.name || '' };
  }

  async write(text: string) {
    const bytes = new TextEncoder().encode(text);
    for (let i = 0; i < bytes.length; i += 20) { // fits the default 23-byte ATT MTU
      const chunk = bytes.slice(i, i + 20);
      await this.rx!.writeValueWithoutResponse(chunk);
    }
  }

  async close() {
    this.closed = true;
    this.device?.removeEventListener('gattserverdisconnected', this.onDisconnect);
    if (this.device?.gatt?.connected) this.device.gatt.disconnect();
  }
}

/** Pretends to be a Keyword with every optional feature, so the app can be tried without hardware. */
export class DemoTransport implements Transport {
  readonly label = 'Demo';
  readonly kind = 'demo';
  readonly uids: Record<Side, string> = { L: '3B0A4F1C88D2E61F5F1821C1', R: '91C7002DE4B3A75C0A33B7E2', T: '0F5E19A2C6B84D37C4D90015' };
  /** Two Keybits chained out from the right one (2 and 3 hops), reported by their low 32 bits like a real relay. */
  readonly chained = 'D00DF00D';
  readonly chained2 = 'C0FFEE42';
  private versions: Record<Side, number> = { L: 3, R: 2, T: 3 };
  readonly present: Record<Side, boolean> = { L: true, R: false, T: false };
  private maps: Record<string, Record<string, string>> = {};
  private battery = 82;
  private readonly kb = 3;
  private onText: ((text: string) => void) | null = null;
  private battTimer: ReturnType<typeof setInterval> | undefined;

  async open(onText: (text: string) => void) {
    this.onText = onText;
    this.battTimer = setInterval(() => {
      if (this.battery > 5) {
        this.battery -= 1;
        this.emit(this.battLine());
      }
    }, 20000);
  }

  identity() { return null; }
  private emit(line: string) { setTimeout(() => this.onText?.(line + '\n'), 20); }
  private battLine() { return `BATT pct=${this.battery} chg=0 usb=0`; }
  private sidesLine() { return 'SIDES ' + SIDES.map((s) => `${s}=${this.present[s] ? 1 : 0}`).join(' '); }
  private keyLine(s: Side) { return `KEY+ uid=${this.uids[s]} at=${s} v=${this.versions[s]} face=${({ L: 'E', R: 'W', T: 'S' } as const)[s]}`; }
  private chainedLine() {
    // Like v11 firmware: each says which side of which Keybit it's on and which way it faces it.
    const r = this.uids.R.slice(-8);
    return `KEY+ uid=${this.chained} at=X via=${this.uids.R} depth=2 parent=${r} pside=N face=S\n`
      + `KEY+ uid=${this.chained2} at=X via=${this.uids.R} depth=3 parent=${this.chained} pside=E face=N`;
  }

  /** Attaches or removes the Keybit on a side. */
  toggle(side: Side) {
    this.present[side] = !this.present[side];
    this.emit(this.present[side] ? this.keyLine(side) : `KEY- uid=${this.uids[side]}`);
    if (side === 'R') this.emit(this.present.R ? this.chainedLine() : `KEY- uid=${this.chained}\nKEY- uid=${this.chained2}`);
    this.emit(this.sidesLine());
  }

  private sideOf(uid: string): Side | 'X' | undefined {
    return SIDES.find((s) => this.uids[s] === uid && this.present[s]) || ((uid === this.chained || uid === this.chained2) && this.present.R ? 'X' : undefined);
  }

  /** Simulates a press and release. */
  tap(uid: string) {
    if (!this.sideOf(uid)) return;
    this.emit(`PRESS uid=${uid} down=1`);
    setTimeout(() => this.emit(`PRESS uid=${uid} down=0`), 180);
  }

  private flash(uid: string) {
    const s = this.sideOf(uid);
    if (!s) return this.emit(`ERR flash ${uid} not present`);
    if (s === 'X') return this.emit('ERR flash only Keybits touching the Keyword can be reflashed');
    let pct = 0;
    const t = setInterval(() => {
      pct += 20;
      this.emit(`FLASH uid=${uid} pct=${pct}`);
      if (pct >= 100) {
        clearInterval(t);
        this.versions[s] = this.kb;
        this.emit(`OK FLASH uid=${uid}`);
        this.emit(this.keyLine(s));
      }
    }, 300);
  }

  async write(text: string) {
    for (const line of text.split('\n').map((s) => s.trim()).filter(Boolean)) {
      const [c, ...rest] = line.split(' ');
      const f = Object.fromEntries(rest.map((p) => p.split('=')));
      if (c === 'HELLO') this.emit(`KEYWORD v=demo proto=1 caps=batt,hold,macro,flash,ota kb=${this.kb}`);
      else if (c === 'SIDES?') this.emit(this.sidesLine());
      else if (c === 'BATT?') this.emit(this.battLine());
      else if (c === 'KEYS?') {
        for (const s of SIDES) if (this.present[s]) this.emit(this.keyLine(s));
        if (this.present.R) this.emit(this.chainedLine());
        this.emit('KEYS END');
      } else if (['MAP', 'MAPC', 'MACRO', 'HOLD'].includes(c)) {
        this.maps[f.uid] = f;
        this.emit(`OK ${c} uid=${f.uid}`);
      } else if (c === 'UNMAP') {
        delete this.maps[f.uid];
        this.emit(`OK UNMAP uid=${f.uid}`);
      } else if (c === 'CLEAR') {
        this.maps = {};
        this.emit('OK CLEAR');
      } else if (c === 'SAVE') this.emit('OK SAVE');
      else if (c === 'FLASH') this.flash(f.uid);
      else if (c === 'STACK?') this.emit('STACK ble v=1.24.0 fus=2.2.0');
      else if (c === 'OTA') this.emit('OK OTA');
      else if (c === 'DFU') this.emit('OK DFU');
      else if (c === 'DOCK?') this.emit('DOCK r=0 cfg=00000000 status=00000000 t=16 od=1 hclk=64000000 cyc10ms=640000');
      else this.emit(`ERR unknown command ${c}`);
    }
  }

  async close() {
    clearInterval(this.battTimer);
    this.onText = null;
  }
}

// ---- Keyword firmware update targets ----

export interface OtaTarget {
  connect(): Promise<void>;
  start(): Promise<void>;
  write(chunk: Uint8Array<ArrayBuffer>): Promise<void>;
  finish(): Promise<void>;
  cancel(): Promise<void>;
  close(): void;
  /** Resolves with the confirm byte (1 = image accepted). Valid after connect (BLE) or finish (demo). */
  confirmed: Promise<number>;
  /** Resolves if the link drops. */
  lost: Promise<void>;
}

/** ST's BLE_Ota bootloader, which the Keyword restarts into after OTA. */
export class BleOtaTarget implements OtaTarget {
  private device: BluetoothDevice | null;
  private base!: BluetoothRemoteGATTCharacteristic;
  private raw!: BluetoothRemoteGATTCharacteristic;
  confirmed: Promise<number> = new Promise(() => {});
  lost: Promise<void> = new Promise(() => {});

  constructor(device?: BluetoothDevice | null) {
    this.device = device ?? null;
  }

  async connect() {
    const device = (this.device ??= await navigator.bluetooth.requestDevice({
      filters: [{ name: OTA.name }],
      optionalServices: [OTA.service],
    }));
    const server = await device.gatt!.connect();
    const svc = await server.getPrimaryService(OTA.service);
    this.base = await svc.getCharacteristic(OTA.base);
    this.raw = await svc.getCharacteristic(OTA.raw);
    const confirm = await svc.getCharacteristic(OTA.confirm);
    this.confirmed = new Promise((resolve) => {
      confirm.addEventListener('characteristicvaluechanged', (e) =>
        resolve((e.target as BluetoothRemoteGATTCharacteristic).value!.getUint8(0)));
    });
    await confirm.startNotifications(); // indications, as far as Web Bluetooth is concerned
    this.lost = new Promise((resolve) => device.addEventListener('gattserverdisconnected', () => resolve(), { once: true }));
  }

  private writeBase(bytes: Uint8Array<ArrayBuffer>) {
    return this.base.writeValueWithoutResponse(bytes);
  }
  start() { return this.writeBase(otaBaseCommand(OTA.START, OTA.APP_OFFSET)); }
  write(chunk: Uint8Array<ArrayBuffer>) {
    return this.raw.writeValueWithoutResponse(chunk);
  }
  finish() { return this.writeBase(otaBaseCommand(OTA.FINISH)); }
  cancel() { return this.writeBase(otaBaseCommand(OTA.CANCEL)).catch(() => {}); }
  close() { if (this.device?.gatt?.connected) this.device.gatt.disconnect(); }
}

/** Stands in for the bootloader in Demo mode. */
export class DemoOtaTarget implements OtaTarget {
  private bytes = 0;
  confirmed: Promise<number> = new Promise(() => {});
  lost: Promise<void> = new Promise(() => {});
  async connect() { await sleep(400); }
  async start() { this.bytes = 0; }
  async write(chunk: Uint8Array<ArrayBuffer>) { this.bytes += chunk.length; }
  async finish() {
    await sleep(300);
    this.confirmed = Promise.resolve(this.bytes > 0 ? 1 : 0);
  }
  async cancel() {}
  close() {}
}

export const sleep = (ms: number) => new Promise<void>((r) => setTimeout(r, ms));
