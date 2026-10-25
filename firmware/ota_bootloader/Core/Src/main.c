/*
 * Keyword OTA bootloader, based on ST's BLE_Ota (STM32CubeWB 1.24.0).
 *
 * The boot decision is made in SystemInit (system_stm32wbxx.c) before any
 * of this runs: a valid app is jumped to straight away, and only when the
 * app is missing, invalid or has asked for an update do we get here and
 * bring up BLE as "Keyword OTA".
 *
 * Only RCC, PWR, RTC, IPCC, HSEM and flash are touched. The status LED,
 * I2C3, SWIO and USB pins stay at their reset state.
 */
#include "main.h"
#include "app_entry.h"
#include "stm32_seq.h"

RTC_HandleTypeDef hrtc;

/* Same reset handling as the app's MX_APPE_Config: CPU2 may still hold
 * stale IPCC state after a pin reset. */
static void Reset_Device(void)
{
  /* A watchdog reset also sets PINRST; keep the backup domain (the crash
   * count lives there) unless it was a real pin reset. */
  if (LL_RCC_IsActiveFlag_PINRST() && !LL_RCC_IsActiveFlag_SFTRST()
      && !LL_RCC_IsActiveFlag_IWDGRST() && !LL_RCC_IsActiveFlag_WWDGRST())
  {
    HAL_PWR_EnableBkUpAccess();
    __HAL_RCC_BACKUPRESET_FORCE();
    __HAL_RCC_BACKUPRESET_RELEASE();
  }
  LL_AHB3_GRP1_EnableClock(LL_AHB3_GRP1_PERIPH_IPCC);
  uint32_t ch = LL_IPCC_CHANNEL_1 | LL_IPCC_CHANNEL_2 | LL_IPCC_CHANNEL_3
              | LL_IPCC_CHANNEL_4 | LL_IPCC_CHANNEL_5 | LL_IPCC_CHANNEL_6;
  LL_C1_IPCC_ClearFlag_CHx(IPCC, ch);
  LL_C2_IPCC_ClearFlag_CHx(IPCC, ch);
  LL_C1_IPCC_DisableTransmitChannel(IPCC, ch);
  LL_C2_IPCC_DisableTransmitChannel(IPCC, ch);
  LL_C1_IPCC_DisableReceiveChannel(IPCC, ch);
  LL_C2_IPCC_DisableReceiveChannel(IPCC, ch);
}

/* SystemInit (ST's) already runs SYSCLK from the 32 MHz HSE, trimmed from
 * OTP like the app. Here: LSE for the RTC, and RF wakeup on HSE/1024 as in
 * the app's PeriphCommonClock_Config. SMPS stays in bypass. */
static void SystemClock_Config(void)
{
  HAL_PWR_EnableBkUpAccess();
  LL_RCC_LSE_SetDriveCapability(LL_RCC_LSEDRIVE_MEDIUMHIGH);
  LL_RCC_LSE_Enable();
  while (!LL_RCC_LSE_IsReady());
  LL_RCC_SetRFWKPClockSource(LL_RCC_RFWKP_CLKSOURCE_HSE_DIV1024);
}

static void Init_Rtc(void)
{
  __HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_LSE);
  __HAL_RCC_RTC_ENABLE();
  __HAL_RCC_RTCAPB_CLK_ENABLE();
  HAL_NVIC_SetPriority(RTC_WKUP_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = CFG_RTC_ASYNCH_PRESCALER;
  hrtc.Init.SynchPrediv = CFG_RTC_SYNCH_PRESCALER;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  HAL_RTC_Init(&hrtc);
  LL_RTC_DisableWriteProtection(RTC);
  LL_RTC_WAKEUP_SetClock(RTC, CFG_RTC_WUCKSEL_DIVIDER);
  LL_RTC_EnableWriteProtection(RTC);
}

int main(void)
{
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);
  SystemCoreClockUpdate();   /* SystemInit switched to HSE */
  HAL_Init();
  Reset_Device();
  __HAL_RCC_CLEAR_RESET_FLAGS();   /* next boot decides on fresh flags */
  SystemClock_Config();
  /* only IPCC and HSEM may wake CPU1 */
  LL_EXTI_DisableIT_0_31(~0UL);
  LL_EXTI_DisableIT_32_63(~(LL_EXTI_LINE_36 | LL_EXTI_LINE_38));
  Init_Rtc();
  APPE_Init();

  while (1)
  {
    UTIL_SEQ_Run(UTIL_SEQ_DEFAULT);
  }
}

/* Asks SystemInit to start the ROM USB DFU bootloader on the next reset,
 * then resets. Entering it from a clean reset, before any clock or CPU2
 * is touched, needs no de-initialising and no option bytes. */
void BOOT_RequestSystemDfu(void)
{
  CFG_SYSDFU_REQ_WORD = CFG_SYSDFU_REQ_MAGIC;
  NVIC_SystemReset();
}

void BOOT_LeaveLoader(void)
{
  if (BOOT_AppValid())
  {
    CFG_OTA_REBOOT_VAL_MSG = CFG_REBOOT_ON_FW_APP;   /* SystemInit jumps to it */
    NVIC_SystemReset();
  }
  BOOT_RequestSystemDfu();
}

void Error_Handler(void)
{
  /* Can't run BLE: back to the app if there is one, else USB DFU. */
  BOOT_LeaveLoader();
}
