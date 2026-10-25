#include <stdio.h>
#include "app_common.h"
#include "main.h"
#include "shci.h"
#include "app_entry.h"
#include "boot_shared.h"
#include "ota.h"

/* The validity tag ST's BLE_Ota checks (CheckFwAppValidity): the word at
 * app + 0x140 holds the address of 0x94448A29, which the linker script
 * puts after the last byte loaded from flash. */
PLACE_IN_SECTION("TAG_OTA_END") const uint32_t MagicKeywordValue = 0x94448A29;
PLACE_IN_SECTION("TAG_OTA_START") const uint32_t MagicKeywordAddress = (uint32_t)&MagicKeywordValue;

#define RESET_DELAY_MS 500

enum { RESET_PLAIN, RESET_OTA, RESET_DFU };

static uint8_t reset_timer_id;
static uint8_t reset_mode;
static uint8_t healthy;

static void Reset_Now(void)
{
  if (reset_mode == RESET_DFU)
  {
    BOOT_SYSDFU_REQ_WORD = BOOT_SYSDFU_REQ_MAGIC;
  }
  else if (reset_mode == RESET_OTA)
  {
    BOOT_REBOOT_VAL_MSG = BOOT_REBOOT_ON_BLE_OTA_APP;
    BOOT_START_SECTOR_MSG = BOOT_APP_FIRST_PAGE;
    BOOT_NBR_OF_SECTOR_MSG = 0xFF;   /* the loader clamps this to below the keymap page */
  }
  else
  {
    BOOT_REBOOT_VAL_MSG = BOOT_REBOOT_ON_FW_APP;
  }
  NVIC_SystemReset();
}

static void Reset_Later(uint8_t mode)
{
  reset_mode = mode;
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
  Reset_Later(RESET_OTA);
}

void OTA_RequestDfu(void)
{
  Reset_Later(RESET_DFU);
}

void OTA_ResetSoon(void)
{
  Reset_Later(RESET_PLAIN);
}

/* Every main loop pass: feed the IWDG the bootloader started (no effect
 * if it didn't), and after BOOT_HEALTHY_MS of running tell the bootloader
 * this app is fine by zeroing its crash count. */
void OTA_Service(void)
{
  IWDG->KR = 0xAAAA;
  if (!healthy && HAL_GetTick() >= BOOT_HEALTHY_MS)
  {
    healthy = 1;
    HAL_PWR_EnableBkUpAccess();
    BOOT_CRASH_COUNT = 0;
  }
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
