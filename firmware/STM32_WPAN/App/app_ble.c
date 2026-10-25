/*
 * BLE app for the keyword, based on ST's BLE_Hid app_ble.c (STM32CubeWB
 * 1.24.0). GAP peripheral named "Keyword" with the HID keyboard
 * appearance, Just Works pairing with bonding, and the services in
 * SVCCTL_SvcInit below.
 */
#include "main.h"
#include "app_common.h"
#include "ble.h"
#include "tl.h"
#include "app_ble.h"
#include "hids_app.h"
#include "nus.h"
#include "keymap.h"
#include "proto.h"
#include "stm32_seq.h"
#include "shci.h"
#include "otp.h"
#include "stm32_lpm.h"

#define DEVICE_NAME      "Keyword"
#define MANUFACTURER     "keybytes"

#define FAST_ADV_TIMEOUT (30 * 1000 * 1000 / CFG_TS_TICK_VAL) /* 30 s, then slow adv */

PLACE_IN_SECTION("MB_MEM1") ALIGN(4) static TL_CmdPacket_t BleCmdBuffer;

static const uint8_t a_BLE_CfgIrValue[16] = CFG_BLE_IR;
static const uint8_t a_BLE_CfgErValue[16] = CFG_BLE_ER;

static uint16_t conn_handle = APP_BLE_NO_CONN;
static uint8_t adv_timer_id;
static uint8_t a_BdAddr[6];

/* Scan response would need an extra round trip, so it all goes in the
 * advertising packet: flags (added by the stack) + name + services +
 * appearance = 22 of 31 bytes. NUS's 128-bit UUID doesn't fit; the
 * desktop app finds the keyword by its name. */
static const uint8_t a_LocalName[] = {AD_TYPE_COMPLETE_LOCAL_NAME, 'K', 'e', 'y', 'w', 'o', 'r', 'd'};
static const uint8_t a_ServUuids[] = {AD_TYPE_16_BIT_SERV_UUID,
                                      (uint8_t)HUMAN_INTERFACE_DEVICE_SERVICE_UUID, (uint8_t)(HUMAN_INTERFACE_DEVICE_SERVICE_UUID >> 8),
                                      (uint8_t)BATTERY_SERVICE_UUID, (uint8_t)(BATTERY_SERVICE_UUID >> 8)};
static const uint8_t a_Appearance[] = {3, AD_TYPE_APPEARANCE,
                                       (uint8_t)BLE_CFG_GAP_APPEARANCE, (uint8_t)(BLE_CFG_GAP_APPEARANCE >> 8)};

static void BLE_UserEvtRx(void *p_Payload);
static void BLE_StatusNot(HCI_TL_CmdStatus_t Status);
static void Gap_Gatt_Init(void);
static void Adv_Start(uint8_t fast);

uint16_t APP_BLE_ConnHandle(void)
{
  return conn_handle;
}

static void Adv_Slow(void)
{
  if (conn_handle == APP_BLE_NO_CONN) Adv_Start(0);
}

static void Adv_Timeout(void)
{
  UTIL_SEQ_SetTask(1 << CFG_TASK_ADV_UPDATE_ID, CFG_SCH_PRIO_0);
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

  /* Keymap SAVE writes flash while CPU2 is running: make CPU2 use
   * semaphore 7 to guard its radio timing instead of the PES bit. */
  SHCI_C2_SetFlashActivityControl(FLASH_ACTIVITY_CONTROL_SEM7);

  Gap_Gatt_Init();
  SVCCTL_Init();

  UTIL_SEQ_RegTask(1 << CFG_TASK_ADV_UPDATE_ID, UTIL_SEQ_RFU, Adv_Slow);
  HW_TS_Create(CFG_TIM_PROC_ID_ISR, &adv_timer_id, hw_ts_SingleShot, Adv_Timeout);

  DIS_Data_t dis;
  dis.pPayload = (uint8_t *)MANUFACTURER;
  dis.Length = sizeof(MANUFACTURER) - 1;
  DIS_UpdateChar(MANUFACTURER_NAME_UUID, &dis);
  dis.pPayload = (uint8_t *)DEVICE_NAME;
  dis.Length = sizeof(DEVICE_NAME) - 1;
  DIS_UpdateChar(MODEL_NUMBER_UUID, &dis);
  dis.pPayload = (uint8_t *)KEYWORD_FW_VERSION;
  dis.Length = sizeof(KEYWORD_FW_VERSION) - 1;
  DIS_UpdateChar(FIRMWARE_REVISION_UUID, &dis);

  /* ponytail: fixed 100 %, the board has no battery gauge wired to the MCU yet */
  uint8_t level = 100;
  BAS_Update_Char(BATTERY_LEVEL_CHAR_UUID, 0, &level);

  HIDSAPP_Init();
  KEYMAP_Init();

  Adv_Start(1);
}

/* Only the services the keyword uses (the weak default inits every service
 * in the middleware). */
void SVCCTL_SvcInit(void)
{
  DIS_Init();
  BAS_Init();
  HIDS_Init();
  NUS_Init();
}

void BAS_Notification(BAS_Notification_evt_t *pNotification)
{
  UNUSED(pNotification);
}

SVCCTL_UserEvtFlowStatus_t SVCCTL_App_Notification(void *p_Pckt)
{
  hci_event_pckt *p_event_pckt = (hci_event_pckt *)((hci_uart_pckt *)p_Pckt)->data;

  switch (p_event_pckt->evt)
  {
    case HCI_DISCONNECTION_COMPLETE_EVT_CODE:
      conn_handle = APP_BLE_NO_CONN;
      HIDSAPP_Disconnected();
      NUS_Disconnected();
      Adv_Start(1);
      break;

    case HCI_LE_META_EVT_CODE:
    {
      evt_le_meta_event *p_meta_evt = (evt_le_meta_event *)p_event_pckt->data;
      uint16_t handle = APP_BLE_NO_CONN;
      if (p_meta_evt->subevent == HCI_LE_CONNECTION_COMPLETE_SUBEVT_CODE)
      {
        handle = ((hci_le_connection_complete_event_rp0 *)p_meta_evt->data)->Connection_Handle;
      }
      else if (p_meta_evt->subevent == HCI_LE_ENHANCED_CONNECTION_COMPLETE_SUBEVT_CODE)
      {
        handle = ((hci_le_enhanced_connection_complete_event_rp0 *)p_meta_evt->data)->Connection_Handle;
      }
      if (handle != APP_BLE_NO_CONN)
      {
        HW_TS_Stop(adv_timer_id);
        conn_handle = handle;
        /* Ask the host to pair (or re-encrypt with the bond) right away
         * instead of waiting for it to hit an encrypted attribute. */
        aci_gap_peripheral_security_req(conn_handle);
      }
      break;
    }

    case HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE:
    {
      evt_blecore_aci *p_blecore_evt = (evt_blecore_aci *)p_event_pckt->data;
      switch (p_blecore_evt->ecode)
      {
        case ACI_GAP_BOND_LOST_VSEVT_CODE:
          /* The host forgot us but we still hold its old bond: let it pair again. */
          aci_gap_allow_rebond(conn_handle);
          break;

        case ACI_GATT_INDICATION_VSEVT_CODE:
          aci_gatt_confirm_indication(((aci_gatt_indication_event_rp0 *)p_blecore_evt->data)->Connection_Handle);
          break;

        default:
          break;
      }
      break;
    }

    default:
      break;
  }
  return SVCCTL_UserEvtFlowEnable;
}

/* Public address from the chip's unique ID (same as ST's examples). */
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

static void Gap_Gatt_Init(void)
{
  uint16_t gap_service_handle, gap_dev_name_char_handle, gap_appearance_char_handle;
  uint16_t appearance = BLE_CFG_GAP_APPEARANCE;

  hci_reset();
  aci_hal_write_config_data(CONFIG_DATA_PUBLIC_ADDRESS_OFFSET, CONFIG_DATA_PUBLIC_ADDRESS_LEN, (uint8_t *)BleGetBdAddress());
  aci_hal_write_config_data(CONFIG_DATA_IR_OFFSET, CONFIG_DATA_IR_LEN, (uint8_t *)a_BLE_CfgIrValue);
  aci_hal_write_config_data(CONFIG_DATA_ER_OFFSET, CONFIG_DATA_ER_LEN, (uint8_t *)a_BLE_CfgErValue);
  aci_hal_set_tx_power_level(1, CFG_TX_POWER);

  aci_gatt_init();
  aci_gap_init(GAP_PERIPHERAL_ROLE, CFG_PRIVACY, sizeof(DEVICE_NAME) - 1,
               &gap_service_handle, &gap_dev_name_char_handle, &gap_appearance_char_handle);
  aci_gatt_update_char_value(gap_service_handle, gap_dev_name_char_handle, 0,
                             sizeof(DEVICE_NAME) - 1, (uint8_t *)DEVICE_NAME);
  aci_gatt_update_char_value(gap_service_handle, gap_appearance_char_handle, 0, 2, (uint8_t *)&appearance);

  aci_gap_set_io_capability(CFG_IO_CAPABILITY);
  aci_gap_set_authentication_requirement(CFG_BONDING_MODE,
                                         CFG_MITM_PROTECTION,
                                         CFG_SC_SUPPORT,
                                         CFG_KEYPRESS_NOTIFICATION_SUPPORT,
                                         CFG_ENCRYPTION_KEY_SIZE_MIN,
                                         CFG_ENCRYPTION_KEY_SIZE_MAX,
                                         USE_FIXED_PIN_FOR_PAIRING_FORBIDDEN,
                                         0,
                                         CFG_IDENTITY_ADDRESS);
  aci_gap_configure_filter_accept_list();
}

static void Adv_Start(uint8_t fast)
{
  HW_TS_Stop(adv_timer_id);
  aci_gap_set_non_discoverable();
  aci_gap_set_discoverable(ADV_IND,
                           fast ? CFG_FAST_CONN_ADV_INTERVAL_MIN : CFG_LP_CONN_ADV_INTERVAL_MIN,
                           fast ? CFG_FAST_CONN_ADV_INTERVAL_MAX : CFG_LP_CONN_ADV_INTERVAL_MAX,
                           CFG_BLE_ADDRESS_TYPE,
                           NO_WHITE_LIST_USE,
                           sizeof(a_LocalName), a_LocalName,
                           sizeof(a_ServUuids), (uint8_t *)a_ServUuids,
                           0, 0);
  aci_gap_update_adv_data(sizeof(a_Appearance), a_Appearance);
  if (fast) HW_TS_Start(adv_timer_id, FAST_ADV_TIMEOUT);
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
  if (SVCCTL_UserEvtRx((void *)&(p_param->pckt->evtserial)) != SVCCTL_UserEvtFlowDisable)
  {
    p_param->status = HCI_TL_UserEventFlow_Enable;
  }
  else
  {
    p_param->status = HCI_TL_UserEventFlow_Disable;
  }
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
