/**
  ******************************************************************************
  * @file    tps.h
  * @author  MCD Application Team
  * @brief   Header for tps.c module
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
#ifndef __TPS_H
#define __TPS_H

#ifdef __cplusplus
extern "C" 
{
#endif

  /* Includes ------------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void TPS_Init(void);
tBleStatus TPS_Update_Char(uint16_t UUID, uint8_t *pPayload);

#ifdef __cplusplus
}
#endif

#endif /*__TPS_H */


