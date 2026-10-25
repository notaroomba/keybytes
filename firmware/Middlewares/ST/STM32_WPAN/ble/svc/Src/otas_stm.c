/**
 ******************************************************************************
 * @file    otas_stm.c
 * @author  MCD Application Team
 * @brief   OTA Service
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2018-2021 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */


/* Includes ------------------------------------------------------------------*/
#include "common_blesvc.h"

/* Private typedef -----------------------------------------------------------*/
typedef enum
{
  OTAS_Conf_Not_Pending,
  OTAS_Conf_Pending,
} OTAS_Conf_Status_t;

typedef struct
{
  uint16_t	OTAS_SvcHdle;				          /**< Service handle */
  uint16_t  OTAS_Base_Addr_CharHdle;      /**< Characteristic handle */
  uint16_t  OTAS_Conf_CharHdle;           /**< Characteristic handle */
  uint16_t  OTAS_Raw_Data_CharHdle;       /**< Characteristic handle */
  OTAS_Conf_Status_t    OTAS_Conf_Status; /**< Indication Status */
}OTAS_Context_t;


/* Private defines -----------------------------------------------------------*/
#define UUID_128_SUPPORTED  1
#if (UUID_128_SUPPORTED == 1)
#define OTA_UUID_LENGTH  UUID_TYPE_128
#else
#define OTA_UUID_LENGTH  UUID_TYPE_16
#endif

#define OTA_BASE_ADR_CHAR_SIZE    (4)
#define OTA_CONF_CHAR_SIZE        (1)
#define OTA_RAW_DATA_CHAR_SIZE    OTAS_STM_RAW_DATA_SIZE

/**
* Service UUID
* 0000fe20-cc7a-482a-984a-7f2ed5b3e58f
*/
#if (UUID_128_SUPPORTED == 1)
const uint8_t OTAS_SVC_UUID[16] = {0x8f, 0xe5, 0xb3, 0xd5,
        0x2e, 0x7f, 0x4a, 0x98,
        0x2a, 0x48, 0x7a, 0xcc,
        0x20, 0xFE, 0x00, 0x00};
#else
const uint8_t OTAS_SVC_UUID[2] = {0x20, 0xFE};
#endif


/**
* Base Address Characteristic UUID
* 0000fe22-8e22-4541-9d4c-21edae82ed19
*/
#if (UUID_128_SUPPORTED == 1)
const uint8_t OTA_BASE_ADR_CHAR_UUID[16] = {0x19, 0xed, 0x82, 0xae,
        0xed, 0x21, 0x4c, 0x9d,
        0x41, 0x45, 0x22, 0x8e,
        0x22, 0xFE, 0x00, 0x00};
#else
const uint8_t OTA_BASE_ADR_CHAR_UUID[2] = {0x22, 0xFE};
#endif

/**
* Confirmation Characteristic UUID
* 0000fe23-8e22-4541-9d4c-21edae82ed19
*/
#if (UUID_128_SUPPORTED == 1)
const uint8_t OTA_CONF_CHAR_UUID[16] = {0x19, 0xed, 0x82, 0xae,
        0xed, 0x21, 0x4c, 0x9d,
        0x41, 0x45, 0x22, 0x8e,
        0x23, 0xFE, 0x00, 0x00};
#else
const uint8_t OTA_CONF_CHAR_UUID[2] = {0x23, 0xfe};
#endif

/**
* Raw Data Characteristic UUID
* 0000fe24-8e22-4541-9d4c-21edae82ed19
*/
#if (UUID_128_SUPPORTED == 1)
const uint8_t OTA_RAW_DATA_CHAR_UUID[16] = {0x19, 0xed, 0x82, 0xae,
        0xed, 0x21, 0x4c, 0x9d,
       0x41, 0x45, 0x22, 0x8e,
        0x24, 0xFE, 0x00, 0x00};
#else
const uint8_t OTA_RAW_DATA_CHAR_UUID[2] = {0x24, 0xfe};
#endif


/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/**
 * START of Section BLE_DRIVER_CONTEXT
 */
PLACE_IN_SECTION("BLE_DRIVER_CONTEXT") static OTAS_Context_t OTAS_Context;
/**
 * END of Section BLE_DRIVER_CONTEXT
 */

/* Private function prototypes -----------------------------------------------*/
static SVCCTL_EvtAckStatus_t OTAS_Event_Handler(void *Event);


/* Functions Definition ------------------------------------------------------*/
/* Private functions ----------------------------------------------------------*/

/**
 * @brief  Event handler
 * @param  Event: Address of the buffer holding the Event
 * @retval Ack: Return whether the Event has been managed or not
 */
static SVCCTL_EvtAckStatus_t OTAS_Event_Handler(void *Event)
{
  SVCCTL_EvtAckStatus_t return_value;
  hci_event_pckt *event_pckt;
  evt_blecore_aci *blecore_evt;
  aci_gatt_attribute_modified_event_rp0    * attribute_modified;
  OTA_STM_Notification_t notification;

  return_value = SVCCTL_EvtNotAck;

  event_pckt = (hci_event_pckt *)(((hci_uart_pckt*)Event)->data);

  switch(event_pckt->evt)
  {
    case HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE:
    {
      blecore_evt = (evt_blecore_aci*)event_pckt->data;
      attribute_modified = (aci_gatt_attribute_modified_event_rp0*)blecore_evt->data;
      switch(blecore_evt->ecode)
      {
        case ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE:
        {
          if(attribute_modified->Attr_Handle == (OTAS_Context.OTAS_Base_Addr_CharHdle + 1))
          {
            /**
             * Base Address
             */
            return_value = SVCCTL_EvtAckFlowEnable;

            notification.ChardId = OTAS_STM_BASE_ADDR_ID;
            notification.pPayload = (uint8_t*)&attribute_modified->Attr_Data[0];
            notification.ValueLength = attribute_modified->Attr_Data_Length;
            OTAS_STM_Notification( &notification );
          }
          else if(attribute_modified->Attr_Handle == (OTAS_Context.OTAS_Raw_Data_CharHdle  + 1))
          {
            /**
             * Raw Data
             */
            return_value = SVCCTL_EvtAckFlowEnable;

            notification.ChardId = OTAS_STM_RAW_DATA_ID;
            notification.pPayload = (uint8_t*)&attribute_modified->Attr_Data[0];
            notification.ValueLength = attribute_modified->Attr_Data_Length;
            OTAS_STM_Notification( &notification );
          }
        }
        break;

        case ACI_GATT_SERVER_CONFIRMATION_VSEVT_CODE:
        {
          if( OTAS_Context.OTAS_Conf_Status != OTAS_Conf_Not_Pending)
          {
            /**
             * Confirmation Event
             */
            OTAS_Context.OTAS_Conf_Status = OTAS_Conf_Not_Pending;

            return_value = SVCCTL_EvtAckFlowEnable;

            notification.ChardId = OTAS_STM_CONF_EVENT_ID;
            notification.pPayload = (uint8_t*)&attribute_modified->Attr_Data[0];
            notification.ValueLength = attribute_modified->Attr_Data_Length;
            OTAS_STM_Notification( &notification );
          }
        }
        break;
      
        case ACI_GATT_WRITE_PERMIT_REQ_VSEVT_CODE:
          {
            aci_gatt_write_permit_req_event_rp0 * write_perm_req;
            write_perm_req = (aci_gatt_write_permit_req_event_rp0*)blecore_evt->data;
                if(write_perm_req->Attribute_Handle == (OTAS_Context.OTAS_Raw_Data_CharHdle  + 1)) 
                {
                  return_value = SVCCTL_EvtAckFlowEnable;          
                  aci_gatt_write_resp(write_perm_req->Connection_Handle,
                                          write_perm_req->Attribute_Handle,
                                          0x00, /* write_status = 0 (no error))*/
                                          0x00, /* err_code */
                                          write_perm_req->Data_Length,
                                          (uint8_t *)&(write_perm_req->Data[0]));
                }
          }
          break;
   
        default:
          break;
      }
    }
    break;

    default:
      break;
  }

  return(return_value);
}

/* Public functions ----------------------------------------------------------*/
void OTAS_STM_Init(void)
{
  /**
   *	Register the event handler to the BLE controller
   */
  SVCCTL_RegisterSvcHandler(OTAS_Event_Handler);

  /**
   *  Add OTA Service
   */
  aci_gatt_add_service(OTA_UUID_LENGTH,
                       (Service_UUID_t *)OTAS_SVC_UUID,
                       PRIMARY_SERVICE,
                       1
                       + 2  /**< OTA_BASE CHAR */
                       + 3  /**< OTA_CONF CHAR */
                       + 2,  /**< OTA_RAW_DATA CHAR */
                       &(OTAS_Context.OTAS_SvcHdle));


  /**
   *  Add Base Address Characteristic
   */
  aci_gatt_add_char(OTAS_Context.OTAS_SvcHdle,
                    OTA_UUID_LENGTH,
                    (Char_UUID_t *)OTA_BASE_ADR_CHAR_UUID,
                    OTA_BASE_ADR_CHAR_SIZE,
                    CHAR_PROP_WRITE_WITHOUT_RESP,
                    ATTR_PERMISSION_NONE,
                    GATT_NOTIFY_ATTRIBUTE_WRITE,
                    10,
                    0,
                    &(OTAS_Context.OTAS_Base_Addr_CharHdle));

  /**
   *  Add Confirmation Characteristic
   */
  aci_gatt_add_char(OTAS_Context.OTAS_SvcHdle,
                    OTA_UUID_LENGTH,
                    (Char_UUID_t *)OTA_CONF_CHAR_UUID,
                    OTA_CONF_CHAR_SIZE,
                    CHAR_PROP_INDICATE,
                    ATTR_PERMISSION_NONE,
                    GATT_DONT_NOTIFY_EVENTS,
                    10,
                    0,
                    &(OTAS_Context.OTAS_Conf_CharHdle));

  /**
   *  Add Raw Data Characteristic
   */
  aci_gatt_add_char(OTAS_Context.OTAS_SvcHdle,
                    OTA_UUID_LENGTH,
                    (Char_UUID_t *)OTA_RAW_DATA_CHAR_UUID,
                    OTA_RAW_DATA_CHAR_SIZE,
                    CHAR_PROP_WRITE_WITHOUT_RESP,
                    ATTR_PERMISSION_NONE,
                    GATT_NOTIFY_ATTRIBUTE_WRITE | GATT_NOTIFY_WRITE_REQ_AND_WAIT_FOR_APPL_RESP,
                    10,
                    1,
                    &(OTAS_Context.OTAS_Raw_Data_CharHdle));

  OTAS_Context.OTAS_Conf_Status = OTAS_Conf_Not_Pending;

  return;
}

tBleStatus OTAS_STM_UpdateChar(OTAS_STM_ChardId_t  ChardId, uint8_t *p_payload)
{
  tBleStatus return_value;

  OTAS_Context.OTAS_Conf_Status = OTAS_Conf_Pending;

  return_value = aci_gatt_update_char_value(OTAS_Context.OTAS_SvcHdle,
                                            OTAS_Context.OTAS_Conf_CharHdle,
                                            0,                  /**< charValOffset */
                                            OTA_CONF_CHAR_SIZE, /**< charValueLen */
                                            p_payload);

  return return_value;
}



