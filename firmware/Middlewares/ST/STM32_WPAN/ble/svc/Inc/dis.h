/**
  ******************************************************************************
  * @file    dis.h
  * @author  MCD Application Team
  * @brief   Header for dis.c module
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
#ifndef __DIS_H
#define __DIS_H

#ifdef __cplusplus
extern "C" 
{
#endif

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef struct
{
  uint8_t     *pPayload;
  uint8_t     Length;
}DIS_Data_t;


/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void DIS_Init(void);
tBleStatus DIS_UpdateChar(uint16_t uuid, DIS_Data_t *p_data);

#ifdef __cplusplus
}
#endif

#endif /*__DIS_H */


