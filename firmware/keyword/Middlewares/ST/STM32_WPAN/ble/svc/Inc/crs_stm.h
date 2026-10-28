
/**
  ******************************************************************************
  * @file   crs_stm.h
  * @author  MCD Application Team
  * @brief   Header for crs_stm.c module
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
#ifndef __STM32XX_CRS_H
#define __STM32XX_CRS_H

#ifdef __cplusplus
extern "C" 
{
#endif

/* Includes ------------------------------------------------------------------*/ 
/* Exported types ------------------------------------------------------------*/
typedef enum
{
  CRS_NOTIFY_ENABLED_EVT,
  CRS_NOTIFY_DISABLED_EVT,
  CRS_READ_EVT,
  CRS_WRITE_EVT,
} CRS_Opcode_evt_t;

typedef struct
{
  uint8_t * pPayload;
  uint8_t   Length;
}CRS_Data_t;  

typedef struct
{
  CRS_Opcode_evt_t                              CRS_Evt_Opcode;
  CRS_Data_t                                    DataTransfered;
  uint16_t                                      ConnectionHandle;
  uint8_t                                       ServiceInstance;
}CRS_STM_Notification_evt_t;


/* Exported constants --------------------------------------------------------*/
#define CRS_MAX_DATA_LEN                                           (BLE_DEFAULT_ATT_MTU - 3) /**< Maximum length of data (in bytes) that can be transmitted to the peer. */

/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void CRS_STM_Init(void);
void CRS_STM_Notification(CRS_STM_Notification_evt_t *p_Notification);
tBleStatus CRS_STM_Update_Char(uint16_t UUID,  uint8_t *p_Payload);


#ifdef __cplusplus
}
#endif

#endif /*__STM32XX_CRS_H */


