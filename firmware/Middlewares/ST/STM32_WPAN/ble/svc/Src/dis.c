/**
  ******************************************************************************
  * @file    dis.c
  * @author  MCD Application Team
  * @brief   Device Information Service
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
  uint16_t  DeviceInformationSvcHdle;       /**< Service handle */
#if (BLE_CFG_DIS_MANUFACTURER_NAME_STRING != 0)
  uint16_t  ManufacturerNameStringCharHdle; /**< Characteristic handle */
#endif
#if (BLE_CFG_DIS_MODEL_NUMBER_STRING != 0)
  uint16_t  ModelNumberStringCharHdle;      /**< Characteristic handle */
#endif
#if (BLE_CFG_DIS_SERIAL_NUMBER_STRING != 0)
  uint16_t  SerialNumberStringCharHdle;     /**< Characteristic handle */
#endif
#if (BLE_CFG_DIS_HARDWARE_REVISION_STRING != 0)
  uint16_t  HardwareRevisionStringCharHdle; /**< Characteristic handle */
#endif
#if (BLE_CFG_DIS_FIRMWARE_REVISION_STRING != 0)
  uint16_t  FirmwareRevisionStringCharHdle; /**< Characteristic handle */
#endif
#if (BLE_CFG_DIS_SOFTWARE_REVISION_STRING != 0)
  uint16_t  SoftwareRevisionStringCharHdle; /**< Characteristic handle */
#endif
#if (BLE_CFG_DIS_SYSTEM_ID != 0)
  uint16_t  SystemIDCharHdle;               /**< Characteristic handle */
#endif
#if (BLE_CFG_DIS_IEEE_CERTIFICATION != 0)
  uint16_t  IEEECertificationCharHdle;      /**< Characteristic handle */
#endif
#if (BLE_CFG_DIS_PNP_ID != 0)
  uint16_t  PNPIDCharHdle;                  /**< Characteristic handle */
#endif
}DIS_Context_t;


/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/**
 * START of Section BLE_DRIVER_CONTEXT
 */

PLACE_IN_SECTION("BLE_DRIVER_CONTEXT") static DIS_Context_t DIS_Context;

/**
 * END of Section BLE_DRIVER_CONTEXT
 */


/* Functions Definition ------------------------------------------------------*/
/* Private functions ----------------------------------------------------------*/
/* Public functions ----------------------------------------------------------*/
/**
 * @brief  Service initialization
 * @param  None
 * @retval None
 */
void DIS_Init(void)
{
  uint16_t uuid;
  tBleStatus hciCmdResult;

  memset ( &DIS_Context, 0, sizeof(DIS_Context_t) );

  /**
   *  Register the event handler to the BLE controller
   *
   *  There is no need of an interrupt handler for this service
   */

  /**
   *  Add Device Information Service
   */
  uuid = DEVICE_INFORMATION_SERVICE_UUID;
  hciCmdResult = aci_gatt_add_service(UUID_TYPE_16,
                                      (Service_UUID_t *) &uuid,
                                      PRIMARY_SERVICE,
#if (BLE_CFG_DIS_MANUFACTURER_NAME_STRING != 0)
                                      2+
#endif
#if (BLE_CFG_DIS_MODEL_NUMBER_STRING != 0)
                                      2+
#endif
#if (BLE_CFG_DIS_SERIAL_NUMBER_STRING != 0)
                                      2+
#endif
#if (BLE_CFG_DIS_HARDWARE_REVISION_STRING != 0)
                                      2+
#endif
#if (BLE_CFG_DIS_FIRMWARE_REVISION_STRING != 0)
                                      2+
#endif
#if (BLE_CFG_DIS_SOFTWARE_REVISION_STRING != 0)
                                      2+
#endif
#if (BLE_CFG_DIS_SYSTEM_ID != 0)
                                      2+
#endif
#if (BLE_CFG_DIS_IEEE_CERTIFICATION != 0)
                                      2+
#endif
#if (BLE_CFG_DIS_PNP_ID != 0)
                                      2+
#endif
                                      1,
                                      &(DIS_Context.DeviceInformationSvcHdle));

  if (hciCmdResult == BLE_STATUS_SUCCESS)
  {
    BLE_DBG_DIS_MSG ("Device Information Service (DIS) is added Successfully %04X\n", 
                 DIS_Context.DeviceInformationSvcHdle);
  }
  else
  {
    BLE_DBG_DIS_MSG ("FAILED to add Device Information Service (DIS), Error: %02X !!\n", 
                 hciCmdResult);
  }

#if (BLE_CFG_DIS_MANUFACTURER_NAME_STRING != 0)
  /**
   *  Add Manufacturer Name String Characteristic
   */
  uuid = MANUFACTURER_NAME_UUID;
  hciCmdResult = aci_gatt_add_char(DIS_Context.DeviceInformationSvcHdle,
                                   UUID_TYPE_16,
                                   (Char_UUID_t *) &uuid ,
                                   BLE_CFG_DIS_MANUFACTURER_NAME_STRING_LEN_MAX,
                                   CHAR_PROP_READ,
                                   ATTR_PERMISSION_NONE,
                                   GATT_DONT_NOTIFY_EVENTS, /* gattEvtMask */
                                   10, /* encryKeySize */
                                   CHAR_VALUE_LEN_VARIABLE, /* isVariable */
                                   &(DIS_Context.ManufacturerNameStringCharHdle));

  if (hciCmdResult == BLE_STATUS_SUCCESS)
  {
    BLE_DBG_DIS_MSG ("Manufacturer Name Characteristic Added Successfully  %04X \n", 
                 DIS_Context.ManufacturerNameStringCharHdle);
  }
  else
  {
    BLE_DBG_DIS_MSG ("FAILED to add Manufacturer Name Characteristic, Error: %02X !!\n", 
                hciCmdResult);
  }

#endif

#if (BLE_CFG_DIS_MODEL_NUMBER_STRING != 0)
  /**
   *  Add Model Number String Characteristic
   */
  uuid = MODEL_NUMBER_UUID;
  hciCmdResult = aci_gatt_add_char(DIS_Context.DeviceInformationSvcHdle,
                                   UUID_TYPE_16,
                                   (Char_UUID_t *) &uuid ,
                                   BLE_CFG_DIS_MODEL_NUMBER_STRING_LEN_MAX,
                                   CHAR_PROP_READ,
                                   ATTR_PERMISSION_NONE,
                                   GATT_DONT_NOTIFY_EVENTS, /* gattEvtMask */
                                   10, /* encryKeySize */
                                   CHAR_VALUE_LEN_VARIABLE, /* isVariable */
                                   &(DIS_Context.ModelNumberStringCharHdle));
  if (hciCmdResult == BLE_STATUS_SUCCESS)
  {
    BLE_DBG_DIS_MSG ("Model Number String Characteristic Added Successfully  %04X \n", 
                 DIS_Context.ModelNumberStringCharHdle);
  }
  else
  {
    BLE_DBG_DIS_MSG ("FAILED to add Model Number String Characteristic, Error: %02X !!\n", 
                hciCmdResult);
  }

#endif

#if (BLE_CFG_DIS_SERIAL_NUMBER_STRING != 0)
  /**
   *  Add Serial Number String Characteristic
   */
  uuid = SERIAL_NUMBER_UUID;
  hciCmdResult = aci_gatt_add_char(DIS_Context.DeviceInformationSvcHdle,
                                   UUID_TYPE_16,
                                   (Char_UUID_t *) &uuid ,
                                   BLE_CFG_DIS_SERIAL_NUMBER_STRING_LEN_MAX,
                                   CHAR_PROP_READ,
                                   ATTR_PERMISSION_NONE,
                                   GATT_DONT_NOTIFY_EVENTS, /* gattEvtMask */
                                   10, /* encryKeySize */
                                   CHAR_VALUE_LEN_VARIABLE, /* isVariable */
                                   &(DIS_Context.SerialNumberStringCharHdle));
  if (hciCmdResult == BLE_STATUS_SUCCESS)
  {
    BLE_DBG_DIS_MSG ("Serial Number String Characteristic Added Successfully  %04X \n", 
                 DIS_Context.SerialNumberStringCharHdle);
  }
  else
  {
    BLE_DBG_DIS_MSG ("FAILED to add Serial Number String Characteristic, Error: %02X !!\n", 
                hciCmdResult);
  }

#endif

#if (BLE_CFG_DIS_HARDWARE_REVISION_STRING != 0)
  /**
   *  Add Hardware Revision String Characteristic
   */
  uuid = HARDWARE_REVISION_UUID;
  hciCmdResult = aci_gatt_add_char(DIS_Context.DeviceInformationSvcHdle,
                                   UUID_TYPE_16,
                                   (Char_UUID_t *) &uuid ,
                                   BLE_CFG_DIS_HARDWARE_REVISION_STRING_LEN_MAX,
                                   CHAR_PROP_READ,
                                   ATTR_PERMISSION_NONE,
                                   GATT_DONT_NOTIFY_EVENTS, /* gattEvtMask */
                                   10, /* encryKeySize */
                                   CHAR_VALUE_LEN_VARIABLE, /* isVariable */
                                   &(DIS_Context.HardwareRevisionStringCharHdle));
  if (hciCmdResult == BLE_STATUS_SUCCESS)
  {
    BLE_DBG_DIS_MSG ("Hardware Revision String Characteristic Added Successfully  %04X \n", 
                 DIS_Context.HardwareRevisionStringCharHdle);
  }
  else
  {
    BLE_DBG_DIS_MSG ("FAILED to add Hardware Revision String Characteristic, Error: %02X !!\n", 
                hciCmdResult);
  }

#endif

#if (BLE_CFG_DIS_FIRMWARE_REVISION_STRING != 0)
  /**
   *  Add Firmware Revision String Characteristic
   */
  uuid = FIRMWARE_REVISION_UUID;
  hciCmdResult = aci_gatt_add_char(DIS_Context.DeviceInformationSvcHdle,
                                   UUID_TYPE_16,
                                   (Char_UUID_t *) &uuid ,
                                   BLE_CFG_DIS_FIRMWARE_REVISION_STRING_LEN_MAX,
                                   CHAR_PROP_READ,
                                   ATTR_PERMISSION_NONE,
                                   GATT_DONT_NOTIFY_EVENTS, /* gattEvtMask */
                                   10, /* encryKeySize */
                                   CHAR_VALUE_LEN_VARIABLE, /* isVariable */
                                   &(DIS_Context.FirmwareRevisionStringCharHdle));
  if (hciCmdResult == BLE_STATUS_SUCCESS)
  {
    BLE_DBG_DIS_MSG ("Firmware Revision String Characteristic Added Successfully  %04X \n", 
                 DIS_Context.FirmwareRevisionStringCharHdle);
  }
  else
  {
    BLE_DBG_DIS_MSG ("FAILED to add Firmware Revision String Characteristic, Error: %02X !!\n", 
                hciCmdResult);
  }
#endif

#if (BLE_CFG_DIS_SOFTWARE_REVISION_STRING != 0)
  /**
   *  Add Software Revision String Characteristic
   */
  uuid = SOFTWARE_REVISION_UUID;
  hciCmdResult = aci_gatt_add_char(DIS_Context.DeviceInformationSvcHdle,
                                   UUID_TYPE_16,
                                   (Char_UUID_t *) &uuid ,
                                   BLE_CFG_DIS_SOFTWARE_REVISION_STRING_LEN_MAX,
                                   CHAR_PROP_READ,
                                   ATTR_PERMISSION_NONE,
                                   GATT_DONT_NOTIFY_EVENTS, /* gattEvtMask */
                                   10, /* encryKeySize */
                                   CHAR_VALUE_LEN_VARIABLE, /* isVariable */
                                   &(DIS_Context.SoftwareRevisionStringCharHdle));
  if (hciCmdResult == BLE_STATUS_SUCCESS)
  {
    BLE_DBG_DIS_MSG ("Software Revision String Characteristic Added Successfully  %04X \n", 
                 DIS_Context.SoftwareRevisionStringCharHdle);
  }
  else
  {
    BLE_DBG_DIS_MSG ("FAILED to add Software Revision String Characteristic, Error: %02X !!\n", 
                hciCmdResult);
  }
#endif

#if (BLE_CFG_DIS_SYSTEM_ID != 0)
  /**
   *  Add System ID Characteristic
   */
  uuid = SYSTEM_ID_UUID;
  hciCmdResult = aci_gatt_add_char(DIS_Context.DeviceInformationSvcHdle,
                                   UUID_TYPE_16,
                                   (Char_UUID_t *) &uuid ,
                                   BLE_CFG_DIS_SYSTEM_ID_LEN_MAX,
                                   CHAR_PROP_READ,
                                   ATTR_PERMISSION_NONE,
                                   GATT_DONT_NOTIFY_EVENTS, /* gattEvtMask */
                                   10, /* encryKeySize */
                                   CHAR_VALUE_LEN_CONSTANT, /* isVariable */
                                   &(DIS_Context.SystemIDCharHdle));
  if (hciCmdResult == BLE_STATUS_SUCCESS)
  {
    BLE_DBG_DIS_MSG ("System ID Characteristic Added Successfully  %04X \n", 
                 DIS_Context.SystemIDCharHdle);
  }
  else
  {
    BLE_DBG_DIS_MSG ("FAILED to add System ID Characteristic, Error: %02X !!\n", 
                hciCmdResult);
  }
#endif

#if (BLE_CFG_DIS_IEEE_CERTIFICATION != 0)
  /**
   *  Add IEEE CertificationCharacteristic
   */
  uuid = IEEE_CERTIFICATION_UUID;
  hciCmdResult = aci_gatt_add_char(DIS_Context.DeviceInformationSvcHdle,
                                   UUID_TYPE_16,
                                   (Char_UUID_t *) &uuid ,
                                   BLE_CFG_DIS_IEEE_CERTIFICATION_LEN_MAX,
                                   CHAR_PROP_READ,
                                   ATTR_PERMISSION_NONE,
                                   GATT_DONT_NOTIFY_EVENTS, /* gattEvtMask */
                                   10, /* encryKeySize */
                                   CHAR_VALUE_LEN_VARIABLE, /* isVariable */
                                   &(DIS_Context.IEEECertificationCharHdle));
  if (hciCmdResult == BLE_STATUS_SUCCESS)
  {
    BLE_DBG_DIS_MSG ("IEEE Certification Characteristic Added Successfully  %04X \n", 
                 DIS_Context.IEEECertificationCharHdle);
  }
  else
  {
    BLE_DBG_DIS_MSG ("FAILED to add IEEE Certification Characteristic, Error: %02X !!\n", 
                hciCmdResult);
  }
#endif

#if (BLE_CFG_DIS_PNP_ID != 0)
  /**
   *  Add PNP ID Characteristic
   */
  uuid = PNP_ID_UUID;
  hciCmdResult = aci_gatt_add_char(DIS_Context.DeviceInformationSvcHdle,
                                   UUID_TYPE_16,
                                   (Char_UUID_t *) &uuid ,
                                   BLE_CFG_DIS_PNP_ID_LEN_MAX,
                                   CHAR_PROP_READ,
                                   ATTR_PERMISSION_NONE,
                                   GATT_DONT_NOTIFY_EVENTS, /* gattEvtMask */
                                   10, /* encryKeySize */
                                   CHAR_VALUE_LEN_CONSTANT, /* isVariable */
                                   &(DIS_Context.PNPIDCharHdle));
  if (hciCmdResult == BLE_STATUS_SUCCESS)
  {
    BLE_DBG_DIS_MSG ("PNP ID Characteristic Added Successfully  %04X \n", 
                 DIS_Context.PNPIDCharHdle);
  }
  else
  {
    BLE_DBG_DIS_MSG ("FAILED to add PNP ID Characteristic, Error: %02X !!\n", 
                hciCmdResult);
  }
#endif
      
  return;
}

/**
 * @brief  Characteristic update
 * @param  UUID: UUID of the characteristic
 * @retval None
 */
tBleStatus DIS_UpdateChar(uint16_t UUID, DIS_Data_t *pPData)
{
  tBleStatus return_value;

  switch(UUID)
  {
#if (BLE_CFG_DIS_MANUFACTURER_NAME_STRING != 0)
    case MANUFACTURER_NAME_UUID:
      return_value = aci_gatt_update_char_value(DIS_Context.DeviceInformationSvcHdle,
                                                DIS_Context.ManufacturerNameStringCharHdle,
                                                0,
                                                pPData->Length,
                                                (uint8_t *)pPData->pPayload);
      break;
#endif

#if (BLE_CFG_DIS_MODEL_NUMBER_STRING != 0)
    case MODEL_NUMBER_UUID:
      return_value = aci_gatt_update_char_value(DIS_Context.DeviceInformationSvcHdle,
                                                DIS_Context.ModelNumberStringCharHdle,
                                                0,
                                                pPData->Length,
                                                (uint8_t *)pPData->pPayload);
      break;
#endif

#if (BLE_CFG_DIS_SERIAL_NUMBER_STRING != 0)
    case SERIAL_NUMBER_UUID:
      return_value = aci_gatt_update_char_value(DIS_Context.DeviceInformationSvcHdle,
                                                DIS_Context.SerialNumberStringCharHdle,
                                                0,
                                                pPData->Length,
                                                (uint8_t *)pPData->pPayload);
      break;
#endif

#if (BLE_CFG_DIS_HARDWARE_REVISION_STRING != 0)
    case HARDWARE_REVISION_UUID:
      return_value = aci_gatt_update_char_value(DIS_Context.DeviceInformationSvcHdle,
                                                DIS_Context.HardwareRevisionStringCharHdle,
                                                0,
                                                pPData->Length,
                                                (uint8_t *)pPData->pPayload);
      break;
#endif

#if (BLE_CFG_DIS_FIRMWARE_REVISION_STRING != 0)
    case FIRMWARE_REVISION_UUID:
      return_value = aci_gatt_update_char_value(DIS_Context.DeviceInformationSvcHdle,
                                                DIS_Context.FirmwareRevisionStringCharHdle,
                                                0,
                                                pPData->Length,
                                                (uint8_t *)pPData->pPayload);
      break;
#endif

#if (BLE_CFG_DIS_SOFTWARE_REVISION_STRING != 0)
    case SOFTWARE_REVISION_UUID:
      return_value = aci_gatt_update_char_value(DIS_Context.DeviceInformationSvcHdle,
                                                DIS_Context.SoftwareRevisionStringCharHdle,
                                                0,
                                                pPData->Length,
                                                (uint8_t *)pPData->pPayload);
      break;
#endif

#if (BLE_CFG_DIS_SYSTEM_ID != 0)
    case SYSTEM_ID_UUID:
      return_value = aci_gatt_update_char_value(DIS_Context.DeviceInformationSvcHdle,
                                                DIS_Context.SystemIDCharHdle,
                                                0,
                                                pPData->Length,
                                                (uint8_t *)pPData->pPayload);
      break;
#endif

#if (BLE_CFG_DIS_IEEE_CERTIFICATION != 0)
    case IEEE_CERTIFICATION_UUID:
      return_value = aci_gatt_update_char_value(DIS_Context.DeviceInformationSvcHdle,
                                                DIS_Context.IEEECertificationCharHdle,
                                                0,
                                                pPData->Length,
                                                (uint8_t *)pPData->pPayload);
      break;
#endif

#if (BLE_CFG_DIS_PNP_ID != 0)
    case PNP_ID_UUID:
      return_value = aci_gatt_update_char_value(DIS_Context.DeviceInformationSvcHdle,
                                                DIS_Context.PNPIDCharHdle,
                                                0,
                                                pPData->Length,
                                                (uint8_t *)pPData->pPayload);
      break;
#endif

    default:
      return_value = BLE_STATUS_ERROR;
      break;
  }

  return return_value;
}/* end DIS_UpdateChar() */


