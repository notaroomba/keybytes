#include "app_common.h"
#include "main.h"
#include "shci.h"
#include "utilities_conf.h"
#include "ota.h"
#include "bootcfg.h"

void BOOTCFG_Info(char *out, size_t n)
{
  BOOTCFG_Format(FLASH->OPTR, out, n);
}

int BOOTCFG_Locked(void)
{
  return BOOTCFG_IsLocked(FLASH->OPTR);
}

/* OPTSTRT when neither CPU is blocking flash access, the same handshake
 * as keymap.c's Flash_Op (ST's BLE_RfWithFlash sequence). */
static HAL_StatusTypeDef Program(FLASH_OBProgramInitTypeDef *ob)
{
  for (int tries = 0; tries < 1000; tries++)
  {
    HAL_StatusTypeDef st = HAL_BUSY;
    while (LL_FLASH_IsActiveFlag_OperationSuspended());
    UTILS_ENTER_CRITICAL_SECTION();
    if (!LL_HSEM_GetStatus(HSEM, CFG_HW_BLOCK_FLASH_REQ_BY_CPU1_SEMID)
        && LL_HSEM_1StepLock(HSEM, CFG_HW_BLOCK_FLASH_REQ_BY_CPU2_SEMID) == 0)
    {
      st = HAL_FLASHEx_OBProgram(ob);
      LL_HSEM_ReleaseLock(HSEM, CFG_HW_BLOCK_FLASH_REQ_BY_CPU2_SEMID, 0);
    }
    UTILS_EXIT_CRITICAL_SECTION();
    if (st != HAL_BUSY) return st;
    HAL_Delay(1);   /* CPU2 is in a radio event, try again */
  }
  return HAL_TIMEOUT;
}

const char *BOOTCFG_Lock(void)
{
  uint32_t old = FLASH->OPTR;
  const char *why = BOOTCFG_Check(old, HAL_GetTick(), OTA_Available());
  if (why) return why;
  uint32_t want = BOOTCFG_LockValue(old);

  /* OPTIONBYTE_USER alone: HAL keeps the current RDP byte and every user
   * bit outside UserType (FLASH_OB_OptrConfig). */
  FLASH_OBProgramInitTypeDef ob = {
    .OptionType = OPTIONBYTE_USER,
    .UserType = OB_USER_nBOOT0 | OB_USER_nSWBOOT0,
    .UserConfig = want & (BOOTCFG_nBOOT0 | BOOTCFG_nSWBOOT0),
  };
  while (LL_HSEM_1StepLock(HSEM, CFG_HW_FLASH_SEMID));
  HAL_FLASH_Unlock();
  HAL_FLASH_OB_Unlock();
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);
  SHCI_C2_FLASH_EraseActivity(ERASE_ACTIVITY_ON);
  HAL_StatusTypeDef st = Program(&ob);
  SHCI_C2_FLASH_EraseActivity(ERASE_ACTIVITY_OFF);
  uint32_t now = FLASH->OPTR;
  LL_HSEM_ReleaseLock(HSEM, CFG_HW_FLASH_SEMID, 0);

  if (st != HAL_OK || now != want)
  {
    /* Not reloaded: the old option bytes stay in force until a power cycle */
    HAL_FLASH_OB_Lock();
    HAL_FLASH_Lock();
    return "option byte write failed, send BOOTCFG? before any reset";
  }
  OTA_ObLaunchSoon();   /* flash stays unlocked: OBL_LAUNCH needs OPTLOCK clear */
  return NULL;
}
