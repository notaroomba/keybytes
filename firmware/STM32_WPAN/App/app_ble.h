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

#endif /* APP_BLE_H */
