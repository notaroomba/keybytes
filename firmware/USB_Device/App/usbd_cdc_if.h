#ifndef USBD_CDC_IF_H
#define USBD_CDC_IF_H

#include "usbd_cdc.h"

extern USBD_CDC_ItfTypeDef USBD_Interface_fops_FS;

/* Bus reset, suspend or unplug: drop the session and anything queued. */
void CDC_Detached(void);

/* Registers the RX task and the protocol port, once at start up. */
void CDC_PortInit(void);

#endif /* USBD_CDC_IF_H */
