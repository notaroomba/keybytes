/*
 * CPU2 boot and transport layer for the OTA bootloader, from ST's BLE_Ota
 * app_entry.c. The difference: when CPU2 comes up running FUS instead of
 * the BLE stack, ST's version tells FUS to install whatever it finds in
 * flash. Here there is no BLE stack to do an OTA with, so it goes back to
 * the app if the app is valid (the app installs the stack itself), or to
 * the ROM USB DFU bootloader if not. It never starts a FUS operation.
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

extern RTC_HandleTypeDef hrtc;

#define POOL_SIZE (CFG_TLBLE_EVT_QUEUE_LENGTH * 4U * DIVC((sizeof(TL_PacketHeader_t) + TL_BLE_EVENT_FRAME_SIZE), 4U))

PLACE_IN_SECTION("MB_MEM2") ALIGN(4) static uint8_t EvtPool[POOL_SIZE];
PLACE_IN_SECTION("MB_MEM2") ALIGN(4) static TL_CmdPacket_t SystemCmdBuffer;
PLACE_IN_SECTION("MB_MEM2") ALIGN(4) static uint8_t SystemSpareEvtBuffer[sizeof(TL_PacketHeader_t) + TL_EVT_HDR_SIZE + 255U];
PLACE_IN_SECTION("MB_MEM2") ALIGN(4) static uint8_t BleSpareEvtBuffer[sizeof(TL_PacketHeader_t) + TL_EVT_HDR_SIZE + 255U];

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
    SHCI_C2_SetFlashActivityControl(FLASH_ACTIVITY_CONTROL_SEM7);
    UTIL_LPM_SetOffMode(1 << CFG_LPM_APP, UTIL_LPM_ENABLE);
    APP_BLE_Init();
  }
  else
  {
    ((tSHCI_UserEvtRxParam *)pPayload)->status = SHCI_TL_UserEventFlow_Disable;
    BOOT_LeaveLoader();   /* no BLE stack, no OTA: app if valid, else USB DFU */
  }
}

void APPE_Init(void)
{
  __HAL_RCC_HSEM_CLK_ENABLE();
  LL_RCC_SetClkAfterWakeFromStop(LL_RCC_STOP_WAKEUPCLOCK_HSI);
  UTIL_LPM_Init();
  LL_C2_PWR_SetPowerMode(LL_PWR_MODE_SHUTDOWN);
  HW_TS_Init(hw_ts_InitMode_Full, &hrtc);
  UTIL_LPM_SetOffMode(1 << CFG_LPM_APP, UTIL_LPM_DISABLE);

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

void UTIL_SEQ_Idle(void)
{
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
