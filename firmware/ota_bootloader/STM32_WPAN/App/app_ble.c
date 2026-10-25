/*
 * BLE side of the OTA bootloader, from ST's BLE_Ota app_ble.c: advertise,
 * expose ST's OTA service (otas_stm.c, driven by otas_app.c) and erase the
 * app area before a download.
 *
 * It advertises "Keyword OTA" on the chip's public address + 1 (as ST
 * does), so a host bonded to the keyboard's own address doesn't mistake
 * the loader for it. No pairing: the OTA characteristics are open, which
 * is how ST's tools expect them.
 */
#include "main.h"
#include "app_common.h"
#include "ble.h"
#include "tl.h"
#include "app_ble.h"
#include "stm32_seq.h"
#include "stm32_lpm.h"
#include "shci.h"
#include "otp.h"
#include "flash_driver.h"

PLACE_IN_SECTION("MB_MEM1") ALIGN(4) static TL_CmdPacket_t BleCmdBuffer;

static const uint8_t a_BLE_CfgIrValue[16] = CFG_BLE_IR;
static const uint8_t a_BLE_CfgErValue[16] = CFG_BLE_ER;
static uint16_t conn_handle;

static const uint8_t a_LocalName[] = {AD_TYPE_COMPLETE_LOCAL_NAME,
                                      'K', 'e', 'y', 'w', 'o', 'r', 'd', ' ', 'O', 'T', 'A'};
/* ST's BlueST layout, which ST BLE Toolbox uses to spot an OTA loader */
static uint8_t a_ManufData[14] = {sizeof(a_ManufData) - 1, AD_TYPE_MANUFACTURER_SPECIFIC_DATA,
                                  0x01 /* SDK version */, CFG_DEV_ID_OTA_FW_UPDATE,
                                  0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00 /* BD address, MSB first */};
static uint8_t a_BdAddr[6];

static void BLE_UserEvtRx(void *p_Payload);
static void BLE_StatusNot(HCI_TL_CmdStatus_t Status);

uint32_t APP_BLE_FirstAppPage(void)
{
  return CFG_APP_START_SECTOR_INDEX;
}

/* The app's keymap lives in the page just under SFSA (see the app's
 * keymap.c), and SFSA itself starts CPU2's area. ST's loader erased up to
 * SFSA; this stops one page earlier. */
uint32_t APP_BLE_EndAppPage(void)
{
  return ((FLASH->SFR & FLASH_SFR_SFSA) >> FLASH_SFR_SFSA_Pos) - 1;
}

/* Erase the pages the app asked for (start and count in SRAM1, set by the
 * app's OTA command or by SystemInit when there is no valid app), clamped
 * to [first app page, keymap page). Retries until every page is done: a
 * page left unerased would make a later write loop forever. */
static void Delete_Sectors(void)
{
  uint32_t start = CFG_OTA_START_SECTOR_IDX_VAL_MSG;
  uint32_t end = APP_BLE_EndAppPage();
  if (start < APP_BLE_FirstAppPage())
  {
    /* never erase the loader itself (ST's check let page 6 through) */
    CFG_OTA_REBOOT_VAL_MSG = CFG_REBOOT_ON_FW_APP;
    NVIC_SystemReset();
  }
  uint32_t count = CFG_OTA_NBR_OF_SECTOR_VAL_MSG;
  if (start >= end) return;
  if (start + count > end) count = end - start;

  uint32_t left = count;
  while (left)
  {
    left = FD_EraseSectors(start + (count - left), left);
  }
}

static const uint8_t *BleGetBdAddress(void)
{
  uint32_t udn = LL_FLASH_GetUDN();
  if (udn != 0xFFFFFFFF)
  {
    uint32_t company_id = LL_FLASH_GetSTCompanyID();
    a_BdAddr[0] = (uint8_t)(udn & 0xFF);
    a_BdAddr[1] = (uint8_t)((udn >> 8) & 0xFF);
    a_BdAddr[2] = (uint8_t)LL_FLASH_GetDeviceID();
    a_BdAddr[3] = (uint8_t)(company_id & 0xFF);
    a_BdAddr[4] = (uint8_t)((company_id >> 8) & 0xFF);
    a_BdAddr[5] = (uint8_t)((company_id >> 16) & 0xFF);
    return a_BdAddr;
  }
  uint8_t *p_otp = OTP_Read(0);
  if (p_otp) return ((OTP_ID0_t *)p_otp)->bd_address;
  for (int i = 0; i < 6; i++) a_BdAddr[i] = (uint8_t)(CFG_ADV_BD_ADDRESS >> (8 * i));
  return a_BdAddr;
}

static void Adv_Start(void)
{
  aci_gap_set_discoverable(ADV_IND, CFG_FAST_CONN_ADV_INTERVAL_MIN, CFG_FAST_CONN_ADV_INTERVAL_MAX,
                           CFG_BLE_ADDRESS_TYPE, NO_WHITE_LIST_USE,
                           sizeof(a_LocalName), a_LocalName, 0, 0, 0, 0);
  aci_gap_update_adv_data(sizeof(a_ManufData), a_ManufData);
}

void APP_BLE_Init(void)
{
  SHCI_C2_Ble_Init_Cmd_Packet_t ble_init_cmd_packet =
  {
    {{0, 0, 0}},
    {0,
     0,
     CFG_BLE_NUM_GATT_ATTRIBUTES,
     CFG_BLE_NUM_GATT_SERVICES,
     CFG_BLE_ATT_VALUE_ARRAY_SIZE,
     CFG_BLE_NUM_LINK,
     CFG_BLE_DATA_LENGTH_EXTENSION,
     CFG_BLE_PREPARE_WRITE_LIST_SIZE,
     CFG_BLE_MBLOCK_COUNT,
     CFG_BLE_MAX_ATT_MTU,
     CFG_BLE_PERIPHERAL_SCA,
     CFG_BLE_CENTRAL_SCA,
     CFG_BLE_LS_SOURCE,
     CFG_BLE_MAX_CONN_EVENT_LENGTH,
     CFG_BLE_HSE_STARTUP_TIME,
     CFG_BLE_VITERBI_MODE,
     CFG_BLE_OPTIONS,
     0,
     CFG_BLE_MAX_COC_INITIATOR_NBR,
     CFG_BLE_MIN_TX_POWER,
     CFG_BLE_MAX_TX_POWER,
     CFG_BLE_RX_MODEL_CONFIG,
     CFG_BLE_MAX_ADV_SET_NBR,
     CFG_BLE_MAX_ADV_DATA_LEN,
     CFG_BLE_TX_PATH_COMPENS,
     CFG_BLE_RX_PATH_COMPENS,
     CFG_BLE_CORE_VERSION,
     CFG_BLE_OPTIONS_EXT,
     CFG_BLE_MAX_ADD_EATT_BEARERS}
  };

  HCI_TL_HciInitConf_t hci_conf;
  hci_conf.p_cmdbuffer = (uint8_t *)&BleCmdBuffer;
  hci_conf.StatusNotCallBack = BLE_StatusNot;
  hci_init(BLE_UserEvtRx, (void *)&hci_conf);
  UTIL_LPM_SetOffMode(1 << CFG_LPM_APP_BLE, UTIL_LPM_DISABLE);
  UTIL_SEQ_RegTask(1 << CFG_TASK_HCI_ASYNCH_EVT_ID, UTIL_SEQ_RFU, hci_user_evt_proc);
  if (SHCI_C2_BLE_Init(&ble_init_cmd_packet) != SHCI_Success)
  {
    Error_Handler();
  }

  uint16_t gap_service_handle, gap_dev_name_char_handle, gap_appearance_char_handle;
  hci_reset();
  /* public address + 1, in the manufacturer data too (MSB first) */
  uint8_t addr[6];
  memcpy(addr, BleGetBdAddress(), 6);
  addr[0] += 1;
  aci_hal_write_config_data(CONFIG_DATA_PUBLIC_ADDRESS_OFFSET, CONFIG_DATA_PUBLIC_ADDRESS_LEN, addr);
  for (int i = 0; i < 6; i++) a_ManufData[sizeof(a_ManufData) - 1 - i] = addr[i];
  aci_hal_write_config_data(CONFIG_DATA_IR_OFFSET, CONFIG_DATA_IR_LEN, (uint8_t *)a_BLE_CfgIrValue);
  aci_hal_write_config_data(CONFIG_DATA_ER_OFFSET, CONFIG_DATA_ER_LEN, (uint8_t *)a_BLE_CfgErValue);
  aci_hal_set_tx_power_level(1, CFG_TX_POWER);
  aci_gatt_init();
  aci_gap_init(GAP_PERIPHERAL_ROLE, CFG_PRIVACY, sizeof(a_LocalName) - 1,
               &gap_service_handle, &gap_dev_name_char_handle, &gap_appearance_char_handle);
  aci_gatt_update_char_value(gap_service_handle, gap_dev_name_char_handle, 0,
                             sizeof(a_LocalName) - 1, (uint8_t *)&a_LocalName[1]);
  SVCCTL_Init();

  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);
  Delete_Sectors();
  Adv_Start();
}

/* Only ST's OTA service (the weak default inits every service). */
void SVCCTL_SvcInit(void)
{
  OTAS_STM_Init();
}

SVCCTL_UserEvtFlowStatus_t SVCCTL_App_Notification(void *p_Pckt)
{
  hci_event_pckt *p_event_pckt = (hci_event_pckt *)((hci_uart_pckt *)p_Pckt)->data;
  switch (p_event_pckt->evt)
  {
    case HCI_DISCONNECTION_COMPLETE_EVT_CODE:
      Adv_Start();
      break;
    case HCI_LE_META_EVT_CODE:
    {
      evt_le_meta_event *p_meta_evt = (evt_le_meta_event *)p_event_pckt->data;
      if (p_meta_evt->subevent == HCI_LE_CONNECTION_COMPLETE_SUBEVT_CODE)
      {
        conn_handle = ((hci_le_connection_complete_event_rp0 *)p_meta_evt->data)->Connection_Handle;
      }
      else if (p_meta_evt->subevent == HCI_LE_ENHANCED_CONNECTION_COMPLETE_SUBEVT_CODE)
      {
        conn_handle = ((hci_le_enhanced_connection_complete_event_rp0 *)p_meta_evt->data)->Connection_Handle;
      }
      break;
    }
    case HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE:
    {
      evt_blecore_aci *p_blecore_evt = (evt_blecore_aci *)p_event_pckt->data;
      if (p_blecore_evt->ecode == ACI_GATT_INDICATION_VSEVT_CODE) aci_gatt_confirm_indication(conn_handle);
      break;
    }
    default:
      break;
  }
  return SVCCTL_UserEvtFlowEnable;
}

void hci_notify_asynch_evt(void *p_Data)
{
  UNUSED(p_Data);
  UTIL_SEQ_SetTask(1 << CFG_TASK_HCI_ASYNCH_EVT_ID, CFG_SCH_PRIO_0);
}

void hci_cmd_resp_release(uint32_t Flag)
{
  UNUSED(Flag);
  UTIL_SEQ_SetEvt(1 << CFG_IDLEEVT_HCI_CMD_EVT_RSP_ID);
}

void hci_cmd_resp_wait(uint32_t Timeout)
{
  UNUSED(Timeout);
  UTIL_SEQ_WaitEvt(1 << CFG_IDLEEVT_HCI_CMD_EVT_RSP_ID);
}

static void BLE_UserEvtRx(void *p_Payload)
{
  tHCI_UserEvtRxParam *p_param = (tHCI_UserEvtRxParam *)p_Payload;
  p_param->status = SVCCTL_UserEvtRx((void *)&(p_param->pckt->evtserial)) != SVCCTL_UserEvtFlowDisable
                    ? HCI_TL_UserEventFlow_Enable : HCI_TL_UserEventFlow_Disable;
}

static void BLE_StatusNot(HCI_TL_CmdStatus_t Status)
{
  uint32_t task_id_list = (1 << CFG_LAST_TASK_ID_WITH_HCICMD) - 1;
  if (Status == HCI_TL_CmdBusy) UTIL_SEQ_PauseTask(task_id_list);
  else if (Status == HCI_TL_CmdAvailable) UTIL_SEQ_ResumeTask(task_id_list);
}

void SVCCTL_ResumeUserEventFlow(void)
{
  hci_resume_flow();
}
