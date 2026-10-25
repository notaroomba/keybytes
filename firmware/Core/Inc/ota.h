/*
 * Hand-off to the OTA bootloader in ota_bootloader/ (pages 0-6) and to
 * ST's ROM USB DFU bootloader. Both go through a software reset with a
 * request left in SRAM1, which the bootloader reads in SystemInit.
 */
#ifndef OTA_H
#define OTA_H

#include <stddef.h>

/* CPU2 state as of its ready event. */
enum { CPU2_UNKNOWN = 0, CPU2_BLE = 1, CPU2_FUS = 2 };

/* 1 when the CPU2 BLE stack runs, which the OTA bootloader needs. */
int OTA_Available(void);

/* Reset into the bootloader's OTA mode ("Keyword OTA") in 500 ms, so the
 * reply that asked for it can still go out. It erases the app first. */
void OTA_Request(void);

/* Reset into the ROM USB DFU bootloader in 500 ms. No option bytes. */
void OTA_RequestDfu(void);

/* "ble v=1.24.0 fus=2.2.0", "none fus=2.2.0" or "unknown". */
void OTA_StackInfo(char *out, size_t n);

#endif /* OTA_H */
