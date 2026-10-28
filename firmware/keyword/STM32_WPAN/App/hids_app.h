/*
 * HID over GATT: report 1 is a boot-style 8 byte keyboard report
 * (modifiers, reserved, 6 keys), report 2 is one 16-bit consumer usage.
 */
#ifndef HIDS_APP_H
#define HIDS_APP_H

#include <stdint.h>

void HIDSAPP_Init(void);
void HIDSAPP_Disconnected(void);

/* Both send only when the value changed since the last send. */
void HIDSAPP_SendKeyboard(const uint8_t report[8]);
void HIDSAPP_SendConsumer(uint16_t usage);

#endif /* HIDS_APP_H */
