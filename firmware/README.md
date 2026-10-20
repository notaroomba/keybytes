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
