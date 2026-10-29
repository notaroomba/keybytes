/*
 * The keyword's BLE app: one peripheral link carrying HID (keyboard +
 * consumer), Battery, Device Information and the NUS config channel.
 */
#ifndef APP_BLE_H
#define APP_BLE_H

#include <stdint.h>
#include "hci_tl.h"

#define APP_BLE_NO_CONN 0xFFFF

void APP_BLE_Init(void);

/* Handle of the current link, APP_BLE_NO_CONN when nobody is connected. */
uint16_t APP_BLE_ConnHandle(void);

/* Stop advertising for a flash page erase (1 if it did: nobody connected),
 * and start again. */
int APP_BLE_PauseAdv(void);
void APP_BLE_ResumeAdv(void);

/* Drop the Bluetooth link, if any (the computer reconnects by itself). */
void APP_BLE_Drop(void);

/* 1 once the CPU2 stack is up and the services exist. */
uint8_t APP_BLE_Ready(void);

/* Battery Service level, 0-100. Safe to call before BLE is up. */
void APP_BLE_SetBattery(uint8_t pct);

#endif /* APP_BLE_H */
