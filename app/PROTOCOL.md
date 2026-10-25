# Keyword host protocol (v1)

How the desktop app talks to the Keyword. The same protocol runs over USB and Bluetooth.

## Transport

- **USB:** CDC ACM (virtual serial port), USB ID `0483:5740` with the product string `Keyword`.
  - The baud rate is ignored.
  - Events are pushed once DTR is set or the host has sent a line.
- **Bluetooth LE:** Nordic UART Service.
  - The 128-bit UUID isn't in the advertising packet, so scan for names starting with `Keyword`.
  - Writes to RX need an encrypted, bonded link, so the host pairs first. The app pairs when it connects.
  - Service `6e400001-b5a3-f393-e0a9-e50e24dcca9e`
  - RX `6e400002-…` (the host writes here, in chunks of 20 bytes or less)
  - TX `6e400003-…` (the Keyword notifies here)
- **Framing:** ASCII lines ending in `\n`. `\r\n` and a bare `\r` are also accepted, so terminal programs work. Over BLE a line can be split across several notifications, so the host buffers until it sees `\n`.
- **Fields** are `name=value` pairs separated by spaces. Hex values are uppercase, and either case is accepted.

## Keys

Each Keybit is identified by its full 96-bit CH32V003 UID. The Keybit holds it at registers 0x10..0x1B, little-endian, and the protocol prints it as 24 uppercase hex digits, MSB first (for example `3B0A4F1C88D2E61F5F1821C1`). The app also accepts an 8-digit UID.

Profiles are keyed by UID and never by I2C address, because the Keyword can reassign an address.

`at` is the Keyword side the Keybit is on: `L` (left), `R` (right) or `T` (top edge).

Keybits relayed through other Keybits are reported as `KEY+ uid=<uid> at=X via=<uid> depth=<n>`:

- `via` is the next Keybit toward the Keyword, and `depth` is the number of hops.
- The relay only carries the low 32 bits of the UID. So `uid` is 8 hex digits unless the Keyword has seen that Keybit directly or holds a mapping for it.
- `MAP` with an 8-digit UID stores a low-32 mapping. A stored 24-digit mapping also matches a relayed Keybit with the same low 32 bits.
- The app maps an 8-digit UID onto a full UID it already knows. When it first sees a Keybit's full UID, it carries over any setup saved under the short one.

## Host → Keyword

| Command | Reply |
|---|---|
| `HELLO` | `KEYWORD v=<fw> proto=1` |
| `SIDES?` | `SIDES L=<0\|1> R=<0\|1> T=<0\|1>` |
| `KEYS?` | one `KEY+ uid=<uid> at=<L\|R\|T>` per present Keybit, then `KEYS END` |
| `MAP uid=<uid> mods=<2hex> key=<2hex>` | `OK MAP uid=<uid>` |
| `MAPC uid=<uid> usage=<4hex>` | `OK MAPC uid=<uid>` (media key) |
| `MAP? uid=<uid>` | `MAP uid=<uid> mods=<2hex> key=<2hex>` (`00`/`00` if unmapped), or `MAPC uid=<uid> usage=<4hex>` for a media key |
| `UNMAP uid=<uid>` | `OK UNMAP uid=<uid>` |
| `CLEAR` | `OK CLEAR` (drop every mapping, both kinds) |
| `SAVE` | `OK SAVE` (write the table to flash) |
| anything invalid | `ERR <text>` |

- `mods` is the HID boot keyboard modifier byte:
  - bit 0 = LCtrl, 1 = LShift, 2 = LAlt, 3 = LGUI (Cmd/Win)
  - bits 4 to 7 are the right-hand versions of the same keys
- `key` is a HID keyboard usage ID (page 0x07): `04` = A, `3A` = F1, `68` = F13.
- `key=00` with only `mods` set is a valid mapping.
- `usage` in `MAPC` is a Consumer page (0x0C) usage: `00CD` = play/pause, `00E9` = volume up, `00E2` = mute.
- Each Keybit has one mapping, so `MAPC` replaces a `MAP` for that UID and `MAP` replaces a `MAPC`. `UNMAP` clears either kind.
- When a mapped Keybit is pressed, the Keyword sends a keyboard report with `mods` and `key`, or a consumer report with `usage`.

## Keyword → host (unsolicited)

| Line | When |
|---|---|
| `SIDES L=.. R=.. T=..` | any side changes (after debounce) |
| `KEY+ uid=<uid> at=<L\|R\|T>` | a Keybit is attached |
| `KEY- uid=<uid>` | a Keybit is removed |
| `PRESS uid=<uid> down=<0\|1>` | a Keybit is pressed or released |

## Optional features (v1 extensions)

The Keyword lists the features it supports in `HELLO`'s `caps`. The app only sends a command when its capability is listed, and hides or greys out the matching UI when it isn't.

`KEYWORD v=<fw> proto=1 caps=<comma list> kb=<n>`

- `caps` can include `batt`, `hold`, `macro`, `flash` and `ota`. `ota` is listed only while the BLE stack runs on CPU2.
- `kb` is the version of the Keybit image the Keyword carries. A Keybit reporting a lower `v` is shown as outdated.
- `KEY+` may carry `v=<n>`, the Keybit firmware version (Keybit register 0x01).

| Cap | Host → Keyword | Reply / unsolicited |
|---|---|---|
| `batt` | `BATT?` | `BATT pct=<0-100> chg=<0\|1> usb=<0\|1>`, also pushed when pct moves by 5 or chg/usb changes |
| `hold` | `HOLD uid=<uid> mods=<2hex> key=<2hex> ms=<dec>` | `OK HOLD uid=<uid>` |
| `macro` | `MACRO uid=<uid> seq=<MMKK,MMKK,…>` | `OK MACRO uid=<uid>` |
| `flash` | `FLASH uid=<uid>` | `FLASH uid=<uid> pct=<n>` progress, then `OK FLASH uid=<uid>` or `ERR flash <reason>` |

| `ota` | `OTA` | `OK OTA`, then the Keyword restarts into the OTA bootloader after about 500 ms. `ERR no BLE stack on CPU2, use DFU` if it can't |
| (any) | `DFU` | `OK DFU`, then the Keyword restarts into the ROM USB DFU bootloader |
| (any) | `STACK?` | `STACK ble v=1.24.0 fus=2.2.0`, `STACK none fus=<x.y.z>` or `STACK unknown` |
| (any) | `BOOTCFG?` | `BOOTCFG nBOOT0=<0\|1> nSWBOOT0=<0\|1> nBOOT1=<0\|1> RDP=0x<2hex>` |
| (USB serial only) | `BOOTLOCK yes` | `OK BOOTLOCK, reloading option bytes`, then a reset that boots from flash only (BOOT0 ignored). `OK BOOTLOCK already`, or `ERR <reason>` before the app has run 10 s, without the BLE stack, or when RDP isn't level 0 |

How each one behaves:

- **Hold (tap/hold).**
  - If the key is released within `ms`, the normal `MAP`/`MAPC`/`MACRO` action fires on release.
  - Held longer than that, the hold action stays pressed until release.
  - `mods=00 key=00` clears the hold action, and so do `UNMAP` and `CLEAR`.
- **Macro.**
  - Each `MMKK` is one tap: modifier byte, then key usage. There are up to 16 steps.
  - A macro replaces `MAP`/`MAPC` for that UID, and `UNMAP` clears it.
- **`MAP?`** can answer with several lines for one key: a `MAP`, `MAPC` or `MACRO` line, followed by a `HOLD` line if a hold action is set.
- **Programming a single key,** the app sends `UNMAP`, then the key's commands, then `SAVE`. That way a stale macro or hold never survives an edit.

## Keyword firmware update (BLE OTA)

After `OK OTA` the Keyword erases its app (4-5 s) and restarts into ST's BLE_Ota bootloader. That works whichever link sent `OTA`, but the image itself always goes over Bluetooth.

- **Finding it:** it advertises as `Keyword OTA`, at the Keyword's BD address + 1, with ST manufacturer data (device ID 0x86). No pairing is needed.
- **GATT:** service `0000fe20-cc7a-482a-984a-7f2ed5b3e58f`
  - Base address `0000fe22-8e22-4541-9d4c-21edae82ed19`: write without response, 4 bytes `[cmd, offset>>16, offset>>8, offset]`, offset from 0x08000000. Start = `02 00 70 00`, finish = `07 00 00 00`, cancel = `08 00 00 00`.
  - Raw data `0000fe24-8e22-4541-9d4c-21edae82ed19`: write without response, up to MTU-3 bytes per write, appended in order.
  - Confirm `0000fe23-8e22-4541-9d4c-21edae82ed19`: indicate, 1 byte, `01` when the image is accepted.
- **Sequence:** connect, enable indications on fe23, write start, stream `firmware/release/keyword_app.bin` to fe24, write finish, wait for `01`. The Keyword then restarts into the new app. A bad image brings it back as `Keyword OTA`.
- **Pacing:** there's no flow control, so the host waits about 3 ms per 248 bytes, the bootloader's flash time.

How the app does it (Settings → **Update Keyword firmware…**, or File → Update Keyword Firmware…):

- It checks the file first: a Cortex-M vector table whose reset handler is inside 0x08007000 plus the image size. That rejects `keyword_full.bin`, which is for USB DFU.
- It sends writes of 240 bytes (176 on macOS, whose usual MTU is 185), stepping down to 176 and then 16 if the first write is refused. Every write is a multiple of 8, and the last one is padded with `FF`, because the bootloader programs flash 8 bytes at a time.
- If the Keyword reports no BLE stack, the app offers **Restart into USB DFU** instead. After DFU, flash with STM32CubeProgrammer and replug. Never use `-g`, `:leave`, `-startfus` or `-fwupgrade` over USB DFU: they rewrite nSWBOOT0 and can lock the Keyword out of DFU.
- With no Keyword connected, it goes straight to finding `Keyword OTA`, so it can recover a Keyword stuck in the bootloader.

## Computer actions (no protocol)

Some actions run on the computer rather than on the Keyword: open a URL, open an app, run a command, or switch profile.

- The app stores them only in its profile and never sends them to the Keyword.
- It reacts to `PRESS … down=1` for that UID, so these keys work only while Keyword Monitor is running.

## How the app uses it

- **On connect,** the app sends `HELLO`, `SIDES?` and `KEYS?`, waits for the `KEYWORD` reply (so it knows `caps`), then pushes the active profile: `CLEAR`, the commands for each key, then `SAVE`. Firmware can skip the flash write when the table is unchanged.
- **While connected,** it polls `SIDES?` every 2 s, `KEYS?` every 10 s and `BATT?` every 30 s (with `batt`), in case an unsolicited line was missed.
- **Profiles** live only in the app. The Keyword holds just the current table.

## Side detection (Keyword ↔ Keybit, I2C)

The full Keybit side is in `firmware/keybit/README.md`. In short:

- **One shared bus on the Keyword.** All three Keyword side joints sit on the same I2C bus (I2C3: left U5/U6, right U4/U7, top edge U13/U15).
- **Four separate buses on the Keybit.** Each Keybit has its own bit-banged bus per side.
- **Addressing.** A new Keybit answers on 0x30. The Keyword reads its UID, then assigns it an address from 0x08..0x77, which the Keybit keeps in flash. If several new Keybits answer at once, they arbitrate by UID.
- **Which side.** Register 0x07 returns the Keybit side the request arrived on, as `N`, `E`, `S` or `W`. The Keyword maps that to its own side:

| Keybit side (reg 0x07) | Keyword side |
|---|---|
| `E` | `L`: the Keybit is on the Keyword's left |
| `W` | `R`: right |
| `S` | `T`: top edge |

A Keybit can only mate upright, since rotating it would land its SDA on the Keyword's SCL. So the side it reports always maps to exactly one Keyword side.
