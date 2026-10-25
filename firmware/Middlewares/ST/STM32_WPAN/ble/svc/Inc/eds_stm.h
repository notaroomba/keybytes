
/**
  ******************************************************************************
  * @file    eds_stm.h
  * @author  MCD Application Team
  * @brief   Header for stm32xx_enddevicemanagement.c module
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
#ifndef __EDS_STM_H
#define __EDS_STM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

  
/* Exported types ------------------------------------------------------------*/
typedef enum
{
  EDS_STM_NOTIFY_DISABLED_EVT,
  EDS_STM_NOTIFY_ENABLED_EVT,
} EDS_STM_Opcode_evt_t;

typedef struct
{
    uint8_t     Device1_Status;
    uint8_t     Device2_Status;
    uint8_t     Device3_Status;
    uint8_t     Device4_Status;
    uint8_t     Device5_Status;
    uint8_t     Device6_Status;
 }EDS_STM_Status_t;
 
typedef struct
{
  uint8_t * pPayload;
  uint8_t     Length;
}EDS_STM_Data_t;  

typedef struct
{
  EDS_STM_Opcode_evt_t  EDS_Evt_Opcode;
  EDS_STM_Data_t        DataTransfered;
  uint16_t              ConnectionHandle;
 }EDS_STM_App_Notification_evt_t;



/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void EDS_STM_Init( void );
void EDS_STM_App_Notification(EDS_STM_App_Notification_evt_t * pNotification);
tBleStatus EDS_STM_Update_Char(uint16_t UUID,  uint8_t *pPayload);


#ifdef __cplusplus
}
#endif

#endif /*__EDS_STM_H */


