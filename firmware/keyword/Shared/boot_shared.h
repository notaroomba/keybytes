/*
 * What the app and the OTA bootloader (ota_bootloader/) agree on. Both
 * build with -IShared, so this is the one copy.
 */
#ifndef BOOT_SHARED_H
#define BOOT_SHARED_H

#include <stdint.h>

/* ---- flash ---- */
#define BOOT_APP_FIRST_PAGE   7u                          /* app at 0x08007000 */
#define BOOT_APP_ADDRESS      (0x08000000u + BOOT_APP_FIRST_PAGE * 0x1000u)

/* ---- boot request in SRAM1, kept across a software reset ----
 * ST's BLE_Ota bytes at SRAM1 + 0..2, plus the ROM DFU word at + 4. RAM1
 * starts at 0x20000008 in both linker scripts so nothing else uses them. */
#define BOOT_REBOOT_ON_FW_APP       0x00u
#define BOOT_REBOOT_ON_BLE_OTA_APP  0x01u
#define BOOT_REBOOT_VAL_MSG     (*(volatile uint8_t *)(0x20000000u + 0))
#define BOOT_START_SECTOR_MSG   (*(volatile uint8_t *)(0x20000000u + 1))
#define BOOT_NBR_OF_SECTOR_MSG  (*(volatile uint8_t *)(0x20000000u + 2))
#define BOOT_SYSDFU_REQ_WORD    (*(volatile uint32_t *)(0x20000000u + 4))
#define BOOT_SYSDFU_REQ_MAGIC   0x4446553Fu                 /* "?UFD" */

/* ---- crash fallback, in RTC backup registers (kept across any reset but
 * a backup domain reset, which the app only does on a real pin reset) ----
 * The bootloader starts the IWDG before jumping to the app, and counts
 * watchdog resets in BOOT_CRASH_COUNT. At BOOT_CRASH_LIMIT in a row it
 * goes to ROM USB DFU instead of the app. The app feeds the IWDG from its
 * main loop and zeroes the count once it has run BOOT_HEALTHY_MS. While
 * BOOT_FUS_BUSY holds BOOT_FUS_BUSY_MAGIC (a CPU2 stack install in
 * progress, where a stuck SHCI call is expected) resets aren't counted. */
#define BOOT_CRASH_COUNT     (RTC->BKP19R)
#define BOOT_FUS_BUSY        (RTC->BKP18R)
#define BOOT_FUS_ATTEMPTS    (RTC->BKP17R)
#define BOOT_FUS_BUSY_MAGIC  0x46555342u                    /* "BSUF" */
#define BOOT_CRASH_LIMIT     3u
#define BOOT_HEALTHY_MS      10000u

/* IWDG: LSI ~32 kHz / 256 = 125 Hz, reload 1000 -> about 8 s. Longer than
 * any blocking path in the app (dock flash 1-2 s, keymap SAVE < 1 s). */
#define BOOT_IWDG_PR         6u
#define BOOT_IWDG_RLR        1000u

#endif /* BOOT_SHARED_H */
