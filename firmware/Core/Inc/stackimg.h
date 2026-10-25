/*
 * A CPU2 firmware image (BLE stack or FUS) written into user flash for FUS
 * to install. No HAL, so it runs in the host test (test/stackimg_test.c).
 *
 * ST's images end in a 20 byte footer, the same in stm32wb5x_FUS_fw.bin and
 * stm32wb5x_BLE_Stack_full_fw.bin (STM32CubeWB 1.24.0):
 *   0x12341234, 0xABCDABCD, 0xFFFF5740, <info word>, 0xD3A12C5E
 */
#ifndef STACKIMG_H
#define STACKIMG_H

#include <stdint.h>

#define STACKIMG_ADDRESS   0x080D0000u   /* ST's install address for the 1.24.0 full stack, 1 MB part */
#define STACKIMG_FOOTER_LEN 20u

/* FUS states (SHCI_C2_FUS_GetState) */
#define FUS_STATE_IDLE           0x00u
#define FUS_STATE_UPGRADE_FIRST  0x10u   /* firmware or FUS upgrade ongoing: 0x10..0x2F */
#define FUS_STATE_UPGRADE_LAST   0x2Fu
#define FUS_STATE_ERROR          0xFFu

/* Looks for the footer in data[0..len), word by word from the start (the
 * image begins at data[0]). Returns the image length (up to and including
 * the footer), or 0 when there is none or data[0] is erased flash. */
uint32_t STACKIMG_Find(const uint8_t *data, uint32_t len);

/* Start an install on our own? Only with CPU2 running FUS and idle, an
 * image staged, and fewer than 2 tries since the stack last ran. */
int STACKIMG_ShouldAutoInstall(int fus_running, uint8_t fus_state, uint32_t image_len, uint32_t attempts);

#endif /* STACKIMG_H */
