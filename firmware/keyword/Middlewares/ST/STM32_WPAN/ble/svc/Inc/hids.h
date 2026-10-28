/**
  ******************************************************************************
  * @file    hids.h
  * @author  MCD Application Team
  * @brief   Header for hids.c module
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
#ifndef __HIDS_H
#define __HIDS_H

#ifdef __cplusplus
extern "C" 
{
#endif

/* Includes ------------------------------------------------------------------*/


/* Exported types ------------------------------------------------------------*/
typedef enum
{
  HIDS_REPORT_NOTIFICATION_ENABLED,
  HIDS_REPORT_NOTIFICATION_DISABLED,
  HIDS_KEYB_INPUT_NOTIFY_ENABLED,
  HIDS_KEYB_INPUT_NOTIFY_DISABLED,
  HIDS_MOUSE_INPUT_NOTIFY_ENABLED,
  HIDS_MOUSE_INPUT_NOTIFY_DISABLED,
  HIDS_OUTPUT_REPORT,
  HIDS_KEYBOARD_INPUT_REPORT,
  HIDS_KEYBOARD_OUTPUT_REPORT,
  HIDS_MOUSE_INPUT_REPORT,
  HIDS_CONN_HANDLE_EVT,
  HIDS_DISCON_HANDLE_EVT
} HIDS_Opcode_Notification_evt_t;

typedef struct
{
  HIDS_Opcode_Notification_evt_t  HIDS_Evt_Opcode;
  uint8_t   Instance;
  uint8_t   Index;
  uint8_t   ReportLength;
  uint8_t   *pReport;
} HIDS_App_Notification_evt_t;


/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void HIDS_Init(void);
tBleStatus HIDS_Update_Char(uint16_t UUID, 
                            uint8_t service_instance, 
                            uint8_t Report_Index, 
                            uint8_t report_size,
                            uint8_t *pPayload);
void HIDS_Notification(HIDS_App_Notification_evt_t *pNotification);


#ifdef __cplusplus
}
#endif

#endif /*__HIDS_H */


