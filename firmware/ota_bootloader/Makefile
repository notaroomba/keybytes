# Keyword OTA bootloader (pages 0-6, 0x08000000-0x08006FFF).
# Shares HAL, CMSIS, STM32_WPAN and utilities with the app in ../
#
#   make            build/ota_bootloader.{elf,bin,hex}
#   make release    also builds the app and writes ../release/:
#                     keyword_full.bin/.hex  bootloader + app, for the one-time DFU install
#                     keyword_app.bin        app only, for OTA at 0x08007000

TARGET = ota_bootloader
BUILD = build
FW = ..
PREFIX = arm-none-eabi-
ifdef ARM_GCC_PATH
PREFIX := $(ARM_GCC_PATH)/arm-none-eabi-
endif
CC = $(PREFIX)gcc
OBJCOPY = $(PREFIX)objcopy
SIZE = $(PREFIX)size

WPAN = $(FW)/Middlewares/ST/STM32_WPAN
HAL = $(FW)/Drivers/STM32WBxx_HAL_Driver

C_SOURCES = \
Core/Src/main.c \
Core/Src/app_entry.c \
Core/Src/system_stm32wbxx.c \
Core/Src/flash_driver.c \
Core/Src/hw_timerserver.c \
Core/Src/stm32_lpm_if.c \
Core/Src/stm32wbxx_it.c \
Core/Src/syscalls.c \
Core/Src/sysmem.c \
STM32_WPAN/App/app_ble.c \
STM32_WPAN/App/otas_app.c \
STM32_WPAN/Target/hw_ipcc.c \
$(HAL)/Src/stm32wbxx_hal.c \
$(HAL)/Src/stm32wbxx_hal_cortex.c \
$(HAL)/Src/stm32wbxx_hal_flash.c \
$(HAL)/Src/stm32wbxx_hal_flash_ex.c \
$(HAL)/Src/stm32wbxx_hal_pwr.c \
$(HAL)/Src/stm32wbxx_hal_pwr_ex.c \
$(HAL)/Src/stm32wbxx_hal_rcc.c \
$(HAL)/Src/stm32wbxx_hal_rcc_ex.c \
$(HAL)/Src/stm32wbxx_hal_rtc.c \
$(HAL)/Src/stm32wbxx_hal_rtc_ex.c \
$(WPAN)/ble/core/auto/ble_gap_aci.c \
$(WPAN)/ble/core/auto/ble_gatt_aci.c \
$(WPAN)/ble/core/auto/ble_hal_aci.c \
$(WPAN)/ble/core/auto/ble_hci_le.c \
$(WPAN)/ble/core/template/osal.c \
$(WPAN)/ble/svc/Src/svc_ctl.c \
$(WPAN)/ble/svc/Src/otas_stm.c \
$(WPAN)/interface/patterns/ble_thread/shci/shci.c \
$(WPAN)/interface/patterns/ble_thread/tl/hci_tl.c \
$(WPAN)/interface/patterns/ble_thread/tl/hci_tl_if.c \
$(WPAN)/interface/patterns/ble_thread/tl/shci_tl.c \
$(WPAN)/interface/patterns/ble_thread/tl/shci_tl_if.c \
$(WPAN)/interface/patterns/ble_thread/tl/tl_mbox.c \
$(WPAN)/utilities/otp.c \
$(WPAN)/utilities/stm_list.c \
$(WPAN)/utilities/stm_queue.c \
$(FW)/Utilities/lpm/tiny_lpm/stm32_lpm.c \
$(FW)/Utilities/sequencer/stm32_seq.c

ASM_SOURCES = startup_stm32wb55xx_cm4.s

C_INCLUDES = \
-ICore/Inc \
-I$(FW)/Shared \
-ISTM32_WPAN/App \
-I$(FW)/Drivers/CMSIS/Device/ST/STM32WBxx/Include \
-I$(FW)/Drivers/CMSIS/Include \
-I$(HAL)/Inc \
-I$(HAL)/Inc/Legacy \
-I$(WPAN) \
-I$(WPAN)/ble \
-I$(WPAN)/ble/core \
-I$(WPAN)/ble/core/auto \
-I$(WPAN)/ble/core/template \
-I$(WPAN)/ble/svc/Inc \
-I$(WPAN)/ble/svc/Src \
-I$(WPAN)/interface/patterns/ble_thread \
-I$(WPAN)/interface/patterns/ble_thread/shci \
-I$(WPAN)/interface/patterns/ble_thread/tl \
-I$(WPAN)/utilities \
-I$(FW)/Utilities/lpm/tiny_lpm \
-I$(FW)/Utilities/sequencer

MCU = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
C_DEFS = -DSTM32WB55xx -DUSE_HAL_DRIVER -DCORE_CM4
# Size first: the loader has to stay inside 28 KB
CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) -Os -g -Wall -fdata-sections -ffunction-sections -MMD -MP
LDFLAGS = $(MCU) -specs=nano.specs -Tota_bootloader.ld -lc -lm -lnosys \
          -Wl,-Map=$(BUILD)/$(TARGET).map,--cref -Wl,--gc-sections -Wl,--print-memory-usage

OBJECTS = $(addprefix $(BUILD)/,$(notdir $(C_SOURCES:.c=.o))) $(addprefix $(BUILD)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

all: $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).bin $(BUILD)/$(TARGET).hex

$(BUILD)/%.o: %.c Makefile | $(BUILD)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD)/%.o: %.s Makefile | $(BUILD)
	$(CC) -x assembler-with-cpp -c $(CFLAGS) $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJECTS) ota_bootloader.ld
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(BUILD)/%.bin: $(BUILD)/%.elf
	$(OBJCOPY) -O binary -S $< $@

$(BUILD)/%.hex: $(BUILD)/%.elf
	$(OBJCOPY) -O ihex $< $@

$(BUILD):
	mkdir -p $@

# ---- release images ----
# The app is built into its own untracked folder: the repo tracks old
# objects in firmware/build/debug, and in a fresh checkout make would take
# those as up to date and link stale code.
APP_BUILD = $(abspath $(BUILD))/app
APP_BIN = $(APP_BUILD)/debug/keyword.bin
APP_OFFSET = 0x7000
REL = $(FW)/release

app:
	$(MAKE) -C $(FW) -f STM32Make.make ARM_GCC_PATH="$(ARM_GCC_PATH)" OPENOCD=openocd BUILD_DIRECTORY=$(APP_BUILD)

release: all app
	mkdir -p $(REL)
	python3 tools/merge.py $(BUILD)/$(TARGET).bin $(APP_BIN) $(APP_OFFSET) $(REL)/keyword_full.bin
	$(OBJCOPY) -I binary -O ihex --change-addresses 0x08000000 $(REL)/keyword_full.bin $(REL)/keyword_full.hex
	cp $(APP_BIN) $(REL)/keyword_app.bin
	python3 tools/merge.py --check $(BUILD)/$(TARGET).bin $(APP_BIN) $(APP_OFFSET) $(REL)/keyword_full.bin

clean:
	rm -rf $(BUILD)

.PHONY: all app release clean

-include $(wildcard $(BUILD)/*.d)
