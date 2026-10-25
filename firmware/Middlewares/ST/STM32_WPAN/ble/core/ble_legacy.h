/*****************************************************************************
 * @file    ble_legacy.h
 *
 * @brief   This file contains legacy definitions used for BLE.
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

#ifndef BLE_LEGACY_H__
#define BLE_LEGACY_H__


/* Various obsolete definitions
 */

#define PERIPHERAL_PRIVACY_FLAG_UUID             0x2A02U
#define RECONNECTION_ADDR_UUID                   0x2A03U

#define OOB_AUTH_DATA_ABSENT                       0x00U
#define OOB_AUTH_DATA_PRESENT                      0x01U

#define BLE_STATUS_SEC_DB_FULL                     0x5DU
#define BLE_STATUS_INSUFFICIENT_ENC_KEYSIZE        0x5FU
#define BLE_STATUS_CHARAC_ALREADY_EXISTS           0x63U

#define GAP_NAME_DISCOVERY_PROC                    0x04U

#define MITM_PROTECTION_REQUIRED                   0x01U

#define HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE         0xFFU

/* Deprecated names for ACI/HCI commands and events
 */

#define hci_le_read_local_supported_features \
        hci_le_read_local_supported_features_page_0
#define hci_le_read_remote_features \
        hci_le_read_remote_features_page_0

#define aci_gap_configure_whitelist \
        aci_gap_configure_filter_accept_list
#define aci_gap_slave_security_req \
        aci_gap_peripheral_security_req
#define aci_hal_set_slave_latency \
        aci_hal_set_peripheral_latency
#define aci_gap_slave_security_initiated_event \
        aci_gap_peripheral_security_initiated_event

typedef __PACKED_STRUCT
{
  /**
   * Identity address type
   * Values:
   * - 0x00: Public Identity Address
   * - 0x01: Random (static) Identity Address
   */
  uint8_t Peer_Identity_Address_Type;
  /**
   * Public or Random (static) Identity Address of the peer device
   */
  uint8_t Peer_Identity_Address[6];
} Identity_Entry_t;

#define Whitelist_Entry_t \
        Peer_Entry_t
#define Whitelist_Identity_Entry_t \
        Identity_Entry_t

#define HCI_LE_READ_REMOTE_FEATURES_COMPLETE_SUBEVT_CODE \
        HCI_LE_READ_REMOTE_FEATURES_PAGE_0_COMPLETE_SUBEVT_CODE

#define hci_le_read_remote_features_complete_event_rp0 \
        hci_le_read_remote_features_page_0_complete_event_rp0

#define ACI_GAP_SLAVE_SECURITY_INITIATED_VSEVT_CODE \
        ACI_GAP_PERIPHERAL_SECURITY_INITIATED_VSEVT_CODE

#define ACI_HAL_FW_ERROR_VSEVT_CODE \
        ACI_WARNING_VSEVT_CODE

#define ACI_HAL_WARNING_VSEVT_CODE \
        ACI_WARNING_VSEVT_CODE

typedef __PACKED_STRUCT
{
  uint8_t FW_Error_Type;
  uint8_t Data_Length;
  uint8_t Data[(BLE_EVT_MAX_PARAM_LEN - 2) - 2];
} aci_hal_fw_error_event_rp0;

#define aci_hal_warning_event_rp0 \
        aci_warning_event_rp0

#define aci_hal_warning_event \
        aci_warning_event

/* Other deprecated names
 */

#define HCI_ADV_FILTER_WHITELIST_SCAN \
        HCI_ADV_FILTER_ACC_LIST_USED_FOR_SCAN
#define HCI_ADV_FILTER_WHITELIST_CONNECT \
        HCI_ADV_FILTER_ACC_LIST_USED_FOR_CONNECT
#define HCI_ADV_FILTER_WHITELIST_SCAN_CONNECT \
        HCI_ADV_FILTER_ACC_LIST_USED_FOR_ALL
#define NO_WHITE_LIST_USE \
        HCI_ADV_FILTER_NO
#define WHITE_LIST_FOR_ONLY_SCAN \
        HCI_ADV_FILTER_ACC_LIST_USED_FOR_SCAN
#define WHITE_LIST_FOR_ONLY_CONN \
        HCI_ADV_FILTER_ACC_LIST_USED_FOR_CONNECT
#define WHITE_LIST_FOR_ALL \
        HCI_ADV_FILTER_ACC_LIST_USED_FOR_ALL

#define HCI_SCAN_FILTER_WHITELIST \
        HCI_SCAN_FILTER_ACC_LIST_USED
#define HCI_SCAN_FILTER_NO_EVEN_RPA  \
        HCI_SCAN_FILTER_NO_EXT
#define HCI_SCAN_FILTER_WHITELIST_BUT_RPA \
        HCI_SCAN_FILTER_ACC_LIST_USED_EXT

#define HCI_INIT_FILTER_WHITELIST \
        HCI_INIT_FILTER_ACC_LIST_USED

#define AD_TYPE_SLAVE_CONN_INTERVAL \
        AD_TYPE_PERIPHERAL_CONN_INTERVAL

#define OOB_NOT_AVAILABLE                 REASON_OOB_NOT_AVAILABLE
#define AUTH_REQ_CANNOT_BE_MET            REASON_AUTHENTICATION_REQ
#define CONFIRM_VALUE_FAILED              REASON_CONFIRM_VALUE_FAILED
#define PAIRING_NOT_SUPPORTED             REASON_PAIRING_NOT_SUPPORTED
#define INSUFF_ENCRYPTION_KEY_SIZE        REASON_ENCRYPTION_KEY_SIZE
#define CMD_NOT_SUPPORTED                 REASON_COMMAND_NOT_SUPPORTED
#define UNSPECIFIED_REASON                REASON_UNSPECIFIED_REASON
#define VERY_EARLY_NEXT_ATTEMPT           REASON_REPEATED_ATTEMPTS
#define SM_INVALID_PARAMS                 REASON_INVALID_PARAMETERS
#define SMP_SC_DHKEY_CHECK_FAILED         REASON_DHKEY_CHECK_FAILED
#define SMP_SC_NUMCOMPARISON_FAILED       REASON_NUM_COMPARISON_FAILED

#define CONFIG_DATA_PUBADDR_OFFSET        CONFIG_DATA_PUBLIC_ADDRESS_OFFSET
#define CONFIG_DATA_PUBADDR_LEN           CONFIG_DATA_PUBLIC_ADDRESS_LEN

#define FW_L2CAP_RECOMBINATION_ERROR                 0x01U
#define FW_GATT_UNEXPECTED_PEER_MESSAGE              0x02U
#define FW_NVM_LEVEL_WARNING                         0x03U
#define FW_COC_RX_DATA_LENGTH_TOO_LARGE              0x04U
#define FW_ECOC_CONN_RSP_ALREADY_ASSIGNED_DCID       0x05U

/* Deprecated commands
 */

#define aci_gatt_read_long_char_desc\
        aci_gatt_read_long_char_value

#define aci_gatt_read_char_desc \
        aci_gatt_read_char_value

#define aci_gatt_write_long_char_desc \
        aci_gatt_write_long_char_value

#define aci_gatt_write_char_desc \
        aci_gatt_write_char_value

#define aci_gatt_write_resp \
        aci_gatt_permit_write

/**
 * @brief ACI_GAP_RESOLVE_PRIVATE_ADDR
 * This command tries to resolve the address provided with the IRKs present in
 * its database. If the address is resolved successfully with any one of the
 * IRKs present in the database, it returns success and also the corresponding
 * public/static random address stored with the IRK in the database.
 *
 * @param Address Address to be resolved
 * @param[out] Actual_Address The public or static random address of the peer
 *        device, distributed during pairing phase.
 * @return Value indicating success or error code.
 */
__STATIC_INLINE
tBleStatus aci_gap_resolve_private_addr( const uint8_t* Address,
                                         uint8_t* Actual_Address )
{
  uint8_t type;
  return aci_gap_check_bonded_device( 1, Address, &type, Actual_Address );
}

/**
 * @brief ACI_GAP_IS_DEVICE_BONDED
 * The command finds whether the device, whose address is specified in the
 * command, is present in the bonding table. If the device is found, the
 * command returns "Success".
 * Note: the specified address can be a RPA. In this case, even if privacy is
 * not enabled, this address is resolved to check the presence of the peer
 * device in the bonding table.
 *
 * @param Peer_Address_Type The address type of the peer device.
 *        Values:
 *        - 0x00: Public Device Address
 *        - 0x01: Random Device Address
 * @param Peer_Address Public Device Address or Random Device Address of the
 *        peer device
 * @return Value indicating success or error code.
 */
__STATIC_INLINE
tBleStatus aci_gap_is_device_bonded( uint8_t Peer_Address_Type,
                                     const uint8_t* Peer_Address )
{
  uint8_t type, address[6];
  return aci_gap_check_bonded_device( Peer_Address_Type, Peer_Address,
                                      &type, address );
}

/**
 * @brief ACI_GAP_ADD_DEVICES_TO_RESOLVING_LIST
 * This  command is used to add devices to the list of address translations
 * used to resolve Resolvable Private Addresses in the Controller.
 *
 * @param Num_of_Resolving_list_Entries Number of devices that have to be added
 *        to the list.
 * @param Identity_Entry See @ref Identity_Entry_t
 * @param Clear_Resolving_List Clear the resolving list
 *        Values:
 *        - 0x00: Do not clear
 *        - 0x01: Clear before adding
 * @return Value indicating success or error code.
 */
__STATIC_INLINE
tBleStatus aci_gap_add_devices_to_resolving_list( uint8_t Num_of_Resolving_list_Entries,
                                                  const Identity_Entry_t* Identity_Entry,
                                                  uint8_t Clear_Resolving_List )
{
  return aci_gap_add_devices_to_list( Num_of_Resolving_list_Entries,
                                      (const List_Entry_t*)Identity_Entry,
                                      Clear_Resolving_List );
}

/**
 * @brief ACI_HAL_GET_FW_BUILD_NUMBER
 * This command returns the build number associated with the firmware version
 * currently running
 *
 * @param[out] Build_Number Build number of the firmware.
 * @return Value indicating success or error code.
 */
__STATIC_INLINE
tBleStatus aci_hal_get_fw_build_number( uint16_t* Build_Number )
{
  uint32_t version[2], options[1], debug_info[3];
  tBleStatus status = aci_get_information( version, options, debug_info );
  *Build_Number = (uint16_t)(version[1] >> 16);
  return status;
}

/**
 * @brief ACI_HAL_GET_PM_DEBUG_INFO
 * This command is used to retrieve TX, RX and total buffer count allocated for
 * ACL packets.
 *
 * @param[out] Allocated_For_TX MBlocks allocated for TXing
 * @param[out] Allocated_For_RX MBlocks allocated for RXing
 * @param[out] Allocated_MBlocks Overall allocated MBlocks
 * @return Value indicating success or error code.
 */
__STATIC_INLINE
tBleStatus aci_hal_get_pm_debug_info( uint8_t* Allocated_For_TX,
                                      uint8_t* Allocated_For_RX,
                                      uint8_t* Allocated_MBlocks )
{
  uint32_t version[2], options[1], debug_info[3];
  tBleStatus status = aci_get_information( version, options, debug_info );
  *Allocated_For_TX = ((uint8_t)(((uint16_t*)debug_info)[2]) +
                       (uint8_t)(((uint16_t*)debug_info)[3]));
  *Allocated_For_RX = (uint8_t)(((uint16_t*)debug_info)[1]);
  *Allocated_MBlocks = (*Allocated_For_TX) + (*Allocated_For_RX);
  return status;
}

/**
 * @brief ACI_HAL_STACK_RESET
 * This command is equivalent to HCI_RESET but ensures the sleep mode is
 * entered immediately after its completion.
 *
 * @return Value indicating success or error code.
 */
__STATIC_INLINE
tBleStatus aci_hal_stack_reset( void )
{
  return aci_reset( 0, 0 );
}

/**
 * @brief ACI_GATT_ALLOW_READ
 * Allow the GATT server to send a response to a read request from a client.
 * The application has to send this command when it receives the
 * ACI_GATT_READ_PERMIT_REQ_EVENT or ACI_GATT_READ_MULTI_PERMIT_REQ_EVENT. This
 * command indicates to the stack that the response can be sent to the client.
 * So if the application wishes to update any of the attributes before they are
 * read by the client, it must update the characteristic values using the
 * ACI_GATT_UPDATE_CHAR_VALUE and then give this command. The application
 * should perform the required operations within 30 seconds. Otherwise the GATT
 * procedure will be timeout.
 *
 * @param Connection_Handle Specifies the ATT bearer for which the command
 *        applies.
 *        Values:
 *        - 0x0000 ... 0x0EFF: Unenhanced ATT bearer (the parameter is the
 *          connection handle)
 *        - 0xEA00 ... 0xEA3F: Enhanced ATT bearer (the LSB-byte of the
 *          parameter is the connection-oriented channel index)
 * @return Value indicating success or error code.
 */
__STATIC_INLINE
tBleStatus aci_gatt_allow_read( uint16_t Connection_Handle )
{
  return aci_gatt_permit_read( Connection_Handle, 0, 0, 0 );
}

/**
 * @brief ACI_GATT_DENY_READ
 * This command is used to deny the GATT server to send a response to a read
 * request from a client.
 * The application may send this command when it receives the
 * ACI_GATT_READ_PERMIT_REQ_EVENT or ACI_GATT_READ_MULTI_PERMIT_REQ_EVENT.
 * This command indicates to the stack that the client is not allowed to read
 * the requested characteristic due to e.g. application restrictions.
 * The Error code shall be either 0x08 (Insufficient Authorization) or a value
 * in the range 0x80-0x9F (Application Error).
 * The application should issue the ACI_GATT_DENY_READ  or ACI_GATT_ALLOW_READ
 * command within 30 seconds from the reception of the
 * ACI_GATT_READ_PERMIT_REQ_EVENT or ACI_GATT_READ_MULTI_PERMIT_REQ_EVENT
 * events; otherwise the GATT procedure issues a timeout.
 *
 * @param Connection_Handle Specifies the ATT bearer for which the command
 *        applies.
 *        Values:
 *        - 0x0000 ... 0x0EFF: Unenhanced ATT bearer (the parameter is the
 *          connection handle)
 *        - 0xEA00 ... 0xEA3F: Enhanced ATT bearer (the LSB-byte of the
 *          parameter is the connection-oriented channel index)
 * @param Error_Code Error code for the command
 *        Values:
 *        - 0x08: Insufficient Authorization
 *        - 0x80 ... 0x9F: Application Error
 * @return Value indicating success or error code.
 */
__STATIC_INLINE
tBleStatus aci_gatt_deny_read( uint16_t Connection_Handle,
                               uint8_t Error_Code )
{
  return aci_gatt_permit_read( Connection_Handle, 1, Error_Code, 0 );
}


#endif /* BLE_LEGACY_H__ */
