/* The Keyword's USB device: serial port (CDC) plus keyboard (HID). */
#ifndef USBD_KW_H
#define USBD_KW_H

#include "usbd_def.h"

extern USBD_ClassTypeDef USBD_KW;

/* Enumerated by a host: keys go out here rather than over Bluetooth. */
int USBD_KW_Ready(USBD_HandleTypeDef *pdev);

/* Report a change; the same report twice sends nothing. */
void USBD_KW_Keyboard(USBD_HandleTypeDef *pdev, const uint8_t report[8]);
void USBD_KW_Media(USBD_HandleTypeDef *pdev, uint16_t usage);

#endif
