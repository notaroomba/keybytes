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

## Bluetooth

The keyword shows up as a BLE keyboard called **Keyword**. It runs ST's
STM32_WPAN middleware from STM32CubeWB 1.24.0 (`Middlewares/`, `Utilities/`,
`STM32_WPAN/`) with HID over GATT (a keyboard report and a consumer control
report), Battery and Device Information. Pairing is Just Works with bonding,
so the host just connects and remembers it.

Each Keybit sends whatever its UID is mapped to in `Core/Src/keymap.c`, and
an unmapped Keybit sends nothing. The map is set over the Nordic UART Service
(6e400001-b5a3-f393-e0a9-e50e24dcca9e) with the line protocol in
`Core/Src/proto.c` (`HELLO`, `KEYS?`, `MAP`, `SAVE` and so on). `SAVE` writes
it to the flash page just under CPU2's area (0x080CF000 with the full stack).

The parser has a host check:

```sh
cc -std=c99 -Itest/stub -ICore/Inc -ISTM32_WPAN/App test/proto_test.c Core/Src/proto.c -o /tmp/proto_test && /tmp/proto_test
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
