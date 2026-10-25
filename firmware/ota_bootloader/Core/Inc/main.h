#ifndef MAIN_H
#define MAIN_H

#include "stm32wbxx_hal.h"
#include "app_conf.h"

void Error_Handler(void);

/* Reset into ST's ROM USB DFU bootloader (system memory). See main.c. */
void BOOT_RequestSystemDfu(void);

/* OTA can't run: reset into the app if it's valid, else into ROM DFU. */
void BOOT_LeaveLoader(void);

/* system_stm32wbxx.c: 1 if the app at 0x08007000 carries a valid tag. */
uint8_t BOOT_AppValid(void);

#endif /* MAIN_H */
