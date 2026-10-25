# keyword firmware

STM32CubeMX project for the keyword main board (STM32WB55RGV6TR). Right now it
just cycles the status RGB LED (U14) through red, yellow, green, cyan, blue,
magenta, white and off, 500 ms per step. If clock or peripheral init fails it
blinks red fast instead (`Error_Handler`), which usually means a crystal
(32 MHz HSE or 32.768 kHz LSE) is not starting.

## LED wiring

U14 is an XL-1608RGBC, common anode to +3.3V, so each colour is active LOW.

| Colour | Net      | Pin  |
|--------|----------|------|
| Red    | STATUS_R | PD1  |
| Green  | STATUS_G | PD0  |
| Blue   | STATUS_B | PC12 |

## Build

Needs an `arm-none-eabi-gcc` with newlib and `make`. The Arm GNU Toolchain
(`brew install --cask gcc-arm-embedded`) or the xPack one that STM32 for VSCode
downloads both work. The bare `arm-none-eabi-gcc` brew formula doesn't (no
`stdint.h`). `.stm32env` holds the extension's tool paths, so override them on
the command line:

```sh
cd firmware
make -C ota_bootloader release ARM_GCC_PATH="$(dirname "$(which arm-none-eabi-gcc)")"
```

That builds the OTA bootloader (`ota_bootloader/`) and the app, and writes
`release/`:

| File | What | Where |
|------|------|-------|
| `keyword_full.bin` / `.hex` | bootloader + app in one image | 0x08000000, first install over USB DFU |
| `keyword_app.bin` | app only | 0x08007000, updates over BLE |

The app alone still builds with `make -f STM32Make.make` (`build/debug/keyword.elf`).

## Flash map

| Address | Pages | What |
|---------|-------|------|
| 0x08000000 | 0-6 | OTA bootloader, ST's BLE_Ota adapted (`ota_bootloader/`) |
| 0x08007000 | 7 up | the app |
| 0x080CF000 | SFSA - 1 | saved keymap, never erased by flashing or OTA |
| 0x080D0000 | SFSA up | CPU2: BLE stack and FUS |

On reset the bootloader jumps straight to the app if the app is valid (ST's
0x94448A29 tag at the end of the image, pointed to from app + 0x140). It only
stays and advertises **Keyword OTA** when the app is missing or broken, or when
the app asked for an update.

## Flash

### First install over USB DFU

> **Warning: over USB DFU, only ever use plain writes (`-d ... -v`) and reads.**
> STM32CubeProgrammer's "run after programming" steps rewrite the option bytes
> over USB: `-startfus` set `nSWBOOT0=0` on this board, which makes it boot
> from flash and ignore BOOT0 from then on. Never use `-g`/`--go`, `-s`, `-rst`,
> `-startfus`, `-fwupgrade`, `-startwirelessstack`, `-ob ...` or dfu-util's
> `:leave` over USB. Leave DFU by unplugging and plugging the keyword back in.

Two plain writes. Neither erases anything but the pages it covers, so the
keymap page and CPU2's area stay as they are:

```sh
# bootloader + app
STM32_Programmer_CLI -c port=usb1 -d release/keyword_full.bin 0x08000000 -v
# CPU2 BLE stack 1.24.0 (from STM32CubeWB, see below), only as data for now
STM32_Programmer_CLI -c port=usb1 -d stm32wb5x_BLE_Stack_full_fw.bin 0x080D0000 -v
```

`dfu-util -a 0 -d 0483:df11 -s 0x08000000 -D release/keyword_full.bin` works
too (no `:leave`). Then unplug and plug the keyword back in.

On that first boot the app comes up on USB serial first, then finds the stack
image at 0x080D0000 and has FUS install it, printing `STACK PROGRESS ...` lines
over USB serial. It resets once when it's done and comes back as the Bluetooth
keyboard **Keyword**. `FUS?` and `STACK?` show where it is.

### Updating over BLE

1. Send `OTA` from the desktop app or over USB serial. The keyword answers
   `OK OTA` and reboots into the bootloader, which erases the old app and
   advertises as **Keyword OTA**. `STACK?` shows whether the BLE stack is there;
   without it `OTA` answers an error instead.
2. Send `keyword_app.bin` to address 0x7000 with ST BLE Toolbox (or the desktop
   app once it does OTA).
3. When it's done the bootloader checks the tag and starts the new app.

If the transfer stops halfway the bootloader stays in OTA mode, so just send
the file again.

### Back to USB DFU without BOOT0

Send `DFU` over USB serial or BLE. The keyword answers `OK DFU`, resets, and the
bootloader starts ST's ROM USB DFU bootloader (system memory at 0x1FFF0000)
straight out of the reset, so it shows up as `0483:df11` without touching BOOT0
or the option bytes. The bootloader also does this by itself if CPU2 has no BLE
stack and the app is broken. A reset out of DFU boots the app again.

The bootloader also starts an 8 s watchdog before it runs the app, and the app
feeds it from its main loop. If the app hangs or crashes three times in a row
(before it has run 10 s), the bootloader goes to USB DFU instead, so a bad app
can't lock the keyword out.

### ST-Link over SWD

Still works if an ST-Link is wired to the SWD header (SWDIO, SWCLK, GND, 3V3,
NRST). The app now lives at 0x08007000 and needs the bootloader in front of it:

```sh
openocd -f openocd.cfg -c "program ota_bootloader/build/ota_bootloader.elf verify" \
  -c "program build/debug/keyword.elf verify reset exit"
```

### BOOT0 option bytes

BOOT0 (PH3) is floating on the keyword board, so the chip could randomly boot
into the system bootloader instead of the firmware. That's why its option bytes
were set to ignore the pin and always boot from flash (`nSWBOOT0 = 0`,
`nBOOT0 = 1`). Leave them like that: `DFU` above gets into the system
bootloader without them.

## Keybit dock and side bus

The keyword flashes Keybits itself. CN7/CN8/CN9 (3V3, SWIO, GND) on the
back line up with a Keybit's TP1/TP2/TP4. `Drivers/CH32V003` bit-bangs
WCH's one-wire debug protocol on PB5 (BB_SWIO) to detect a Keybit there,
halt it, and write `Core/Src/keybit_image.c` (built from `keybit/`) into its
flash. If the flash already matches, it just resets it.

On I2C3 (the side pogo pins) `Core/Src/keybit_bus.c` hands addresses to new
Keybits and lists the ones present, with UID, key state and which side they
are on. See `keybit/README.md` for the protocol.

Status LED: green idle, blue while a Keybit sits on the dock, blinking
white when a Keybit connects on a side, red if flashing the docked Keybit
failed.

## Bluetooth

The keyword shows up as a BLE keyboard called **Keyword**. It runs ST's
STM32_WPAN middleware from STM32CubeWB 1.24.0 (`Middlewares/`, `Utilities/`,
`STM32_WPAN/`) with HID over GATT (a keyboard report and a consumer control
report), Battery and Device Information. Pairing is Just Works with bonding,
so the host just connects and remembers it.

Each Keybit sends whatever its UID is mapped to in `Core/Src/keymap.c`, and
an unmapped Keybit sends nothing. The map is set with the line protocol in
`Core/Src/proto.c` (`HELLO`, `KEYS?`, `MAP`, `MAPC`, `SAVE` and so on) over
either the Nordic UART Service (6e400001-b5a3-f393-e0a9-e50e24dcca9e) or USB.
A Keybit can also be a media key (`MAPC`), type a short macro (`MACRO`, up to
16 taps), or do one thing on a tap and another while held (`HOLD`, like Escape
on tap and Ctrl on hold). Keybits further out, relayed through the ones
touching the keyword, work too. They only report the low 32 bits of their
UID, so they can be mapped with an 8 digit `uid=`. `BATT?` reads the
BQ25895's ADC for the battery level, which also goes to the BLE Battery
Service.
`SAVE` writes it to the flash page just under CPU2's area (0x080CF000 with the
full stack).

## USB serial

Plugged into USB-C the keyword is also a CDC serial port (VID 0x0483, PID
0x5740, "Keyword", it shows up as `/dev/tty.usbmodem*` on a Mac) talking the
same protocol. Each port gets its own replies, and KEY+/KEY-/PRESS/SIDES go to
every port that is open: BLE once TX notifications are on, USB once DTR is set
or a line comes in. USB runs off HSI48 trimmed by CRS against the host's
frames instead of PLLSAI1, set in `USB_Device/App/usb_device.c`.

```sh
screen /dev/tty.usbmodem* 115200   # then type HELLO and enter
```

The parser and the tap/hold/macro logic have host checks:

```sh
cc -std=c99 -Itest/stub -ICore/Inc test/proto_test.c Core/Src/proto.c Core/Src/keytable.c -o /tmp/proto_test && /tmp/proto_test
cc -std=c99 -ICore/Inc test/keyact_test.c Core/Src/keyact.c -o /tmp/keyact_test && /tmp/keyact_test
cc -std=c99 -Itest/stub -ICore/Inc test/batt_test.c Core/Src/batt.c -o /tmp/batt_test && /tmp/batt_test
```

### The CPU2 BLE stack

The radio runs on CPU2, which needs ST's BLE stack. The keyword installs it
itself, so it never needs `-startfus` or `-fwupgrade` (see the warning above):

1. Write `stm32wb5x_BLE_Stack_full_fw.bin` (1.24.0, 129460 bytes, SHA-256
   b7855d9ce0b012fabff5782a574a3038e6e5b1889c5c9bcdb06c2cab2201c05d) to
   0x080D0000 over DFU as above. It's in
   [STM32CubeWB](https://github.com/STMicroelectronics/STM32CubeWB) under
   `Projects/STM32WB_Copro_Wireless_Binaries/STM32WB5x`. It isn't in this repo:
   ST's SLA0044 doesn't allow it in an MIT licensed project.
2. When CPU2 boots with only FUS (no stack), the app checks 0x080D0000 for ST's
   image footer and asks FUS to install it (`SHCI_C2_FUS_FwUpgrade`, ST's
   CPU1-driven path). FUS moves SFSA down to 0xD0 itself. Over USB serial, `FUS?`
   shows `v=<fus> state=<idle|upgrade> stack=<version|none> sfsa=<page>
   image=<bytes>`, and `STACK INSTALL` starts it by hand.
3. Then the app resets and starts the stack. `STACK?` should answer
   `STACK ble v=1.24.0 fus=2.2.0`.

This board has FUS 2.2.0, which the 1.24.0 stack needs, so no FUS update.
Until the stack runs, `SAVE` is refused, since the keymap page moves with SFSA.

