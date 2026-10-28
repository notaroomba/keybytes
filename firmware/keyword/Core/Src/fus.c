#include <stdio.h>
#include "app_common.h"
#include "main.h"
#include "shci.h"
#include "stm32_seq.h"
#include "app_entry.h"
#include "boot_shared.h"
#include "ota.h"
#include "proto.h"
#include "stackimg.h"
#include "fus.h"

#define CHECK_DELAY_MS  3000    /* after the ready event: let USB CDC enumerate first */
#define POLL_MS         2000    /* ST's BLE_Ota polls every 10 s; 2 s for progress lines */
#define POLL_LIMIT      300     /* 10 min */

static uint8_t timer_id;
static uint8_t auto_check;      /* next task run is the boot-time check */
static uint8_t installing;
static uint8_t seen_upgrading;
static uint16_t polls;

static uint32_t Sfsa_Page(void)
{
  return (FLASH->SFR & FLASH_SFR_SFSA) >> FLASH_SFR_SFSA_Pos;
}

/* The staged image: from 0x080D0000 up to the page before SFSA (the one
 * below SFSA is the keymap page, kept clear of the image). */
static uint32_t Image_Len(void)
{
  uint32_t end = FLASH_BASE + (Sfsa_Page() - 1) * FLASH_PAGE_SIZE;
  if (end <= STACKIMG_ADDRESS) return 0;   /* stack installed: SFSA moved down to it */
  return STACKIMG_Find((const uint8_t *)STACKIMG_ADDRESS, end - STACKIMG_ADDRESS);
}

static int Upgrading(uint8_t st)
{
  return st >= FUS_STATE_UPGRADE_FIRST && st <= FUS_STATE_UPGRADE_LAST;
}

/* A wireless stack is installed when SFSA has moved down to (or below)
 * the stack's address: FUS moves it on install (0xF4 with FUS alone, 0xD0
 * with the 1.24.0 full stack). Going by SFSA rather than FUS's version
 * field, which isn't defined without a stack. */
static int Stack_Installed(void)
{
  return FLASH_BASE + Sfsa_Page() * FLASH_PAGE_SIZE <= STACKIMG_ADDRESS;
}

static void Schedule(uint32_t ms)
{
  HW_TS_Start(timer_id, MS_TO_TS_TICKS(ms));
}

static void Done(void)
{
  installing = 0;
  BOOT_FUS_BUSY = 0;
}

static void Start(void)
{
  BOOT_FUS_BUSY = BOOT_FUS_BUSY_MAGIC;   /* bootloader: don't count resets as crashes now */
  BOOT_FUS_ATTEMPTS = BOOT_FUS_ATTEMPTS + 1;
  PROTO_Send("STACK INSTALL start image=%lu", (unsigned long)Image_Len());
  if (SHCI_C2_FUS_FwUpgrade(0, 0) != SHCI_Success)   /* 0, 0: FUS finds the image itself */
  {
    PROTO_Send("ERR STACK FwUpgrade refused");
    Done();
    return;
  }
  installing = 1;
  seen_upgrading = 0;
  polls = 0;
  Schedule(POLL_MS);
}

static void Task(void)
{
  SHCI_FUS_GetState_ErrorCode_t err = 0;

  if (!installing)
  {
    if (!auto_check) return;
    auto_check = 0;
    uint8_t st = SHCI_C2_FUS_GetState(&err);
    if (Upgrading(st))
    {
      /* we came back mid-install (watchdog or reset): keep reporting */
      installing = 1;
      seen_upgrading = 1;
      polls = 0;
      Schedule(POLL_MS);
      return;
    }
    if (st == FUS_STATE_IDLE && Stack_Installed())
    {
      /* Installed but FUS still runs (as after an install): start the
       * stack like ST's BLE_Ota does. CPU2 reboots into it and its ready
       * event brings BLE up. */
      PROTO_Send("STACK starting");
      BOOT_FUS_BUSY = 0;
      SHCI_C2_FUS_StartWs();
      return;
    }
    if (STACKIMG_ShouldAutoInstall(1, st, Image_Len(), BOOT_FUS_ATTEMPTS)) Start();
    else BOOT_FUS_BUSY = 0;
    return;
  }

  uint8_t st = SHCI_C2_FUS_GetState(&err);
  PROTO_Send("STACK PROGRESS state=0x%02X err=0x%02X", st, (unsigned)err);
  if (Upgrading(st))
  {
    seen_upgrading = 1;
    if (++polls < POLL_LIMIT) Schedule(POLL_MS);
    else
    {
      PROTO_Send("ERR STACK timeout, reset the keyword");
      Done();
    }
    return;
  }

  if (st == FUS_STATE_IDLE && Stack_Installed())
  {
    /* ST's BLE_Ota resets here. On the way back up either CPU2 boots the
     * stack (ready event, BLE starts) or FUS is still running and the
     * boot-time check above starts the stack. */
    PROTO_Send("OK STACK installed, resetting");
    OTA_ResetSoon();
    return;
  }
  if (st == FUS_STATE_IDLE && !seen_upgrading && ++polls < 5)
  {
    Schedule(POLL_MS);   /* FUS may not have started yet */
    return;
  }
  PROTO_Send("ERR STACK state=0x%02X err=0x%02X", st, (unsigned)err);
  Done();
}

static void Tick(void)
{
  UTIL_SEQ_SetTask(1 << CFG_TASK_FUS_ID, CFG_SCH_PRIO_0);
}

void FUS_OnCpu2Ready(int cpu2_state)
{
  if (cpu2_state == CPU2_BLE)
  {
    BOOT_FUS_BUSY = 0;
    BOOT_FUS_ATTEMPTS = 0;
    return;
  }
  UTIL_SEQ_RegTask(1 << CFG_TASK_FUS_ID, UTIL_SEQ_RFU, Task);
  if (!timer_id) HW_TS_Create(CFG_TIM_PROC_ID_ISR, &timer_id, hw_ts_SingleShot, Tick);
  auto_check = 1;
  Schedule(CHECK_DELAY_MS);
}

int FUS_Busy(void)
{
  return installing;
}

const char *FUS_Install(void)
{
  if (APPE_Cpu2State() != CPU2_FUS) return "CPU2 is not running FUS";
  if (installing) return "already installing";
  if (!Image_Len()) return "no image at 0x080D0000";
  SHCI_FUS_GetState_ErrorCode_t err = 0;
  uint8_t st = SHCI_C2_FUS_GetState(&err);
  if (st != FUS_STATE_IDLE) return "FUS busy";
  BOOT_FUS_ATTEMPTS = 0;   /* a manual try starts the count over */
  Start();
  return NULL;
}

void FUS_Info(char *out, size_t n)
{
  WirelessFwInfo_t info;
  int cpu2 = APPE_Cpu2State();
  if (cpu2 == CPU2_UNKNOWN || SHCI_GetWirelessFwInfo(&info) != SHCI_Success)
  {
    snprintf(out, n, "v=unknown state=unknown stack=unknown sfsa=0x%02lX image=%lu",
             (unsigned long)Sfsa_Page(), (unsigned long)Image_Len());
    return;
  }
  char state[8];
  if (cpu2 == CPU2_BLE)
  {
    snprintf(state, sizeof(state), "stack");   /* asking FUS would stop the stack */
  }
  else
  {
    SHCI_FUS_GetState_ErrorCode_t err = 0;
    uint8_t st = installing ? FUS_STATE_UPGRADE_FIRST : SHCI_C2_FUS_GetState(&err);
    if (st == FUS_STATE_IDLE) snprintf(state, sizeof(state), "idle");
    else if (Upgrading(st)) snprintf(state, sizeof(state), "upgrade");
    else snprintf(state, sizeof(state), "0x%02X", st);
  }
  char stack[12] = "none";
  if (Stack_Installed())
  {
    snprintf(stack, sizeof(stack), "%u.%u.%u", info.VersionMajor, info.VersionMinor, info.VersionSub);
  }
  snprintf(out, n, "v=%u.%u.%u state=%s stack=%s sfsa=0x%02lX image=%lu",
           info.FusVersionMajor, info.FusVersionMinor, info.FusVersionSub, state, stack,
           (unsigned long)Sfsa_Page(), (unsigned long)Image_Len());
}
