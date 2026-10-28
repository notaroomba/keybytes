/*
 * USB CDC serial (VID 0x0483, PID 0x5740, "Keyword") carrying the same
 * line protocol as NUS.
 */
#ifndef USB_DEVICE_H
#define USB_DEVICE_H

/* 48 MHz for USB from HSI48 trimmed by CRS against the host's SOFs.
 * Called from HAL_PCD_MspInit (USER CODE USB_MspInit 1). */
void USB_Clock_HSI48(void);

/* Starts the device stack. Call after MX_USB_PCD_Init and after the
 * sequencer and timer server are up (MX_APPE_Init). */
void MX_USB_Device_Init(void);

#endif /* USB_DEVICE_H */
