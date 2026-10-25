/*
 * Config for the keyword's OTA bootloader, from ST's BLE_Ota (STM32CubeWB
 * 1.24.0) with the keyword app's clocks (HSE, SMPS bypass, RF wakeup on
 * HSE/1024) and no Nucleo LEDs, buttons or debug UART.
 */
#ifndef APP_CONF_H
#define APP_CONF_H

#include "hw.h"
#include "hw_conf.h"
#include "hw_if.h"
#include "ble_bufsize.h"

/* ---- flash map ----
 * 0x08000000  this bootloader, pages 0-6 (28 KB)
 * 0x08007000  the keyword app, page 7 up to the keymap page
 * SFSA - 1    the app's saved keymap, never erased or written here
 * SFSA        CPU2 (BLE stack, FUS), never touched */
#define CFG_APP_START_SECTOR_INDEX   (7)
#define CFG_APP_START_ADDRESS        (FLASH_BASE + CFG_APP_START_SECTOR_INDEX * FLASH_PAGE_SIZE)
#define CFG_APP_TAG_OFFSET           (0x140)        /* word holding the address of the 0x94448A29 tag */
#define CFG_APP_MAGIC                (0x94448A29UL)

/* ---- boot request in SRAM1, kept across a software reset ----
 * Same bytes as ST's BLE_Ota (SRAM1_BASE + 0..2), so the app sets them the
 * same way. The DFU word at + 4 is the keyword's addition; RAM1 in both
 * linker scripts starts at 0x20000008 so nothing else lives there. */
#define CFG_REBOOT_ON_FW_APP          (0x00)
#define CFG_REBOOT_ON_BLE_OTA_APP     (0x01)
#define CFG_REBOOT_ON_CPU2_UPGRADE    (0x02)
#define CFG_OTA_REBOOT_VAL_MSG            (*(volatile uint8_t *)(SRAM1_BASE + 0))
#define CFG_OTA_START_SECTOR_IDX_VAL_MSG  (*(volatile uint8_t *)(SRAM1_BASE + 1))
#define CFG_OTA_NBR_OF_SECTOR_VAL_MSG     (*(volatile uint8_t *)(SRAM1_BASE + 2))
#define CFG_SYSDFU_REQ_WORD               (*(volatile uint32_t *)(SRAM1_BASE + 4))
#define CFG_SYSDFU_REQ_MAGIC              (0x4446553FUL)   /* "?UFD" */

/* ---- GAP ---- */
#define CFG_TX_POWER                      (0x18) /* -0.15 dBm */
#define CFG_ADV_BD_ADDRESS                (0x7257acd87a6c)
#define CFG_IDENTITY_ADDRESS              GAP_PUBLIC_ADDR
#define CFG_PRIVACY                       PRIVACY_DISABLED
#define CFG_BLE_ADDRESS_TYPE              CFG_IDENTITY_ADDRESS
#define CFG_FAST_CONN_ADV_INTERVAL_MIN    (0x80)  /* 80 ms */
#define CFG_FAST_CONN_ADV_INTERVAL_MAX    (0xA0)  /* 100 ms */
#define CFG_DEV_ID_OTA_FW_UPDATE          (0x86)  /* ST's id for an OTA loader, in the manufacturer data */
#define CFG_BLE_IR {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0}
#define CFG_BLE_ER {0xFE, 0xDC, 0xBA, 0x09, 0x87, 0x65, 0x43, 0x21, 0xFE, 0xDC, 0xBA, 0x09, 0x87, 0x65, 0x43, 0x21}

#define CFG_USE_SMPS    0

/* ---- BLE stack (SHCI_C2_BLE_Init), as BLE_Ota: 251 byte MTU for 248 byte chunks ---- */
#define CFG_BLE_NUM_LINK                1
#define CFG_BLE_NUM_GATT_SERVICES       8
#define CFG_BLE_NUM_GATT_ATTRIBUTES     68
#define CFG_BLE_MAX_ATT_MTU             (251)
#define CFG_BLE_ATT_VALUE_ARRAY_SIZE    (1344)
#define CFG_BLE_PREPARE_WRITE_LIST_SIZE BLE_PREP_WRITE_X_ATT(CFG_BLE_MAX_ATT_MTU)
#define CFG_BLE_MBLOCK_COUNT            (BLE_MBLOCKS_CALC(CFG_BLE_PREPARE_WRITE_LIST_SIZE, CFG_BLE_MAX_ATT_MTU, CFG_BLE_NUM_LINK))
#define CFG_BLE_DATA_LENGTH_EXTENSION   1
#define CFG_BLE_PERIPHERAL_SCA          500
#define CFG_BLE_CENTRAL_SCA             0
/* Same as the app: RFWKP on HSE/1024 and the stack's sleep clock to match */
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

#define CFG_TLBLE_EVT_QUEUE_LENGTH          5
#define CFG_TLBLE_MOST_EVENT_PAYLOAD_SIZE   255
#define TL_BLE_EVENT_FRAME_SIZE             (TL_EVT_HDR_SIZE + CFG_TLBLE_MOST_EVENT_PAYLOAD_SIZE)

#define CFG_USB_INTERFACE_ENABLE    0
#define CFG_LPM_SUPPORTED           0

/* ---- timer server on the RTC wakeup timer, LSE / 16 ---- */
#define CFG_RTCCLK_DIV            (16)
#define CFG_RTC_WUCKSEL_DIVIDER   (0)
#define CFG_RTC_ASYNCH_PRESCALER  (0x0F)
#define CFG_RTC_SYNCH_PRESCALER   (0x7FFF)
#define CFG_TS_TICK_VAL           DIVR((CFG_RTCCLK_DIV * 1000000), LSE_VALUE)
#define CFG_TS_TICK_VAL_PS        DIVR(((uint64_t)CFG_RTCCLK_DIV * 1e12), (uint64_t)LSE_VALUE)

typedef enum
{
  CFG_TIM_PROC_ID_ISR,
} CFG_TimProcID_t;

#define CFG_HW_RESET_BY_FW        1
#define CFG_DEBUGGER_SUPPORTED    1

#define APP_DBG_MSG               PRINT_NO_MESG
#define CFG_DEBUG_TRACE_FULL      0
#define CFG_DEBUG_TRACE_LIGHT     0

/* ---- sequencer ---- */
typedef enum
{
  CFG_TASK_HCI_ASYNCH_EVT_ID,
  CFG_LAST_TASK_ID_WITH_HCICMD,
} CFG_Task_Id_With_HCI_Cmd_t;

typedef enum
{
  CFG_FIRST_TASK_ID_WITH_NO_HCICMD = CFG_LAST_TASK_ID_WITH_HCICMD - 1,
  CFG_TASK_SYSTEM_HCI_ASYNCH_EVT_ID,
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
