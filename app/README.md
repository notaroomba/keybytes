# Keyword Monitor

A desktop app for the Keyword. It connects over USB serial or Bluetooth and lets you:

- see which sides of the Keyword have a Keybit, live
- assign any key, shortcut, media key, macro or computer action to each Keybit, with an optional hold action
- keep keymap profiles, switch them automatically by app, and import or export them

It's built with Electron, so it runs on macOS, Windows and Linux. It uses Chromium's Web Serial and Web Bluetooth, so there are no native modules to compile.

## Run

```sh
cd app
npm install
npm start
```

On macOS, `npm start` launches Electron through `open`. macOS would otherwise blame the terminal for the Bluetooth scan and kill the app when you press **Bluetooth**. The app's output still shows in the terminal. Set `KEYWORD_DIRECT_START=1` to run Electron directly, for example under a debugger.

**Demo** works without hardware and pretends to be a Keyword with every optional feature (battery, hold, macros, reflashing):

- Click a side of the Keyword to attach or remove a Keybit.
- Click **Tap** on a key to simulate a press.

To build a standalone app, run `npm run dist`. It writes a `.dmg` on macOS, an installer on Windows or an AppImage on Linux, into `dist/`. The build includes the Bluetooth permission text that macOS requires.

Run the tests with `npm test`. They cover the protocol parser, the HID tables and text-to-macro, profiles, undo history and the template library.

## Connecting

- **Picking a device:** choose **USB serial** or **Bluetooth**, then the Keyword from the list.
  - ST devices (USB VID 0483) and BLE names starting with `Keyword` are highlighted.
  - Bluetooth pairs the first time, which the Keyword requires.
- **Auto-reconnect** is on by default.
  - If the link drops, the app keeps retrying with backoff.
  - A Keyword plugged back in over USB reconnects straight away.
  - On launch, the app reconnects to the last Keyword if its permission is still granted.
- **The status bar** shows the Keyword firmware version and, if the firmware reports it, the battery level. A low-battery warning pops up at 15%.

## Keys

- **The key list** shows every Keybit the Keyword reports: its side, its UID (click to copy), its Keybit firmware version and how often it's been pressed. A key lights up while it's held.
- **Keys you've set up that aren't attached** show greyed out. **Forget** removes them from the profile. **Move to…** hands their setup to a connected Keybit that has nothing assigned yet, which is handy after swapping a Keybit out.
- **Find a key** (`Cmd/Ctrl+F`) filters the list by name, action or UID.
- **Firmware updates:** a Keybit on older firmware than the Keyword carries gets an **Update** button, which reflashes it over SWIO, with progress shown.
- **Opening Assign:** use **Assign…**, double-click a key, or click it on the board diagram.

## Assigning

The Assign dialog has three tabs.

- **Key or shortcut:** click the box and press anything, for example `Ctrl+Shift+K`, `Cmd+Space`, `F13` or a media key.
  - The box catches combos that would normally close or quit the app.
  - You can also search ("volume", "page down", "f13") or tick modifiers and pick from the list.
- **Macro:** up to 16 taps in a row.
  - Record them by pressing keys, or type text and click **Add text**, which uses a US layout.
- **Computer action:** runs on this computer while the app is open, and the Keyword sends no key for it. You can:
  - open a website or link
  - open an app
  - run a shell command
  - switch to the next profile, or to a specific one
  - **Test** runs the action straight away.
- **When held:** an optional second action that fires when the key is held past a delay. A quick tap still sends the main action.
- **Name:** a label shown in the list and on the board.

Macros and hold actions need firmware support. If the connected Keyword doesn't list them in its capabilities, the app says so and doesn't send them.

## Profiles

- **Saving:** edits save automatically. Everything is undoable with `Cmd/Ctrl+Z` and redoable with `Cmd+Shift+Z` or `Ctrl+Y`, and deletes show an **Undo** toast.
- **Switching:** use the dropdown, `Cmd/Ctrl+1…9`, `Cmd/Ctrl+[` and `Cmd/Ctrl+]`, or the tray / menu bar icon.
- **Library…:** ready-made layouts, such as media controls, editing, meetings (Zoom/Meet), streaming (F13+), screenshots, browser, VS Code and system.
  - Each one is handed out to your Keybits in order: left, top edge, right.
  - Use it as a new profile, or apply it to the current one.
- **Per-app profiles:** turn on **Switch profiles by the app in front** in Settings, then use **+ Add app** on a profile.
  - That profile is used whenever that app is in front.
  - Leaving the app switches back to the profile you picked by hand.
  - On macOS this uses `lsappinfo` and needs no extra permission.
- **Import and export:** **Export…** saves one profile, and **Export all profiles** (in Settings or the File menu) saves them all to one file. **Import…** takes either kind, several files at once, or files dragged onto the window. If an imported profile would open apps or run commands, the app lists them and asks before keeping them.

## Settings

Settings (`Cmd/Ctrl+,`) has:

- theme: system, light or dark (`Cmd/Ctrl+Shift+L` toggles it)
- auto-reconnect
- per-app switching
- notifications when Keybits connect or disconnect
- keep running in the tray / menu bar when the window is closed
- open at login
- reset press counts, and open the data folder
- **Update Keyword firmware…**: pick `firmware/release/keyword_app.bin`, and the app restarts the Keyword into its Bluetooth bootloader and sends the image, with progress. It works when you're connected over USB too, since the image always goes over Bluetooth. If the Keyword has no Bluetooth stack, the app offers USB DFU instead. A Keyword stuck in update mode (showing as `Keyword OTA`) can be recovered from here without connecting first

The window remembers its size and position.

## Protocol

[PROTOCOL.md](PROTOCOL.md) has the line protocol the Keyword firmware needs to speak, and the I2C address scheme used for left/right detection.

## Firmware status

- **Done in the Keyword firmware:** BLE (HID keyboard, Nordic UART Service and keymap table) and USB CDC (`0483:5740`, "Keyword"). Both speak the same protocol, so either connection works.
- **Optional features** are switched on by the app when they appear in `caps`. The Keyword firmware now reports `batt`, `hold`, `macro` and `ota`, plus `kb=` for the Keybit image it carries. Reflashing Keybits from the app (`flash`) isn't reported yet.
- **The Keybit side** (assigned addresses, and the side register 0x07) lives in `firmware/keybit/`.

**Demo** works without any firmware.

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
