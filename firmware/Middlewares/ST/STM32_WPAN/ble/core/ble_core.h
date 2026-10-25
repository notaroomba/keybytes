/*****************************************************************************
 * @file    ble_core.h
 *
 * @brief   This file contains the definitions for BLE stack
 *****************************************************************************
 * @attention
 *
 * Copyright (c) 2018-2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 *****************************************************************************
 */

#ifndef BLE_CORE_H__
#define BLE_CORE_H__


/* BLE standard definitions */
#include "ble_std.h"

/* BLE stack API definitions */
#include "ble_defs.h"
#include "auto/ble_vs_codes.h"
#include "auto/ble_gen_aci.h"
#include "auto/ble_gap_aci.h"
#include "auto/ble_gatt_aci.h"
#include "auto/ble_l2cap_aci.h"
#include "auto/ble_hal_aci.h"
#include "auto/ble_hci_le.h"
#include "auto/ble_events.h"

/* BLE stack buffer size definitions */
#include "ble_bufsize.h"

/* BLE stack legacy definitions */
#include "ble_legacy.h"


#endif /* BLE_CORE_H__ */
