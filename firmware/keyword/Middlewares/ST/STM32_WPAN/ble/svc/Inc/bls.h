/**
  ******************************************************************************
  * @file    bls.h
  * @author  MCD Application Team
  * @brief   Header for bls.c module
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
#ifndef __BLS_H
#define __BLS_H

#ifdef __cplusplus
extern "C" 
{
#endif

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef enum
{
  NO_FLAG = 0,
  VALUE_UNIT_KILO_PASCAL = (1<<0),                       /*0 -> Blood pressure systolic, diastolic & Mean values in units of mmHg - if 1 -> in units of kPa*/
  TIME_STAMP_PRESENT = (1<<1),
  PULSE_RATE_PRESENT = (1<<2),
  USER_ID_PRESENT = (1<<3),
  MEASUREMENT_STATUS_PRESENT = (1<<4)
} BLS_Measurement_Flags_t;

typedef enum
{
  BLS_MEASUREMENT_IND_ENABLED_EVT,
  BLS_MEASUREMENT_IND_DISABLED_EVT,
#if (BLE_CFG_BLS_INTERMEDIATE_CUFF_PRESSURE != 0)
  BLS_INTERMEDIATE_CUFF_PRESSURE_NOTIF_ENABLED_EVT,
  BLS_INTERMEDIATE_CUFF_PRESSURE_NOTIF_DISABLED_EVT,   
#endif
} BLS_App_Opcode_Notification_evt_t;

typedef struct
{
  BLS_App_Opcode_Notification_evt_t  BLS_Evt_Opcode;
}BLS_App_Notification_evt_t;

typedef struct
{
  uint16_t  Year;
  uint8_t   Month;
  uint8_t   Day;
  uint8_t   Hours;
  uint8_t   Minutes;
  uint8_t   Seconds;
}BLS_TimeStamp_t;

typedef struct
{
  uint16_t MeasurementValue_Systolic;
  uint16_t MeasurementValue_Diastolic;
  uint16_t MeasurementValue_Mean;
#if (BLE_CFG_BLS_TIME_STAMP_FLAG != 0)
  BLS_TimeStamp_t TimeStamp;
#endif
#if (BLE_CFG_BLS_PULSE_RATE_FLAG != 0)
  uint16_t          PulseRate;
#endif    
#if (BLE_CFG_BLS_USER_ID_FLAG != 0)
  uint8_t          UserID;
#endif        
#if (BLE_CFG_BLS_MEASUREMENT_STATUS_FLAG != 0)
  uint16_t MeasurementStatus;
#endif
  uint8_t Flags;
}BLS_Value_t;
  

/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void BLS_Init(void);
void BLS_Update_Char(uint16_t UUID, uint8_t *pPayload);
void BLS_App_Notification(BLS_App_Notification_evt_t * pNotification);

#ifdef __cplusplus
}
#endif

#endif /*__BLS_H */


