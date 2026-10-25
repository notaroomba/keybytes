
/**
  ******************************************************************************
  * @file    zdd_stm.h
  * @author  MCD Application Team
  * @brief   Header for zdd_stm.c module
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
#ifndef __ZDD_STM_H
#define __ZDD_STM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

 
  
/* Exported types ------------------------------------------------------------*/
typedef enum
{
  /* ZDD P2P Events */
  ZDD_P2P_STM__NOTIFY_ENABLED_EVT,
  ZDD_P2P_STM_NOTIFY_DISABLED_EVT,
  ZDD_P2P_STM_READ_EVT,
  ZDD_P2P_STM_WRITE_EVT,
  ZDD_P2P_STM_BOOT_REQUEST_EVT,
  /* ZDD Security Events */
  ZDD_SEC_P_256_INDICATE_ENABLED_EVT,
  ZDD_SEC_P_256_INDICATE_DISABLED_EVT,
  ZDD_SEC_P_256_WRITE_EVT,
  ZDD_SEC_CURVE25519_INDICATE_ENABLED_EVT,
  ZDD_SEC_CURVE25519_INDICATE_DISABLED_EVT,
  ZDD_SEC_CURVE25519_WRITE_EVT,
  /* ZDD Commissioning Events */
  ZDD_COMM_FORM_NWK_WRITE_EVT,
  ZDD_COMM_JOIN_NWK_WRITE_EVT,
  ZDD_COMM_PERMIT_JOIN_WRITE_EVT,
  ZDD_COMM_LEAVE_NWK_WRITE_EVT,
  ZDD_COMM_STATUS_NOTIFY_ENABLED_EVT,
  ZDD_COMM_STATUS_NOTIFY_DISABLED_EVT,
  ZDD_COMM_STATUS_READ_EVT,
  /* ZDD Tunnelling Events */
  ZDD_TUNN_ZDTS_NPDU_INDICATE_ENABLED_EVT,
  ZDD_TUNN_ZDTS_NPDU_INDICATE_DISABLED_EVT,
  ZDD_TUNN_ZDTS_NPDU_WRITE_EVT

} ZDD_STM_Opcode_evt_t;

typedef struct
{
  uint8_t * pPayload;
  uint8_t     Length;
}ZDD_STM_Data_t;

typedef struct
{
  ZDD_STM_Opcode_evt_t      ZDD_Evt_Opcode;
  ZDD_STM_Data_t            DataTransfered;
  uint16_t                  ConnectionHandle;
  uint8_t                   ServiceInstance;
}ZDD_STM_App_Notification_evt_t;


/* Exported constants --------------------------------------------------------*/
#define ZDD_P2P_NOTIFY_CHAR_UUID        P2P_NOTIFY_CHAR_UUID  /* Temp */
#define ZDD_SEC_P_256_CHAR_UUID         (0xAF42)              /* P-256 */
#define ZDD_SEC_CURVE25519_CHAR_UUID    (0xAF43)              /* Curve25519 */
#define ZDD_COMM_STATUS_CHAR_UUID       (0x377D)              /* Commissioning Status */
#define ZDD_TUNN_ZDTS_NPDU_CHAR_UUID    (0x78FD)              /* ZDTS-NPDU */

/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void ZDD_STM_Init( void );
void ZDD_STM_App_Notification(ZDD_STM_App_Notification_evt_t *pNotification);
tBleStatus ZDD_STM_App_Update_Char(uint16_t UUID, uint8_t payloadLen, uint8_t *pPayload);


#ifdef __cplusplus
}
#endif

#endif /*__ZDD_STM_H */


