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

Needs `arm-none-eabi-gcc`, `make` and OpenOCD (`brew install --cask gcc-arm-embedded`
and `brew install open-ocd` on macOS). `.stm32env` holds Windows tool paths from
the STM32 for VSCode extension, so override them on the command line:

```sh
cd firmware
make -f STM32Make.make ARM_GCC_PATH="$(dirname "$(which arm-none-eabi-gcc)")" OPENOCD=openocd
```

The ELF lands in `build/debug/keyword.elf`.

## Flash (ST-Link over SWD)

Wire an ST-Link to the SWD header (SWDIO, SWCLK, GND, 3V3, NRST), then:

```sh
make -f STM32Make.make flash ARM_GCC_PATH="$(dirname "$(which arm-none-eabi-gcc)")" OPENOCD=openocd
# or directly:
openocd -f openocd.cfg -c "program build/debug/keyword.elf verify reset exit"
```

### First flash: BOOT0 option bytes

BOOT0 (PH3) is floating on the keyword board, so the chip may randomly boot into
the system bootloader instead of your code. Once per chip, tell it to ignore the
pin and always boot from flash (`nSWBOOT0 = 0`, `nBOOT0 = 1`) with
STM32CubeProgrammer:

```sh
STM32_Programmer_CLI -c port=SWD mode=UR -ob nSWBOOT0=0 nBOOT0=1
STM32_Programmer_CLI -c port=SWD -ob displ   # check: nSWBOOT0 = 0x0, nBOOT0 = 0x1
```

The same can be done in the STM32CubeProgrammer GUI under Option bytes > User
Configuration. This only needs doing once; it survives reflashing and mass erase.

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

### First flash: the CPU2 BLE stack

The radio runs on the second core (CPU2), which needs ST's BLE stack flashed
once with STM32CubeProgrammer 2.20 or newer. The binaries are in
[STM32CubeWB](https://github.com/STMicroelectronics/STM32CubeWB) under
`Projects/STM32WB_Copro_Wireless_Binaries/STM32WB5x`. The 1.24.0 stack needs
FUS v2, so check that first:

```sh
STM32_Programmer_CLI -c port=SWD -startfus
STM32_Programmer_CLI -c port=SWD mode=UR -r32 0x20030030 1   # 0x0202xx00 = FUS v2.2, good
```

If it reads `0x0102xx00` (FUS v1.2), update FUS first. If it is older than
v1.2, go through `stm32wb5x_FUS_fw_1_2_0.bin` at 0x080EC000 first.

```sh
STM32_Programmer_CLI -c port=SWD mode=UR -fwupgrade stm32wb5x_FUS_fw.bin 0x080EE000 firstinstall=0
```

Then the stack itself (addresses are for the 1 MB STM32WB55RG):

```sh
STM32_Programmer_CLI -c port=SWD mode=UR -fwupgrade stm32wb5x_BLE_Stack_full_fw.bin 0x080D0000 firstinstall=0
```

ST's notes end by setting nSWBOOT0 back to 1, skip that here or the floating
BOOT0 comes back. After this the app flashes as normal with the steps above.
If the stack is missing the app still runs, it just never advertises.
