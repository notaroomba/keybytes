
/**
  ******************************************************************************
  *  @file    template_stm.h
  * @author  MCD Application Team
  * @brief   Header for template_stm.c module
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
#ifndef __TEMPLATE_STM_H
#define __TEMPLATE_STM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

 
  
/* Exported types ------------------------------------------------------------*/
typedef enum
{
  TEMPLATE_STM_NOTIFY_ENABLED_EVT,
  TEMPLATE_STM_NOTIFY_DISABLED_EVT,
  TEMPLATE_STM_READ_EVT,
  TEMPLATE_STM_WRITE_EVT,
  TEMPLATE_STM_BOOT_REQUEST_EVT,
} TEMPLATE_STM_Opcode_evt_t;

typedef struct
{
  uint8_t * pPayload;
  uint8_t     Length;
}TEMPLATE_STM_Data_t;  

typedef struct
{
  TEMPLATE_STM_Opcode_evt_t     Template_Evt_Opcode;
  TEMPLATE_STM_Data_t           DataTransfered;
  uint16_t                  ConnectionHandle;
  uint8_t                   ServiceInstance;
}TEMPLATE_STM_App_Notification_evt_t;


/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void TEMPLATE_STM_Init( void );
void TEMPLATE_STM_App_Notification(TEMPLATE_STM_App_Notification_evt_t *pNotification);
tBleStatus TEMPLATE_STM_App_Update_Char(uint16_t UUID,  uint8_t *pPayload);


#ifdef __cplusplus
}
#endif

#endif /*__TEMPLATE_STM_H */


