/*
 * STM32_WPAN entry: CPU2 boot, transport layer, timer server and the
 * sequencer hook. Trimmed from ST's BLE_Hid app_entry.c (STM32CubeWB 1.24.0)
 * without the Nucleo LEDs, buttons and debug UART.
 *
 * main.c calls, all from USER CODE sections:
 *   MX_APPE_Config()  before SystemClock_Config (Init)
 *   MX_APPE_Init()    after the peripherals (RF_Init 2)
 *   MX_APPE_Process() once per pass of the main loop (WHILE)
 */
#include "app_common.h"
#include "main.h"
#include "app_entry.h"
#include "app_ble.h"
#include "ble.h"
#include "tl.h"
#include "stm32_seq.h"
#include "shci_tl.h"
#include "stm32_lpm.h"
#include "shci.h"
#include "otp.h"

RTC_HandleTypeDef hrtc;

#define POOL_SIZE (CFG_TLBLE_EVT_QUEUE_LENGTH * 4U * DIVC((sizeof(TL_PacketHeader_t) + TL_BLE_EVENT_FRAME_SIZE), 4U))

PLACE_IN_SECTION("MB_MEM2") ALIGN(4) static uint8_t EvtPool[POOL_SIZE];
PLACE_IN_SECTION("MB_MEM2") ALIGN(4) static TL_CmdPacket_t SystemCmdBuffer;
PLACE_IN_SECTION("MB_MEM2") ALIGN(4) static uint8_t SystemSpareEvtBuffer[sizeof(TL_PacketHeader_t) + TL_EVT_HDR_SIZE + 255U];
PLACE_IN_SECTION("MB_MEM2") ALIGN(4) static uint8_t BleSpareEvtBuffer[sizeof(TL_PacketHeader_t) + TL_EVT_HDR_SIZE + 255U];

static void APPE_SysUserEvtRx(void *pPayload);
static void APPE_SysStatusNot(SHCI_TL_CmdStatus_t status);

/* Runs before SystemClock_Config: the backup domain reset has to happen
 * before the LSE is started, and the HSE trim before the HSE is. */
void MX_APPE_Config(void)
{
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);

#if (CFG_HW_RESET_BY_FW == 1)
  /* After a pin reset (not a software one) CPU2 may still be running with
   * stale IPCC state, so start both clean. */
  if (LL_RCC_IsActiveFlag_PINRST() && !LL_RCC_IsActiveFlag_SFTRST())
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
#endif

  OTP_ID0_t *p_otp = (OTP_ID0_t *)OTP_Read(0);
  if (p_otp)
  {
    LL_RCC_HSE_SetCapacitorTuning(p_otp->hse_tuning);
  }
}

/* The timer server runs off the RTC wakeup timer, clocked from the LSE. */
static void Init_Rtc(void)
{
  RCC_PeriphCLKInitTypeDef clk = {0};
  clk.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  clk.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  if (HAL_RCCEx_PeriphCLKConfig(&clk) != HAL_OK)
  {
    Error_Handler();
  }
  __HAL_RCC_RTC_ENABLE();
  __HAL_RCC_RTCAPB_CLK_ENABLE();
  HAL_NVIC_SetPriority(RTC_WKUP_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);

  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = CFG_RTC_ASYNCH_PRESCALER;
  hrtc.Init.SynchPrediv = CFG_RTC_SYNCH_PRESCALER;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_RTCEx_EnableBypassShadow(&hrtc);

  LL_RTC_DisableWriteProtection(RTC);
  LL_RTC_WAKEUP_SetClock(RTC, CFG_RTC_WUCKSEL_DIVIDER);
  LL_RTC_EnableWriteProtection(RTC);
}

void MX_APPE_Init(void)
{
  __HAL_RCC_HSEM_CLK_ENABLE();

#if (CFG_USE_SMPS != 0)
  LL_PWR_SMPS_SetStartupCurrent(LL_PWR_SMPS_STARTUP_CURRENT_80MA);
  LL_PWR_SMPS_SetOutputVoltageLevel(LL_PWR_SMPS_OUTPUT_VOLTAGE_1V40);
  LL_PWR_SMPS_Enable();
#endif
  /* IPCC and HSEM wake CPU1 up */
  LL_EXTI_EnableIT_32_63(LL_EXTI_LINE_36 | LL_EXTI_LINE_38);
  Init_Rtc();

  LL_RCC_SetClkAfterWakeFromStop(LL_RCC_STOP_WAKEUPCLOCK_HSI);
  UTIL_LPM_Init();
  /* CPU2 may drop to shutdown whenever it wants, CPU1 decides the real mode. */
  LL_C2_PWR_SetPowerMode(LL_PWR_MODE_SHUTDOWN);

  HW_TS_Init(hw_ts_InitMode_Full, &hrtc);
  UTIL_LPM_SetOffMode(1 << CFG_LPM_APP, UTIL_LPM_DISABLE);

  /* Transport layer. CPU2 boots once TL_Enable() runs and answers with
   * SHCI_SUB_EVT_CODE_READY, which brings up the BLE app. */
  TL_MM_Config_t tl_mm_config;
  SHCI_TL_HciInitConf_t shci_conf;

  TL_Init();
  UTIL_SEQ_RegTask(1 << CFG_TASK_SYSTEM_HCI_ASYNCH_EVT_ID, UTIL_SEQ_RFU, shci_user_evt_proc);
  shci_conf.p_cmdbuffer = (uint8_t *)&SystemCmdBuffer;
  shci_conf.StatusNotCallBack = APPE_SysStatusNot;
  shci_init(APPE_SysUserEvtRx, (void *)&shci_conf);

  tl_mm_config.p_BleSpareEvtBuffer = BleSpareEvtBuffer;
  tl_mm_config.p_SystemSpareEvtBuffer = SystemSpareEvtBuffer;
  tl_mm_config.p_AsynchEvtPool = EvtPool;
  tl_mm_config.AsynchEvtPoolSize = POOL_SIZE;
  TL_MM_Init(&tl_mm_config);

  TL_Enable();
}

void MX_APPE_Process(void)
{
  UTIL_SEQ_Run(UTIL_SEQ_DEFAULT);
}

void UTIL_SEQ_Idle(void)
{
#if (CFG_LPM_SUPPORTED == 1)
  UTIL_LPM_EnterLowPower();
#endif
}

static void APPE_SysStatusNot(SHCI_TL_CmdStatus_t status)
{
  UNUSED(status);
}

static void APPE_SysUserEvtRx(void *pPayload)
{
  TL_AsynchEvt_t *p_sys_event = (TL_AsynchEvt_t *)(((tSHCI_UserEvtRxParam *)pPayload)->pckt->evtserial.evt.payload);
  if (p_sys_event->subevtcode != SHCI_SUB_EVT_CODE_READY) return;

  SHCI_C2_Ready_Evt_t *p_ready = (SHCI_C2_Ready_Evt_t *)p_sys_event->payload;
  if (p_ready->sysevt_ready_rsp == WIRELESS_FW_RUNNING)
  {
    SHCI_C2_CONFIG_Cmd_Param_t config_param = {0};
    config_param.PayloadCmdSize = SHCI_C2_CONFIG_PAYLOAD_CMD_SIZE;
    config_param.EvtMask1 = SHCI_C2_CONFIG_EVTMASK1_BIT0_ERROR_NOTIF_ENABLE
                          + SHCI_C2_CONFIG_EVTMASK1_BIT1_BLE_NVM_RAM_UPDATE_ENABLE;
    config_param.RevisionID = (uint16_t)LL_DBGMCU_GetRevisionID();
    config_param.DeviceID = (uint16_t)LL_DBGMCU_GetDeviceID();
    (void)SHCI_C2_Config(&config_param);

    APP_BLE_Init();
    UTIL_LPM_SetOffMode(1U << CFG_LPM_APP, UTIL_LPM_ENABLE);
  }
  else
  {
    /* FUS is running instead of the BLE stack: the stack binary is missing
     * or was never started. Keep the user event flow off, nothing to do. */
    ((tSHCI_UserEvtRxParam *)pPayload)->status = SHCI_TL_UserEventFlow_Disable;
  }
}

void shci_notify_asynch_evt(void *pdata)
{
  UNUSED(pdata);
  UTIL_SEQ_SetTask(1 << CFG_TASK_SYSTEM_HCI_ASYNCH_EVT_ID, CFG_SCH_PRIO_0);
}

void shci_cmd_resp_release(uint32_t flag)
{
  UNUSED(flag);
  UTIL_SEQ_SetEvt(1 << CFG_IDLEEVT_SYSTEM_HCI_CMD_EVT_RSP_ID);
}

void shci_cmd_resp_wait(uint32_t timeout)
{
  UNUSED(timeout);
  UTIL_SEQ_WaitEvt(1 << CFG_IDLEEVT_SYSTEM_HCI_CMD_EVT_RSP_ID);
}

/* These IRQs aren't enabled in keyword.ioc, so the handlers live here
 * rather than in the generated stm32wbxx_it.c. */
void IPCC_C1_RX_IRQHandler(void)
{
  HW_IPCC_Rx_Handler();
}

void IPCC_C1_TX_IRQHandler(void)
{
  HW_IPCC_Tx_Handler();
}

void RTC_WKUP_IRQHandler(void)
{
  HW_TS_RTC_Wakeup_Handler();
}
