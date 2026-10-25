/**
  ******************************************************************************
  * @file    ble.h
  * @author  MCD Application Team
  * @brief   BLE interface
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
#ifndef __BLE_H
#define __BLE_H

#ifdef __cplusplus
extern "C" {
#endif


/* Includes ------------------------------------------------------------------*/
#include "ble_conf.h"
#include "ble_dbg_conf.h"

/**< core */
#include "core/ble_core.h"
#include "core/ble_bufsize.h"
#include "core/ble_defs.h"
#include "core/auto/ble_vs_codes.h"
#include "core/ble_legacy.h"
#include "core/ble_std.h"

/**< blesvc */
#include "svc/Inc/bas.h"    
#include "svc/Inc/bls.h"
#include "svc/Inc/crs_stm.h"
#include "svc/Inc/dis.h"
#include "svc/Inc/eds_stm.h"
#include "svc/Inc/hids.h"
#include "svc/Inc/hrs.h"
#include "svc/Inc/hts.h"
#include "svc/Inc/ias.h"
#include "svc/Inc/lls.h"
#include "svc/Inc/tps.h"
#include "svc/Inc/motenv_stm.h"
#include "svc/Inc/p2p_stm.h"
#include "svc/Inc/zdd_stm.h"
#include "svc/Inc/otas_stm.h"
#include "svc/Inc/mesh.h"  
#include "svc/Inc/template_stm.h"  
  
#include "svc/Inc/svc_ctl.h"

#include "svc/Inc/uuid.h"


/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
  /* -------------------------------- *
   * [retrieved from ble_legacy file]
   * Macro to get RSSI from advertising report #0.
   * "p" must be a pointer to the event parameters buffer
   * -------------------------------- */
#define HCI_LE_ADVERTISING_REPORT_RSSI_0(p) \
        (*(int8_t*)((&((hci_le_advertising_report_event_rp0*)(p))-> \
                      Advertising_Report[0].Length_Data) + 1 + \
                    ((hci_le_advertising_report_event_rp0*)(p))-> \
                    Advertising_Report[0].Length_Data))

/* Exported functions ------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /*__BLE_H */


