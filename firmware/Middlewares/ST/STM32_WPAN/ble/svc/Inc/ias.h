/**
  ******************************************************************************
  * @file    ias.h
  * @author  MCD Application Team
  * @brief   Header for ias.c module
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
#ifndef __IAS_H
#define __IAS_H

#ifdef __cplusplus
extern "C" 
{
#endif


/* Includes ------------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  IAS_NO_ALERT_EVT,
  IAS_MID_ALERT_EVT,
  IAS_HIGH_ALERT_EVT
} IAS_App_Opcode_Notification_evt_t;

typedef struct
{
  IAS_App_Opcode_Notification_evt_t  IAS_Evt_Opcode;
}IAS_App_Notification_evt_t;


/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void IAS_Init(void);
tBleStatus IAS_Update_Char(uint16_t UUID, uint8_t *pPayload);
void IAS_App_Notification(IAS_App_Notification_evt_t *pNotification);


#ifdef __cplusplus
}
#endif

#endif /*__IAS_H */


