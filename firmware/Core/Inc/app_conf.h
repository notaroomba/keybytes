/*
 * STM32_WPAN config for the keyword, trimmed down from ST's BLE_Hid example
 * (STM32CubeWB 1.24.0). Peripheral only, one link, Just Works bonding.
 */
#ifndef APP_CONF_H
#define APP_CONF_H

#include "hw.h"
#include "hw_conf.h"
#include "hw_if.h"
#include "ble_bufsize.h"

/* ---- GAP ---- */
#define CFG_TX_POWER                      (0x18) /* -0.15 dBm */
#define CFG_ADV_BD_ADDRESS                (0x7257acd87a6c) /* only used if the UDN and OTP are both blank */
#define CFG_IDENTITY_ADDRESS              GAP_PUBLIC_ADDR
#define CFG_PRIVACY                       PRIVACY_DISABLED
#define CFG_BLE_ADDRESS_TYPE              CFG_IDENTITY_ADDRESS

#define CFG_FAST_CONN_ADV_INTERVAL_MIN    (0x30)  /* 30 ms */
#define CFG_FAST_CONN_ADV_INTERVAL_MAX    (0x60)  /* 60 ms */
#define CFG_LP_CONN_ADV_INTERVAL_MIN      (0x640) /* 1 s */
#define CFG_LP_CONN_ADV_INTERVAL_MAX      (0xFA0) /* 2.5 s */

/* Just Works: no display or keyboard on the keyword, bond so the host
 * reconnects without asking again. */
#define CFG_BONDING_MODE                  (1)
#define CFG_ENCRYPTION_KEY_SIZE_MAX       (16)
#define CFG_ENCRYPTION_KEY_SIZE_MIN       (8)
#define CFG_IO_CAPABILITY                 IO_CAP_NO_INPUT_NO_OUTPUT
#define CFG_MITM_PROTECTION               MITM_PROTECTION_NOT_REQUIRED
#define CFG_SC_SUPPORT                    (0x01) /* LE Secure Connections optional */
#define CFG_KEYPRESS_NOTIFICATION_SUPPORT (0x00)

/* Identity root / encryption root keys. ST's defaults: fine for a hobby
 * board, swap for per-device random ones before shipping many. */
#define CFG_BLE_IR {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0}
#define CFG_BLE_ER {0xFE, 0xDC, 0xBA, 0x09, 0x87, 0x65, 0x43, 0x21, 0xFE, 0xDC, 0xBA, 0x09, 0x87, 0x65, 0x43, 0x21}

/* SMPS stays in bypass. Turn on only if the SMPS inductor is fitted. */
#define CFG_USE_SMPS    0

/* ---- BLE stack (SHCI_C2_BLE_Init) ---- */
#define CFG_BLE_NUM_LINK                1
#define CFG_BLE_NUM_GATT_SERVICES       8
#define CFG_BLE_NUM_GATT_ATTRIBUTES     68
#define CFG_BLE_MAX_ATT_MTU             (156)
#define CFG_BLE_ATT_VALUE_ARRAY_SIZE    (1344)
#define CFG_BLE_PREPARE_WRITE_LIST_SIZE BLE_PREP_WRITE_X_ATT(CFG_BLE_MAX_ATT_MTU)
#define CFG_BLE_MBLOCK_COUNT            (BLE_MBLOCKS_CALC(CFG_BLE_PREPARE_WRITE_LIST_SIZE, CFG_BLE_MAX_ATT_MTU, CFG_BLE_NUM_LINK))
#define CFG_BLE_DATA_LENGTH_EXTENSION   1
#define CFG_BLE_PERIPHERAL_SCA          500
#define CFG_BLE_CENTRAL_SCA             0
/* PeriphCommonClock_Config feeds RFWKP from HSE/1024, so the stack's sleep
 * clock has to match. Switch both to LSE if low power ever gets turned on. */
#define CFG_BLE_LS_SOURCE  (SHCI_C2_BLE_INIT_CFG_BLE_LS_NOCALIB | SHCI_C2_BLE_INIT_CFG_BLE_LS_OTHER_DEV | SHCI_C2_BLE_INIT_CFG_BLE_LS_CLK_HSE_1024)
#define CFG_BLE_HSE_STARTUP_TIME        0x148
#define CFG_BLE_MAX_CONN_EVENT_LENGTH   (0xFFFFFFFF)
#define CFG_BLE_VITERBI_MODE            1
#define CFG_BLE_OPTIONS  (SHCI_C2_BLE_INIT_OPTIONS_LL_HOST | SHCI_C2_BLE_INIT_OPTIONS_WITH_SVC_CHANGE_DESC | SHCI_C2_BLE_INIT_OPTIONS_DEVICE_NAME_RW | SHCI_C2_BLE_INIT_OPTIONS_NO_EXT_ADV | SHCI_C2_BLE_INIT_OPTIONS_NO_CS_ALGO2 | SHCI_C2_BLE_INIT_OPTIONS_FULL_GATTDB_NVM | SHCI_C2_BLE_INIT_OPTIONS_GATT_CACHING_NOTUSED | SHCI_C2_BLE_INIT_OPTIONS_POWER_CLASS_2_3)
#define CFG_BLE_OPTIONS_EXT  (SHCI_C2_BLE_INIT_OPTIONS_APPEARANCE_READONLY | SHCI_C2_BLE_INIT_OPTIONS_ENHANCED_ATT_NOTSUPPORTED)
#define CFG_BLE_MAX_COC_INITIATOR_NBR   (32)
#define CFG_BLE_MIN_TX_POWER            (-40)
#define CFG_BLE_MAX_TX_POWER            (6)
#define CFG_BLE_MAX_ADD_EATT_BEARERS    (4)
#define CFG_BLE_RX_MODEL_CONFIG         (SHCI_C2_BLE_INIT_RX_MODEL_AGC_RSSI_LEGACY)
#define CFG_BLE_MAX_ADV_SET_NBR         (3)
#define CFG_BLE_MAX_ADV_DATA_LEN        (1650)
#define CFG_BLE_TX_PATH_COMPENS         (0)
#define CFG_BLE_RX_PATH_COMPENS         (0)
#define CFG_BLE_CORE_VERSION            (SHCI_C2_BLE_INIT_BLE_CORE_5_4)

/* ---- transport layer ---- */
#define CFG_TLBLE_EVT_QUEUE_LENGTH          5
#define CFG_TLBLE_MOST_EVENT_PAYLOAD_SIZE   255
#define TL_BLE_EVENT_FRAME_SIZE             (TL_EVT_HDR_SIZE + CFG_TLBLE_MOST_EVENT_PAYLOAD_SIZE)

#define CFG_USB_INTERFACE_ENABLE    0

/* Off: the keyword's main loop also bit-bangs SWIO and polls I2C3, which a
 * STOP2 entry from the sequencer idle would break. */
#define CFG_LPM_SUPPORTED    0

/* ---- timer server on the RTC wakeup timer, LSE / 16 ---- */
#define CFG_RTCCLK_DIV            (16)
#define CFG_RTC_WUCKSEL_DIVIDER   (0)
#define CFG_RTC_ASYNCH_PRESCALER  (0x0F)
#define CFG_RTC_SYNCH_PRESCALER   (0x7FFF)
#define CFG_TS_TICK_VAL           DIVR((CFG_RTCCLK_DIV * 1000000), LSE_VALUE)
#define CFG_TS_TICK_VAL_PS        DIVR(((uint64_t)CFG_RTCCLK_DIV * 1e12), (uint64_t)LSE_VALUE)
#define MS_TO_TS_TICKS(ms)        ((uint32_t)((ms) * 1000UL / CFG_TS_TICK_VAL))

typedef enum
{
  CFG_TIM_PROC_ID_ISR,
} CFG_TimProcID_t;

#define CFG_HW_RESET_BY_FW        1
#define CFG_DEBUGGER_SUPPORTED    1

/* No debug UART on the keyword. */
#define APP_DBG_MSG               PRINT_NO_MESG
#define CFG_DEBUG_TRACE_FULL      0
#define CFG_DEBUG_TRACE_LIGHT     0

/* ---- sequencer ---- */
typedef enum
{
  CFG_TASK_ADV_UPDATE_ID,
  CFG_TASK_KEYS_ID,
  CFG_TASK_NUS_RX_ID,
  CFG_TASK_CDC_RX_ID,
  CFG_TASK_HCI_ASYNCH_EVT_ID,
  CFG_LAST_TASK_ID_WITH_HCICMD,   /* shall be last in this list */
} CFG_Task_Id_With_HCI_Cmd_t;

typedef enum
{
  CFG_FIRST_TASK_ID_WITH_NO_HCICMD = CFG_LAST_TASK_ID_WITH_HCICMD - 1,
  CFG_TASK_SYSTEM_HCI_ASYNCH_EVT_ID,
  CFG_TASK_FUS_ID,
  CFG_LAST_TASK_ID_WITH_NO_HCICMD,
} CFG_Task_Id_With_NO_HCI_Cmd_t;

#define CFG_TASK_NBR    CFG_LAST_TASK_ID_WITH_NO_HCICMD

typedef enum
{
  CFG_SCH_PRIO_0,
  CFG_SCH_PRIO_NBR
} CFG_SCH_Prio_Id_t;

typedef enum
{
  CFG_IDLEEVT_HCI_CMD_EVT_RSP_ID,
  CFG_IDLEEVT_SYSTEM_HCI_CMD_EVT_RSP_ID,
} CFG_IdleEvt_Id_t;

typedef enum
{
  CFG_LPM_APP,
  CFG_LPM_APP_BLE,
} CFG_LPM_Id_t;

#define CFG_OTP_BASE_ADDRESS    OTP_AREA_BASE
#define CFG_OTP_END_ADRESS      OTP_AREA_END_ADDR

#endif /* APP_CONF_H */
