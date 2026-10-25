/**
  ******************************************************************************
  * @file    hrs.h
  * @author  MCD Application Team
  * @brief   Header for stm32xx_heartrate.c module
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



/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __HRS_H
#define __HRS_H

#ifdef __cplusplus
extern "C" {
#endif

  /* Includes ------------------------------------------------------------------*/
  /* Exported types ------------------------------------------------------------*/
  typedef enum
  {
    HRS_HRM_VALUE_FORMAT_UINT16 = 1,
    HRS_HRM_SENSOR_CONTACTS_PRESENT = 2,
    HRS_HRM_SENSOR_CONTACTS_SUPPORTED = 4,
    HRS_HRM_ENERGY_EXPENDED_PRESENT = 8,
    HRS_HRM_RR_INTERVAL_PRESENT = 0x10
  } HRS_HrmFlags_t;

  typedef enum
  {
    HRS_BODY_SENSOR_LOCATION_OTHER = 0,
    HRS_BODY_SENSOR_LOCATION_CHEST = 1,
    HRS_BODY_SENSOR_LOCATION_WRIST = 2,
    HRS_BODY_SENSOR_LOCATION_FINGER = 3,
    HRS_BODY_SENSOR_LOCATION_HAND = 4,
    HRS_BODY_SENSOR_LOCATION_EAR_LOBE = 5,
    HRS_BODY_SENSOR_LOCATION_FOOT = 6
  } HRS_BodySensorLocation_t;

  typedef enum
  {
    HRS_RESET_ENERGY_EXPENDED_EVT,
    HRS_NOTIFICATION_ENABLED,
    HRS_NOTIFICATION_DISABLED,
    HRS_STM_BOOT_REQUEST_EVT,
  } HRS_NotCode_t;
  
  typedef struct
{
  uint8_t * pPayload;
  uint8_t     Length;
}HRS_Data_t;  

typedef struct
{
  HRS_NotCode_t             HRS_Evt_Opcode;
  HRS_Data_t                DataTransfered;
  uint16_t                  ConnectionHandle;
  uint8_t                   ServiceInstance;
}HRS_App_Notification_evt_t;

  typedef struct{
    uint16_t    MeasurementValue;
#if (BLE_CFG_HRS_ENERGY_EXPENDED_INFO_FLAG == 1)
    uint16_t    EnergyExpended;
#endif
#if (BLE_CFG_HRS_ENERGY_RR_INTERVAL_FLAG != 0)
    uint16_t    aRRIntervalValues[BLE_CFG_HRS_ENERGY_RR_INTERVAL_FLAG + BLE_CFG_HRS_ENERGY_EXPENDED_INFO_FLAG];
    uint8_t     NbreOfValidRRIntervalValues;
#endif
    uint8_t     Flags;
  }HRS_MeasVal_t;

  /* Exported constants --------------------------------------------------------*/
  /* External variables --------------------------------------------------------*/
  /* Exported macros -----------------------------------------------------------*/
  /* Exported functions ------------------------------------------------------- */
  void HRS_Init(void);
  tBleStatus HRS_UpdateChar(uint16_t uuid, uint8_t *p_payload);
  void HRS_Notification(HRS_App_Notification_evt_t *pNotification);

#ifdef __cplusplus
}
#endif

#endif /*__HRS_H */


