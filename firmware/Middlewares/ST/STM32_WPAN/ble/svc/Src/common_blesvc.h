
/**
  ******************************************************************************
  * @file    common_blesvc.h
  * @author  MCD Application Team
  * @brief   Header for ble modules
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
#ifndef __COMMON_BLESVC_H
#define __COMMON_BLESVC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "ble_common.h"
#include "ble.h"
#include "dbg_trace.h"



/* Exported types ------------------------------------------------------------*/
typedef enum
{
  COMSVC_Notification = ( 1 << 0 ),
  COMSVC_Indication =   ( 1 << 1 ),
} COMSVC_ClientCharConfMask_t;

/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/


#ifdef __cplusplus
}
#endif

#endif /*__COMMON_BLESVC_H */


