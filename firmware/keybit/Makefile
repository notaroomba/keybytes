all : build

TARGET := main
TARGET_MCU ?= CH32V003
ADDITIONAL_C_FILES := soft_i2c.c link.c

# ch32v003fun is fetched on first build (pinned) unless you point CH32V003FUN
# at your own checkout.
CH32V003FUN ?= ch32v003fun
CH32V003FUN_REV := a800cbd1f1ca5e250c73da83c669f0965d2052e5
ifeq ($(wildcard $(CH32V003FUN)/ch32fun/ch32fun.mk),)
$(info Fetching ch32v003fun into $(CH32V003FUN))
$(shell git clone --quiet https://github.com/cnlohr/ch32v003fun.git $(CH32V003FUN) && git -C $(CH32V003FUN) checkout --quiet $(CH32V003FUN_REV))
endif
CH32FUN := $(CH32V003FUN)/ch32fun
include $(CH32FUN)/ch32fun.mk

# Rebuild the image the keyword embeds and flashes onto Keybits.
image : $(TARGET).bin
	python3 tools/bin2c.py $(TARGET).bin ../keyword/Core/Src/keybit_image.c main.c

flash : cv_flash
clean : cv_clean
