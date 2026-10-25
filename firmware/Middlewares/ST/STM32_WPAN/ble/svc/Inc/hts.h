
/**
 ******************************************************************************
 * @file    hts.h
 * @author  MCD Application Team
 * @brief   Header for shst.c module
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
#ifndef __HTS_H
#define __HTS_H

#ifdef __cplusplus
extern "C" 
{
#endif

/* Includes ------------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  NO_FLAGS = 0,
  VALUE_UNIT_FAHRENHEIT = (1<<0),
  SENSOR_TIME_STAMP_PRESENT = (1<<1),
  SENSOR_TEMPERATURE_TYPE_PRESENT = (1<<2),
} HTS_TM_Flags_t;

typedef enum
{
  TT_Armpit = 1,
  TT_Body = 2,
  TT_Ear = 3,
  TT_Finger = 4,
  TT_Gastro_intestinal_Tract = 5,
  TT_Mouth = 6,
  TT_Rectum = 7,
  TT_Toe = 8,
  TT_Tympanum = 9
} HTS_Temperature_Type_t;

typedef enum
{
  HTS_MEASUREMENT_INTERVAL_RECEIVED_EVT,
  HTS_MEASUREMENT_IND_ENABLED_EVT,
  HTS_MEASUREMENT_IND_DISABLED_EVT,
  HTS_MEASUREMENT_INTERVAL_IND_ENABLED_EVT,
  HTS_MEASUREMENT_INTERVAL_IND_DISABLED_EVT,
  HTS_INTERMEDIATE_TEMPERATURE_NOT_ENABLED_EVT,
  HTS_INTERMEDIATE_TEMPERATURE_NOT_DISABLED_EVT,
} HTS_App_Opcode_Notification_evt_t;

typedef struct
{
  HTS_App_Opcode_Notification_evt_t  HTS_Evt_Opcode;
#if (BLE_CFG_HTS_MEASUREMENT_INTERVAL != 0)
  uint16_t  RangeInterval;
#endif
}HTS_App_Notification_evt_t;

#if (BLE_CFG_HTS_TIME_STAMP_FLAG != 0)
typedef struct
{
  uint16_t  Year;
  uint8_t   Month;
  uint8_t   Day;
  uint8_t   Hours;
  uint8_t   Minutes;
  uint8_t   Seconds;
}HTS_TimeStamp_t;
#endif

typedef struct
{
  uint32_t MeasurementValue;
#if (BLE_CFG_HTS_TIME_STAMP_FLAG != 0)
 HTS_TimeStamp_t TimeStamp;
#endif
#if (BLE_CFG_HTS_TEMPERATURE_TYPE_VALUE_STATIC == 0)
  HTS_Temperature_Type_t  TemperatureType;
#endif
  uint8_t Flags;
}HTS_TemperatureValue_t;


/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void HTS_Init(void);
tBleStatus HTS_Update_Char(uint16_t UUID, 
                           uint8_t *pPayload);
void HTS_App_Notification(HTS_App_Notification_evt_t * pNotification);

#ifdef __cplusplus
}
#endif

#endif /*__HTS_H */


