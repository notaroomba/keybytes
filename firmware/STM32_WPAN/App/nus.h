/*
 * Nordic UART Service, the config channel for the desktop app. Bytes
 * written to RX go to PROTO_Input(); NUS_Write() queues bytes for TX and
 * sends them as 20 byte notifications.
 */
#ifndef NUS_H
#define NUS_H

#include <stdint.h>

void NUS_Init(void);
void NUS_Disconnected(void);
void NUS_Write(const char *data, uint16_t len);

#endif /* NUS_H */
