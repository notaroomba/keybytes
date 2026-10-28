/**
 ******************************************************************************
 * @file    otas_stm.h
 * @author  MCD Application Team
 * @brief   Interface to OTA BLE service
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
#ifndef __OTAS_STM_H
#define __OTAS_STM_H

#ifdef __cplusplus
extern "C" {
#endif

  /* Includes ------------------------------------------------------------------*/
#include "stm32_wpan_common.h"

  /* Exported defines -----------------------------------------------------------*/
#define OTAS_STM_RAW_DATA_SIZE    (248)

  /* Exported types ------------------------------------------------------------*/
  typedef enum
  {
    OTAS_STM_BASE_ADDR_ID,
    OTAS_STM_RAW_DATA_ID,
    OTAS_STM_CONF_ID,
    OTAS_STM_CONF_EVENT_ID,
   } OTAS_STM_ChardId_t;

  typedef enum
  {
    OTAS_STM_STOP_ALL_UPLOAD          = 0x00,
    OTAS_STM_WIRELESS_FW_UPLOAD       = 0x01,
    OTAS_STM_APPLICATION_UPLOAD       = 0x02,
    OTAS_STM_UPLOAD_FINISHED          = 0x07,
    OTAS_STM_CANCEL_UPLOAD            = 0x08,
  } OTAS_STM_Command_t;

  typedef enum
  {
    OTAS_STM_REBOOT_CONFIRMED         = 0x01,
  } OTAS_STM_Indication_Msg_t;

  typedef struct{
    uint8_t             *pPayload;
    OTAS_STM_ChardId_t  ChardId;
    uint8_t             ValueLength;
  } OTA_STM_Notification_t;

  typedef PACKED_STRUCT{
    OTAS_STM_Command_t   Command;     /**< [0:7] */
    uint8_t   Base_Addr[3];           /**< [8:31] */
  } OTA_STM_Base_Addr_Event_Format_t;

  typedef PACKED_STRUCT{
    uint8_t   Raw_Data[OTAS_STM_RAW_DATA_SIZE];
  } OTA_STM_Raw_Data_Event_Format_t;

  typedef PACKED_STRUCT{
    aci_gatt_server_confirmation_event_rp0    Conf_Event;
  } OTA_STM_Conf_Event_Format_t;

  typedef PACKED_STRUCT{
    OTAS_STM_Indication_Msg_t Conf_Msg;
  } OTA_STM_Conf_Char_Format_t;

  /* Exported constants --------------------------------------------------------*/
  /* External variables --------------------------------------------------------*/
  /* Exported macros -----------------------------------------------------------*/
  /* Exported functions ------------------------------------------------------- */
  void OTAS_STM_Notification( OTA_STM_Notification_t *p_notification );

  /**
   * @brief  Service initialization
   * @param  None
   * @retval None
   */
  void OTAS_STM_Init(void);

  /**
   * @brief  Characteristic update
   * @param  ChardId: Id of the characteristic to be written
   * @param  p_payload: The new value to be written
   * @retval Command status
   */
  tBleStatus OTAS_STM_UpdateChar(OTAS_STM_ChardId_t  ChardId, uint8_t *p_payload);

#ifdef __cplusplus
}
#endif

#endif /*__OTAS_STM_H */


