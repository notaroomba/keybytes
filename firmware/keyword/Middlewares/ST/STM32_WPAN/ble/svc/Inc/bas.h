/**
  ******************************************************************************
  * @file    bas.h
  * @author  MCD Application Team
  * @brief   Header for bas.c module
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
#ifndef __BAS_H
#define __BAS_H

#ifdef __cplusplus
extern "C" 
{
#endif

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef enum
{
  BAS_LEVEL_NOT_ENABLED_EVT,
  BAS_LEVEL_NOT_DISABLED_EVT,
  BAS_LEVEL_READ_EVT
} BAS_Opcode_Notification_evt_t;

typedef struct
{
  BAS_Opcode_Notification_evt_t  BAS_Evt_Opcode;
  uint8_t ServiceInstance;
}BAS_Notification_evt_t;


/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
#define BAS_LEVEL_NOTIFICATION_OPTION                                          1


/* Exported functions ------------------------------------------------------- */
void BAS_Init(void);
void BAS_Update_Char(uint16_t UUID, uint8_t service_instance, uint8_t *pPayload);
void BAS_Notification(BAS_Notification_evt_t * pNotification);

#ifdef __cplusplus
}
#endif

#endif /*__BAS_H */


