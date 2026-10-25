# Keyword Monitor

A desktop app for the Keyword. It connects over USB serial or Bluetooth and lets you:

- see which sides of the Keyword have a Keybit, live
- assign any key or shortcut to each Keybit
- keep keymap profiles, and switch between, import and export them

It's built with Electron, so it runs on macOS, Windows and Linux. It uses Chromium's Web Serial and Web Bluetooth, so there are no native modules to compile.

## Run

```sh
cd app
npm install
npm start
```

**Demo** works without hardware. Click a side of the Keyword to attach or remove a Keybit, and click **Tap** to simulate a key press.

To run the tests:

```sh
npm test
```

They cover the protocol parser, the HID key table and profile import/export.

## Using it

- **USB serial / Bluetooth:** pick the Keyword from the list. ST devices (USB VID 0483) and BLE names starting with `Keyword` are highlighted.
- **Keys:** every Keybit the Keyword reports, with its side, its UID and what it's assigned to.
  - A key lights up while it's held.
  - Keys from the profile that aren't attached right now show greyed out.
- **Assign…:** press the key or shortcut you want, for example `Ctrl+Shift+K`, `Cmd+Space` or `F13`. Modifiers on their own work too.
  - For combos the OS grabs first, like Cmd+Tab, tick the modifiers and pick the key from the list instead.
  - You can also give the key a name.
- **Profiles:** edits save automatically to `profiles.json` in the app's user-data folder. Switching profiles sends the new keymap to the Keyword.
  - **Export…** writes a `.keyword.json` file and **Import…** reads one back.
- **Activity:** shows connects, disconnects and assignments as they happen. The raw protocol log is at the bottom.

## Protocol

[PROTOCOL.md](PROTOCOL.md) has the line protocol the Keyword firmware needs to speak, and the I2C address scheme used for left/right detection.

## Firmware status

The app is ready, but the Keyword firmware doesn't speak this protocol yet:

- USB CDC, the BLE Nordic UART Service, the I2C3 side scan and the keymap table still need adding on the Keyword.
- The Keybit side (assigned addresses, and the side register 0x07) lives in `firmware/keybit/`.

Until then, use **Demo**.

## Ways to detect a Keybit's side

The Keyword's three side joints share one I2C bus, so an ACK on its own can't tell you the side. These are the options:

1. **The Keybit reports which of its sides was addressed (used).** Each Keybit side has its own bus, so the Keybit knows which one a request came in on, and returns it in register 0x07.
   - A Keybit on the Keyword's left touches with its east side, so `E` means left, `W` means right and `S` means the top edge.
   - Two Keybits on the shared bus are kept apart by Keyword-assigned addresses, with UID arbitration for new ones.
   - It relies on Keybits being upright. The pogo layout forces that anyway, since a rotated Keybit swaps SDA and SCL.
2. **Side-encoded addresses.** The Keybit answers on a different address per side (for example E=0x32, W=0x34). This is simpler, but it leaves no room for addressing several Keybits individually.
3. **SWIO presence on PB5.** This tells you a Keybit is on the stud contacts, and it's needed for flashing. It can't say which side.
4. **A sense pin per side.** Use a pull-up on the Keyword and pull it to GND on the Keybit, or give each side its own I2C bus.
   - This is the most robust option and works for any rotation.
   - It needs a board revision.
