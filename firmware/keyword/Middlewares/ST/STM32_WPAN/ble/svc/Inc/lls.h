/**
  ******************************************************************************
  * @file    lls.h
  * @author  MCD Application Team
  * @brief   Header for lls.c module
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
#ifndef __LLS_H
#define __LLS_H

#ifdef __cplusplus
extern "C" 
{
#endif

/* Includes ------------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  LLS_NO_ALERT_EVT,
  LLS_MID_ALERT_EVT,
  LLS_HIGH_ALERT_EVT,
  LLS_DISCONNECT_EVT,
  LLS_CONNECT_EVT
} LLS_App_Opcode_Notification_evt_t;

typedef struct
{
  LLS_App_Opcode_Notification_evt_t  LLS_Evt_Opcode;
}LLS_App_Notification_evt_t;


/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void LLS_Init(void);
tBleStatus LLS_Update_Char(uint16_t UUID, uint8_t *pPayload);
void LLS_App_Notification(LLS_App_Notification_evt_t *pNotification);


#ifdef __cplusplus
}
#endif

#endif /*__LLS_H */


