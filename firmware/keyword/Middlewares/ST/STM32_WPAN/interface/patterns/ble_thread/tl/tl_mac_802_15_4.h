/**
 ******************************************************************************
 * @file    tl_mac_802_15_4.h
 * @author  MCD Application Team
 * @brief   Constants and functions for managing MAC 802.15.4 TL
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

#ifndef __TL_MAC_802_15_4_H
#define __TL_MAC_802_15_4_H

/* Includes ------------------------------------------------------------------*/

#include "tl.h"
#include <string.h>

#include "802_15_4_mac_core.h"

/* Exported functions  ------------------------------------------------------------*/
void Mac_802_15_4_PreCmdProcessing(void);
void Mac_802_15_4_CmdTransfer(void);

TL_CmdPacket_t* MAC_802_15_4_GetCmdBuffer(void);
TL_Evt_t* MAC_802_15_4_GetRspPayEvt(void);

TL_Evt_t* MAC_802_15_4_GetNotificationBuffer(void);
MAC_802_15_4_Notification_t* MAC_802_15_4_GetNotificationPayloadBuffer(void);

/* Exported defines -----------------------------------------------------------*/


#endif /* __TL_MAC_802_15_4_H_ */
