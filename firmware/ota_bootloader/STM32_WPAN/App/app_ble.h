#ifndef APP_BLE_H
#define APP_BLE_H

#include "hci_tl.h"

void APP_BLE_Init(void);

/* First page OTA may write, and the first it may not (the keymap page). */
uint32_t APP_BLE_FirstAppPage(void);
uint32_t APP_BLE_EndAppPage(void);

#endif /* APP_BLE_H */
