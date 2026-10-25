#include <stdio.h>
#include "app_common.h"
#include "main.h"
#include "shci.h"
#include "app_entry.h"
#include "ota.h"

/* The validity tag ST's BLE_Ota checks (CheckFwAppValidity): the word at
 * app + 0x140 holds the address of 0x94448A29, which the linker script
 * puts after the last byte loaded from flash. */
PLACE_IN_SECTION("TAG_OTA_END") const uint32_t MagicKeywordValue = 0x94448A29;
PLACE_IN_SECTION("TAG_OTA_START") const uint32_t MagicKeywordAddress = (uint32_t)&MagicKeywordValue;

/* Boot request in SRAM1, kept across a software reset. Must match
 * ota_bootloader/Core/Inc/app_conf.h; RAM1 starts at 0x20000008 in both
 * linker scripts so these 8 bytes are never touched otherwise. */
#define REBOOT_ON_BLE_OTA_APP  0x01
#define APP_FIRST_PAGE         7
#define REBOOT_VAL_MSG         (*(volatile uint8_t *)(SRAM1_BASE + 0))
#define START_SECTOR_VAL_MSG   (*(volatile uint8_t *)(SRAM1_BASE + 1))
#define NBR_OF_SECTOR_VAL_MSG  (*(volatile uint8_t *)(SRAM1_BASE + 2))
#define SYSDFU_REQ_WORD        (*(volatile uint32_t *)(SRAM1_BASE + 4))
#define SYSDFU_REQ_MAGIC       0x4446553FUL

#define RESET_DELAY_MS 500

static uint8_t reset_timer_id;
static uint8_t want_dfu;

static void Reset_Now(void)
{
  if (want_dfu)
  {
    SYSDFU_REQ_WORD = SYSDFU_REQ_MAGIC;
  }
  else
  {
    REBOOT_VAL_MSG = REBOOT_ON_BLE_OTA_APP;
    START_SECTOR_VAL_MSG = APP_FIRST_PAGE;
    NBR_OF_SECTOR_VAL_MSG = 0xFF;   /* the loader clamps this to below the keymap page */
  }
  NVIC_SystemReset();
}

static void Reset_Later(uint8_t dfu)
{
  want_dfu = dfu;
  if (!reset_timer_id)
  {
    HW_TS_Create(CFG_TIM_PROC_ID_ISR, &reset_timer_id, hw_ts_SingleShot, Reset_Now);
  }
  HW_TS_Start(reset_timer_id, MS_TO_TS_TICKS(RESET_DELAY_MS));
}

int OTA_Available(void)
{
  return APPE_Cpu2State() == CPU2_BLE;
}

void OTA_Request(void)
{
  Reset_Later(0);
}

void OTA_RequestDfu(void)
{
  Reset_Later(1);
}

void OTA_StackInfo(char *out, size_t n)
{
  WirelessFwInfo_t info;
  int state = APPE_Cpu2State();
  if (state == CPU2_UNKNOWN || SHCI_GetWirelessFwInfo(&info) != SHCI_Success)
  {
    snprintf(out, n, "unknown");
    return;
  }
  if (state == CPU2_BLE)
  {
    snprintf(out, n, "ble v=%u.%u.%u fus=%u.%u.%u", info.VersionMajor, info.VersionMinor, info.VersionSub,
             info.FusVersionMajor, info.FusVersionMinor, info.FusVersionSub);
  }
  else
  {
    snprintf(out, n, "none fus=%u.%u.%u", info.FusVersionMajor, info.FusVersionMinor, info.FusVersionSub);
  }
}
