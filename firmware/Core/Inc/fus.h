/*
 * Installing the CPU2 BLE stack from the app, with CPU2 running only FUS.
 *
 * The stack binary is written as plain data over USB DFU to 0x080D0000
 * (ST's address for the 1.24.0 full stack on a 1 MB part). On boot, when
 * CPU2 reports FUS instead of the stack, the app finds the image (by ST's
 * footer) and asks FUS to install it (SHCI_C2_FUS_FwUpgrade), reporting
 * progress over USB CDC, then resets so CPU2 starts the new stack. That is
 * ST's CPU1-driven path from BLE_Ota / AN5185, with no option bytes from us
 * (FUS itself moves SFSA down to the stack).
 */
#ifndef FUS_H
#define FUS_H

#include <stddef.h>

/* From the CPU2 ready event (app_entry.c). With FUS running, a check
 * runs 3 s later (USB CDC is up by then) and installs a staged image. */
void FUS_OnCpu2Ready(int cpu2_state);

/* "v=2.2.0 state=idle stack=none sfsa=0xF4 image=129460" */
void FUS_Info(char *out, size_t n);

/* STACK INSTALL. NULL when started, else why not. */
const char *FUS_Install(void);

/* 1 while an install runs (SAVE and OTA are refused then). */
int FUS_Busy(void);

#endif /* FUS_H */
