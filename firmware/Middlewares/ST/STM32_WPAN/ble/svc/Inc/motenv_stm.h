
/**
  ******************************************************************************
  * @file    motenv_stm.h
  * @author  SRA/AST
  * @brief   Header for motenv_stm.c module
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2018-2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef MOTENV_STM_H
#define MOTENV_STM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
/**
 * @brief  MOTENV Event Opcode definition
 */
typedef enum
{
  /* HW Service Chars related events */
  HW_MOTION_NOTIFY_ENABLED_EVT,
  HW_MOTION_NOTIFY_DISABLED_EVT,
  HW_ENV_NOTIFY_ENABLED_EVT,
  HW_ENV_NOTIFY_DISABLED_EVT,
  HW_ENV_READ_EVT,
  HW_ACC_EVENT_NOTIFY_ENABLED_EVT,
  HW_ACC_EVENT_NOTIFY_DISABLED_EVT,
  HW_ACC_EVENT_READ_EVT,
  HW_TOF_NOTIFY_ENABLED_EVT,
  HW_TOF_NOTIFY_DISABLED_EVT,
  HW_TOF_WRITE_EVT,
  /* SW Service Chars related events */
  SW_MOTIONFX_NOTIFY_ENABLED_EVT,
  SW_MOTIONFX_NOTIFY_DISABLED_EVT,
  SW_ECOMPASS_NOTIFY_ENABLED_EVT,
  SW_ECOMPASS_NOTIFY_DISABLED_EVT,
  SW_ACTIVITY_REC_NOTIFY_ENABLED_EVT,
  SW_ACTIVITY_REC_NOTIFY_DISABLED_EVT,
  SW_ACTIVITY_REC_READ_EVT,
  SW_CARRY_POSITION_NOTIFY_ENABLED_EVT,
  SW_CARRY_POSITION_NOTIFY_DISABLED_EVT,
  SW_CARRY_POSITION_READ_EVT,
  SW_GESTURE_REC_NOTIFY_ENABLED_EVT,
  SW_GESTURE_REC_NOTIFY_DISABLED_EVT,
  SW_GESTURE_REC_READ_EVT,
  SW_PEDOMETER_NOTIFY_ENABLED_EVT,
  SW_PEDOMETER_NOTIFY_DISABLED_EVT,
  SW_PEDOMETER_READ_EVT,
  SW_INTENSITY_DET_NOTIFY_ENABLED_EVT,
  SW_INTENSITY_DET_NOTIFY_DISABLED_EVT,
  MOTENV_STM_BOOT_REQUEST_EVT,
  /* Config Service Chars related events */
  CONFIG_NOTIFY_ENABLED_EVT,
  CONFIG_NOTIFY_DISABLED_EVT,
  CONFIG_WRITE_EVT,
  /* Console Service Chars related events */
  CONSOLE_TERM_NOTIFY_ENABLED_EVT,
  CONSOLE_TERM_NOTIFY_DISABLED_EVT,
  CONSOLE_STDERR_NOTIFY_ENABLED_EVT,
  CONSOLE_STDERR_NOTIFY_DISABLED_EVT,
  CONSOLE_TERM_READ_EVT,
  CONSOLE_STDERR_READ_EVT
} MOTENV_STM_Opcode_evt_t;

/**
 * @brief  MOTENV Event data structure definition
 */
typedef struct
{
  uint8_t *pPayload;
  uint8_t  Length;
} MOTENV_STM_Data_t;  

/**
 * @brief  MOTENV Notification structure definition
 */
typedef struct
{
  MOTENV_STM_Opcode_evt_t Motenv_Evt_Opcode;
  MOTENV_STM_Data_t       DataTransfered;
  uint16_t                ConnectionHandle;
  uint8_t                 ServiceInstance;
} MOTENV_STM_App_Notification_evt_t;


/* Exported constants --------------------------------------------------------*/
/* Exported Variables ------------------------------------------------------- */
extern uint8_t ToF_BoardPresent;

/* Exported macros -----------------------------------------------------------*/
/**
 * @brief  Motion (Acc-Gyro-Magneto) Char shortened UUID
 */
#define MOTION_CHAR_UUID                (0xE000)
/**
 * @brief  Environmental (Temp-Humidity-Pressure) Char shortened UUID
 */
#define ENV_CHAR_UUID                   (0x1D00)
/**
 * @brief  ToF Char shortened UUID
 */
#define TOF_CHAR_UUID                (0x0000)
/**
 * @brief  Acceleration event Char shortened UUID
 */
#define ACC_EVENT_CHAR_UUID             (0x0004)
/**
 * @brief  Sensor Fusion Char shortened UUID
 */
#define MOTION_FX_CHAR_UUID             (0x0100)
/**
 * @brief  E-Compass event Char shortened UUID
 */
#define ECOMPASS_CHAR_UUID              (0x0040)
/**
 * @brief  Activity Recognition Char event Char shortened UUID
 */
#define ACTIVITY_REC_CHAR_UUID          (0x0010)
/**
 * @brief  Carry Position event Char shortened UUID
 */
#define CARRY_POSITION_CHAR_UUID        (0x0008)
/**
 * @brief  Gesture Recognition event Char shortened UUID
 */
#define GESTURE_REC_CHAR_UUID           (0x0200)
/**
 * @brief  Pedometer Char shortened UUID
 */
#define PEDOMETER_CHAR_UUID             (0x0001)
/**
 * @brief  Intensity Detection Char shortened UUID
 */
#define INTENSITY_DET_CHAR_UUID         (0x0020)
/**
 * @brief  Config Char shortened UUID
 */
#define CONFIG_CHAR_UUID                (0x0002)
/**
 * @brief  Console Terminal Char shortened UUID
 */
#define CONSOLE_TERM_CHAR_UUID          (0x010E)
/**
 * @brief  Cosnole Stderr Char shortened UUID
 */
#define CONSOLE_STDERR_CHAR_UUID        (0x020E)

/* Exported functions ------------------------------------------------------- */
void MOTENV_STM_Init(void);
void MOTENV_STM_App_Notification(MOTENV_STM_App_Notification_evt_t *pNotification);
tBleStatus MOTENV_STM_App_Update_Char(uint16_t UUID, uint8_t payloadLen, uint8_t *pPayload);


#ifdef __cplusplus
}
#endif

#endif /* MOTENV_STM_H */


