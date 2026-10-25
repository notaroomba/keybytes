/*****************************************************************************
 * @file    ble_gen_aci.h
 * @brief   STM32WB BLE API (GEN_ACI)
 *          Auto-generated file: do not edit!
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

#ifndef BLE_GEN_ACI_H__
#define BLE_GEN_ACI_H__


#include "auto/ble_types.h"

/**
 * @brief ACI_RESET
 * This command resets the BLE stack (Host and LE Controller).
 *
 * @param Mode ACI reset mode.
 *        Values:
 *        - 0x00: Reset without BLE stack options change
 *        - 0x01: Reset with BLE stack option changes
 * @param Options New BLE stack options to set at ACI reset (a bit set to 1
 *        means that the corresponding optional feature is activated).
 *        Flags:
 *        - 0x00000001: LL only mode
 *        - 0x00000002: No service change description
 *        - 0x00000004: Device Name is read-only
 *        - 0x00000008: Support of Extended Advertising
 *        - 0x00000010: Support of Channel Selection Algorithm #2
 *        - 0x00000020: Reduced GATT database in NVM
 *        - 0x00000040: Support of GATT caching
 *        - 0x00000080: Support of LE Power Class 1 (flag not available in RCP
 *          mode)
 *        - 0x00000100: Appearance is writable
 *        - 0x00000200: Support of Enhanced ATT
 * @return Value indicating success or error code.
 */
tBleStatus aci_reset( uint8_t Mode,
                      uint32_t Options );

/**
 * @brief ACI_GET_INFORMATION
 * This command reads the local ACI information.
 *
 * @param[out] Version BLE stack version.
 * @param[out] Options Current BLE stack options (a bit set to 1 means that the
 *        corresponding optional feature is activated).
 *        Flags:
 *        - 0x00000001: LL only mode
 *        - 0x00000002: No service change description
 *        - 0x00000004: Device Name is read-only
 *        - 0x00000008: Support of Extended Advertising
 *        - 0x00000010: Support of Channel Selection Algorithm #2
 *        - 0x00000020: Reduced GATT database in NVM
 *        - 0x00000040: Support of GATT caching
 *        - 0x00000080: Support of LE Power Class 1 (flag not available in RCP
 *          mode)
 *        - 0x00000100: Appearance is writable
 *        - 0x00000200: Support of Enhanced ATT
 * @param[out] Debug_Info BLE stack debug information.
 * @return Value indicating success or error code.
 */
tBleStatus aci_get_information( uint32_t* Version,
                                uint32_t* Options,
                                uint32_t* Debug_Info );

/**
 * @brief ACI_WRITE_CONFIG_DATA
 * This command writes a value to a configure data structure. It is useful to
 * setup directly some parameters for the BLE stack.
 * Refer to Annex for details on the different parameters that can be
 * configured.
 *
 * @param Offset Offset of the element in the configuration data structure
 *        which has to be written.
 *        Values:
 *        - 0x00: CONFIG_DATA_PUBLIC_ADDRESS_OFFSET;
 *          Bluetooth public address; 6 bytes
 *        - 0x08: CONFIG_DATA_ER_OFFSET;
 *          Encryption root key; 16 bytes
 *        - 0x18: CONFIG_DATA_IR_OFFSET;
 *          Identity root key; 16 bytes
 *        - 0x2E: CONFIG_DATA_RANDOM_ADDRESS_OFFSET;
 *          Static Random Address; 6 bytes
 *        - 0x34: CONFIG_DATA_GAP_ADD_REC_NBR_OFFSET;
 *          GAP service additional record number; 1 byte
 *        - 0x35: CONFIG_DATA_SC_KEY_TYPE_OFFSET;
 *          Secure Connections key type; 1 byte
 *        - 0xB0: CONFIG_DATA_SMP_MODE_OFFSET;
 *          SMP mode; 1 byte
 *        - 0xC0: CONFIG_DATA_LL_SCAN_CHAN_MAP_OFFSET;
 *          LL scan channel map; 1 byte
 *        - 0xC1: CONFIG_DATA_LL_BG_SCAN_MODE_OFFSET;
 *          LL background scan mode; 1 byte
 *        - 0xC3: CONFIG_DATA_LL_RPA_MODE_OFFSET;
 *          LL RPA mode; 1 byte
 *        - 0xD1: CONFIG_DATA_LL_MAX_DATA_EXT_OFFSET [only for full stack];
 *          LL maximum data length extension; 8 bytes
 * @param Length Length of data to be written
 * @param Value Data to be written
 * @return Value indicating success or error code.
 */
tBleStatus aci_write_config_data( uint8_t Offset,
                                  uint8_t Length,
                                  const uint8_t* Value );

/**
 * @brief ACI_READ_CONFIG_DATA
 * This command requests the value in the configure data structure. The number
 * of read bytes changes for different Offset.
 *
 * @param Offset Offset of the element in the configuration data structure
 *        which has to be read.
 *        Values:
 *        - 0x00: CONFIG_DATA_PUBLIC_ADDRESS_OFFSET;
 *          Bluetooth public address; 6 bytes
 *        - 0x08: CONFIG_DATA_ER_OFFSET;
 *          Encryption root key used to derive LTK (legacy) and CSRK; 16 bytes
 *        - 0x18: CONFIG_DATA_IR_OFFSET
 *          Identity root key used to derive DHK (legacy) and IRK; 16 bytes
 *        - 0x2E: CONFIG_DATA_RANDOM_ADDRESS_OFFSET;
 *          Static Random Address; 6 bytes
 * @param[out] Data_Length Length of Data in octets
 * @param[out] Data Data field associated with Offset parameter
 * @return Value indicating success or error code.
 */
tBleStatus aci_read_config_data( uint8_t Offset,
                                 uint8_t* Data_Length,
                                 uint8_t* Data );


#endif /* BLE_GEN_ACI_H__ */
