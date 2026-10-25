/**
 ******************************************************************************
 * @file    bas.c
 * @author  MCD Application Team
 * @brief   Battery Service
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
typedef struct
{
  uint16_t  BatterySvcHdle;                 /**< Service handle */
  uint16_t  BatteryLevelCharHdle;           /**< Characteristic handle */
  uint16_t  CharPresentationFormatDescHdle; /**< Descriptor handle */
}BAS_Context_t;


/* Private defines -----------------------------------------------------------*/
/* Store Value into a buffer in Little Endian Format */
#define STORE_LE_16(buf, val)    ( ((buf)[0] =  (uint8_t) (val)    ) , \
                                   ((buf)[1] =  (uint8_t) (val>>8) ) )


/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/**
 * START of Section BLE_APP_CONTEXT
 */

PLACE_IN_SECTION("BLE_DRIVER_CONTEXT")  BAS_Context_t BAS_Context[BLE_CFG_BAS_NUMBER];

/**
 * END of Section BLE_APP_CONTEXT
 */


/* Private function prototypes -----------------------------------------------*/
static SVCCTL_EvtAckStatus_t BAS_Event_Handler(void *Event);


/* Functions Definition ------------------------------------------------------*/
/* Private functions ----------------------------------------------------------*/
/**
 * @brief  Event handler
 * @param  Event: Address of the buffer holding the Event
 * @retval Ack: Return whether the Event has been managed or not
 */
static SVCCTL_EvtAckStatus_t BAS_Event_Handler(void *Event)
{
  SVCCTL_EvtAckStatus_t return_value;
  hci_event_pckt *event_pckt;
  evt_blecore_aci *blecore_evt;
  aci_gatt_attribute_modified_event_rp0    * attribute_modified;
  aci_gatt_read_permit_req_event_rp0 * attribute_read;
  uint8_t service_instance;
  BAS_Notification_evt_t Notification;

  return_value = SVCCTL_EvtNotAck;
  event_pckt = (hci_event_pckt *)(((hci_uart_pckt*)Event)->data);

  switch(event_pckt->evt)
  {
    case HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE:
    {
      blecore_evt = (evt_blecore_aci*)event_pckt->data;
      switch(blecore_evt->ecode)
      {
        case ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE:
          {
            attribute_modified = (aci_gatt_attribute_modified_event_rp0*)blecore_evt->data;
            for(service_instance = 0; service_instance < BLE_CFG_BAS_NUMBER ; service_instance++)
            {
              if(attribute_modified->Attr_Handle == (BAS_Context[service_instance].BatteryLevelCharHdle + 2))
              {
                return_value = SVCCTL_EvtAckFlowEnable;
                if(attribute_modified->Attr_Data[0] & COMSVC_Notification)
                {
                  Notification.BAS_Evt_Opcode = BAS_LEVEL_NOT_ENABLED_EVT;
                  Notification.ServiceInstance = service_instance;
                  BAS_Notification(&Notification);
                }
                else
                {
                  Notification.BAS_Evt_Opcode = BAS_LEVEL_NOT_DISABLED_EVT;
                  Notification.ServiceInstance = service_instance;
                  BAS_Notification(&Notification);
                }
              }
            }
          }
        break;
        case ACI_GATT_READ_PERMIT_REQ_VSEVT_CODE:
          {
            attribute_read = (aci_gatt_read_permit_req_event_rp0*)blecore_evt->data;
            for(service_instance = 0; service_instance < BLE_CFG_BAS_NUMBER ; service_instance++)
            {
              if(attribute_read->Attribute_Handle == (BAS_Context[service_instance].BatteryLevelCharHdle + 1))
              {
                return_value = SVCCTL_EvtAckFlowEnable;
                aci_gatt_allow_read(attribute_read->Connection_Handle);
                Notification.BAS_Evt_Opcode = BAS_LEVEL_READ_EVT;
                Notification.ServiceInstance = service_instance;
                BAS_Notification(&Notification);
              }
            }
          }
          break;

        default:
          break;
      }
    }
    break; /* HCI_HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE_SPECIFIC */

    default:
      break;
  }

  return(return_value);
}/* end BAS_Event_Handler */


/* Public functions ----------------------------------------------------------*/
/**
 * @brief  Service initialization
 * @param  none
 * @retval None
 */
void BAS_Init(void)
{
  uint16_t uuid;
  uint8_t numAttribRec = 0;
  tBleStatus hciCmdResult;
  uint8_t bat_option = 0;
  uint8_t service_instance;
    
  for(service_instance = 0; service_instance < BLE_CFG_BAS_NUMBER ; service_instance++)
  {
    /**
     *	Register the event handler to the BLE controller
     */
    /* Only one registraton */
    if(service_instance == 0)
    SVCCTL_RegisterSvcHandler(BAS_Event_Handler);

    /* serviceMaxAttributeRecord = 1 for battery service itself + 
     *                             2 for battery level characteristic + 
     */
    numAttribRec = 3;

    if(BLE_CFG_BAS_NUMBER > 1)
      numAttribRec++;                  /* + Characteristic Presentation Format Descriptor */
    
#if (BLE_CFG_BAS_LEVEL_NOT_PROP == 1)
    numAttribRec++;                  /* + Client Characteristic Configuration Descriptor */
    bat_option = CHAR_PROP_NOTIFY;
#endif
    
    /* store the Battery Service UUID */
    uuid = BATTERY_SERVICE_UUID;
    BLE_DBG_BAS_MSG("Battery Service UUID %x \n",
               uuid);
    hciCmdResult = aci_gatt_add_service(UUID_TYPE_16, 
                                     (Service_UUID_t *)&uuid, 
                                     PRIMARY_SERVICE, 
                                     numAttribRec,
                                       &(BAS_Context[service_instance].BatterySvcHdle));
    if (hciCmdResult == BLE_STATUS_SUCCESS)
    {
        BLE_DBG_BAS_MSG ("Battery Service (BAS) is added Successfully %04X\n", 
                    BAS_Context[service_instance].BatterySvcHdle);
    }
    else
    {
        BLE_DBG_BAS_MSG ("FAILED to add Battery Service (BAS), Error: %02X !!\n", 
                  hciCmdResult);
    }

    BLE_DBG_BAS_MSG ("Adding Battery Level Characteristic.\n");
                        
    uuid = BATTERY_LEVEL_CHAR_UUID;

    /* Add ES Battery Level Characteristic */
    hciCmdResult = aci_gatt_add_char(BAS_Context[service_instance].BatterySvcHdle, 
                                   UUID_TYPE_16, 
                                   (Char_UUID_t *) &uuid , 
                                   1, 
                                   CHAR_PROP_READ | bat_option, 
                                   ATTR_PERMISSION_NONE, 
                                   GATT_NOTIFY_READ_REQ_AND_WAIT_FOR_APPL_RESP, /* gattEvtMask */
                                   10,                      /* encryKeySize */
                                   CHAR_VALUE_LEN_VARIABLE, /* isVariable */
                                     &(BAS_Context[service_instance].BatteryLevelCharHdle));

    if (hciCmdResult == BLE_STATUS_SUCCESS)
    {
      BLE_DBG_BAS_MSG ("Battery Level Characteristic Added Successfully  %04X \n", 
                  BAS_Context[service_instance].BatteryLevelCharHdle);
    }
    else
    {
      BLE_DBG_BAS_MSG ("FAILED to add Battery Level Characteristic, Error: %02X !!\n", 
                hciCmdResult);
    }

    if(BLE_CFG_BAS_NUMBER > 1)
    {
      uint8_t buf[20] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

      /* Add Battery Characteristic Presentation Format Descriptor */
      uuid = CHAR_PRESENTATION_FORMAT_DESCRIPTOR_UUID;
      buf[0] = 4;        /* unsigned 8-bit integer */
      buf[1] = 0;        /* exponent 0 */
      STORE_LE_16(&buf[2], 0x27AD /*+ service_instance*/); /* percentage unit(0x27AD) + service instance */
      buf[4] = 1;        /* Bluetooth SIG Assigned Numbers */
        STORE_LE_16(&buf[5], (0x0030+service_instance)); /* ST microelectronics assigned number */
        hciCmdResult = aci_gatt_add_char_desc(BAS_Context[service_instance].BatterySvcHdle,
                                              BAS_Context[service_instance].BatteryLevelCharHdle,
                                            UUID_TYPE_16,
                                            (Char_Desc_Uuid_t *)&uuid,
                                            7,
                                            7,
                                            (void *)&buf,
                                            ATTR_PERMISSION_NONE,
                                            ATTR_ACCESS_READ_ONLY,
                                            GATT_DONT_NOTIFY_EVENTS,
                                            10,
                                            CHAR_VALUE_LEN_CONSTANT,
                                              &BAS_Context[service_instance].CharPresentationFormatDescHdle);

      if (hciCmdResult == BLE_STATUS_SUCCESS)
      {
        BLE_DBG_BAS_MSG ("Battery Characteristic Presentation Format Descriptor Added Successfully  %04X \n", 
                    BAS_Context[service_instance].CharPresentationFormatDescHdle);
      }
      else
      {
        BLE_DBG_BAS_MSG ("FAILED to add Battery Characteristic Presentation Format Descriptor, Error: %02X !!\n", 
                  hciCmdResult);
      }
    }

  } /* End for(service_instance = 0; service_instance < CFG_BAS_NUMBER ; service_instance++) */

  return;
}


/**
 * @brief  Characteristic update
 * @param  UUID: UUID of the characteristic
 * @param  service_snstance: Instance of the service to which the characteristic belongs
 * @param  pPayload
 */
void BAS_Update_Char(uint16_t UUID, uint8_t service_instance, uint8_t *pPayload)
{
  switch(UUID)
  {
    case BATTERY_LEVEL_CHAR_UUID:
      aci_gatt_update_char_value(BAS_Context[service_instance].BatterySvcHdle,
                                 BAS_Context[service_instance].BatteryLevelCharHdle,
                                 0,           /* charValOffset */
                                 1, /* charValueLen */
                                 (uint8_t *)pPayload);
      break;

    default:
      break;
  }

  return;
}/* end BAS_Update_Char() */



