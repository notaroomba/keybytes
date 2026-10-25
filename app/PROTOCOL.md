# Keyword host protocol (v1)

How the desktop app talks to the Keyword. The same protocol runs over USB and Bluetooth.

## Transport

- **USB:** CDC ACM (virtual serial port). The baud rate is ignored.
- **Bluetooth LE:** Nordic UART Service, with the advertised name starting with `Keyword`.
  - Service `6e400001-b5a3-f393-e0a9-e50e24dcca9e`
  - RX `6e400002-…` (the host writes here, in chunks of 20 bytes or less)
  - TX `6e400003-…` (the Keyword notifies here)
- **Framing:** ASCII lines ending in `\n`, with `\r\n` also accepted. Over BLE a line can be split across several notifications, so the host buffers until it sees `\n`.
- **Fields** are `name=value` pairs separated by spaces. Hex values are uppercase, and either case is accepted.

## Keys

Each Keybit is identified by its full 96-bit CH32V003 UID. The Keybit holds it at registers 0x10..0x1B, little-endian, and the protocol prints it as 24 uppercase hex digits, MSB first (for example `3B0A4F1C88D2E61F5F1821C1`). The app also accepts an 8-digit UID.

Profiles are keyed by UID and never by I2C address, because the Keyword can reassign an address.

Only Keybits that touch the Keyword directly are visible. `at` is the Keyword side the Keybit is on: `L` (left), `R` (right) or `T` (top edge).

## Host → Keyword

| Command | Reply |
|---|---|
| `HELLO` | `KEYWORD v=<fw> proto=1` |
| `SIDES?` | `SIDES L=<0\|1> R=<0\|1> T=<0\|1>` |
| `KEYS?` | one `KEY+ uid=<uid> at=<L\|R\|T>` per present Keybit, then `KEYS END` |
| `MAP uid=<uid> mods=<2hex> key=<2hex>` | `OK MAP uid=<uid>` |
| `MAP? uid=<uid>` | `MAP uid=<uid> mods=<2hex> key=<2hex>` (`00`/`00` if unmapped) |
| `UNMAP uid=<uid>` | `OK UNMAP uid=<uid>` |
| `CLEAR` | `OK CLEAR` (drop every mapping) |
| `SAVE` | `OK SAVE` (write the table to flash) |
| anything invalid | `ERR <text>` |

- `mods` is the HID boot keyboard modifier byte:
  - bit 0 = LCtrl, 1 = LShift, 2 = LAlt, 3 = LGUI (Cmd/Win)
  - bits 4 to 7 are the right-hand versions of the same keys
- `key` is a HID keyboard usage ID (page 0x07): `04` = A, `3A` = F1, `68` = F13.
- `key=00` with only `mods` set is a valid mapping.
- When a mapped Keybit is pressed, the Keyword sends a HID report containing `mods` and `key`.

## Keyword → host (unsolicited)

| Line | When |
|---|---|
| `SIDES L=.. R=.. T=..` | any side changes (after debounce) |
| `KEY+ uid=<uid> at=<L\|R\|T>` | a Keybit is attached |
| `KEY- uid=<uid>` | a Keybit is removed |
| `PRESS uid=<uid> down=<0\|1>` | a Keybit is pressed or released |

## How the app uses it

- **On connect,** the app sends `HELLO`, `SIDES?` and `KEYS?`, then pushes the active profile: `CLEAR`, one `MAP` per key, then `SAVE`. Firmware can skip the flash write when the table is unchanged.
- **While connected,** it polls `SIDES?` every 2 s and `KEYS?` every 10 s, in case an unsolicited line was missed.
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
