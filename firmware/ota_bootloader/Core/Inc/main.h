#ifndef MAIN_H
#define MAIN_H

#include "stm32wbxx_hal.h"
#include "app_conf.h"

void Error_Handler(void);

/* Reset into ST's ROM USB DFU bootloader (system memory). See main.c. */
void BOOT_RequestSystemDfu(void);

#endif /* MAIN_H */
