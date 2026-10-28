/*****************************************************************************
 * @file    ble_gap_aci.h
 * @brief   STM32WB BLE API (GAP_ACI)
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

#ifndef BLE_GAP_ACI_H__
#define BLE_GAP_ACI_H__


#include "auto/ble_types.h"

/**
 * @brief ACI_GAP_SET_NON_DISCOVERABLE
 * This command stops advertising.
 * Note: this command only supports legacy advertising. For extended
 * advertising, refer to ACI_GAP_ADV_SET_ENABLE.
 *
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_set_non_discoverable( void );

/**
 * @brief ACI_GAP_SET_LIMITED_DISCOVERABLE
 * Puts the device in limited discoverable mode (as defined in Core
 * Specification [Vol 3, Part C, 9.2.3]). The device will be discoverable for
 * maximum period of TGAP (lim_adv_timeout) = 180 seconds (from errata). The
 * advertising can be disabled at any time by issuing
 * ACI_GAP_SET_NON_DISCOVERABLE command.
 * The Adv_Interval_Min and Adv_Interval_Max parameters are optional. If both
 * are set to 0, the GAP will use default values for adv intervals for limited
 * discoverable mode (250 ms and 500 ms respectively).
 * To allow a fast connection, the host can set Local_Name, Service_Uuid_List,
 * Conn_Interval_Min and Conn_Interval_Max. If provided, these data will be
 * inserted into the advertising packet payload as AD data. These parameters
 * are optional in this command. These values can be set in advertised data
 * using ACI_GAP_UPDATE_ADV_DATA command separately.
 * The total size of data in advertising packet cannot exceed 31 bytes.
 * With this command, the BLE Stack will also add automatically the following
 * standard AD types:
 * - AD Flags
 * - Power Level
 * When advertising timeout happens (i.e. limited discovery period has
 * elapsed), controller generates ACI_GAP_LIMITED_DISCOVERABLE_EVENT event.
 * Note: this command only supports legacy advertising For extended
 * advertising, refer to ACI_GAP_ADV_SET_CONFIGURATION and
 * ACI_GAP_ADV_SET_ENABLE..
 *
 * @param Advertising_Type Advertising type
 *        Values:
 *        - 0x00: ADV_IND (Connectable undirected advertising)
 *        - 0x02: ADV_SCAN_IND (Scannable undirected advertising)
 *        - 0x03: ADV_NONCONN_IND (Non connectable undirected advertising)
 * @param Advertising_Interval_Min Minimum advertising interval.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x0020 (20.000 ms)  ... 0x4000 (10240.000 ms)
 * @param Advertising_Interval_Max Maximum advertising interval.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x0020 (20.000 ms)  ... 0x4000 (10240.000 ms)
 * @param Own_Address_Type Own address type: if Privacy is disabled, the
 *        address can be public or static random; otherwise, it can be a
 *        resolvable private address or a non-resolvable private address.
 *        Values:
 *        - 0x00: Public address
 *        - 0x01: Static random address
 *        - 0x02: Resolvable private address
 *        - 0x03: Non-resolvable private address
 * @param Advertising_Filter_Policy Advertising filter policy: not applicable
 *        (the value of Advertising_Filter_Policy parameter is not used inside
 *        the Stack)
 * @param Local_Name_Length Length of the local_name field in octets.
 *        If length is set to 0x00, Local_Name parameter is not used.
 * @param Local_Name Local name of the device. First byte must be 0x08 for
 *        Shortened Local Name or 0x09 for Complete Local Name. No NULL
 *        character at the end.
 * @param Service_Uuid_length Length of the Service Uuid List in octets.
 *        If there is no service to be advertised, set this field to 0x00.
 * @param Service_Uuid_List This is the list of the UUIDs as defined in Volume
 *        3, Section 11 of GAP Specification. First byte is the AD Type.
 * @param Conn_Interval_Min Connection interval minimum value suggested by
 *        Peripheral.
 *        If Conn_Interval_Min and Conn_Interval_Max are not 0x0000, Peripheral
 *        Connection Interval Range AD structure will be added in advertising
 *        data.
 *        Connection interval is defined in the following manner:
 *        connIntervalmin = Conn_Interval_Min x 1.25ms.
 *        Values:
 *        - 0x0000 (NaN)
 *        - 0xFFFF (NaN) : No specific minimum
 *        - 0x0006 (7.50 ms)  ... 0x0C80 (4000.00 ms)
 * @param Conn_Interval_Max Connection interval maximum value suggested by
 *        Peripheral.
 *        If Conn_Interval_Min and Conn_Interval_Max are not 0x0000, Peripheral
 *        Connection Interval Range AD structure will be added in advertising
 *        data.
 *        Connection interval is defined in the following manner:
 *        connIntervalmax = Conn_Interval_Max x 1.25ms
 *        Values:
 *        - 0x0000 (NaN)
 *        - 0xFFFF (NaN) : No specific maximum
 *        - 0x0006 (7.50 ms)  ... 0x0C80 (4000.00 ms)
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_set_limited_discoverable( uint8_t Advertising_Type,
                                             uint16_t Advertising_Interval_Min,
                                             uint16_t Advertising_Interval_Max,
                                             uint8_t Own_Address_Type,
                                             uint8_t Advertising_Filter_Policy,
                                             uint8_t Local_Name_Length,
                                             const uint8_t* Local_Name,
                                             uint8_t Service_Uuid_length,
                                             const uint8_t* Service_Uuid_List,
                                             uint16_t Conn_Interval_Min,
                                             uint16_t Conn_Interval_Max );

/**
 * @brief ACI_GAP_SET_DISCOVERABLE
 * Puts the device in general discoverable mode (as defined in Core
 * Specification [Vol 3, Part C, 9.2.4]). The device will be discoverable until
 * the host issues the ACI_GAP_SET_NON_DISCOVERABLE command. The
 * Adv_Interval_Min and Adv_Interval_Max parameters are optional. If both are
 * set to 0, the GAP uses the default values for adv intervals for general
 * discoverable mode.
 * When using connectable undirected advertising events:
 * - Adv_Interval_Min = 30 ms
 * - Adv_Interval_Max = 60 ms
 * When using non-connectable advertising events or scannable undirected
 * advertising events:
 * - Adv_Interval_Min = 100 ms
 * - Adv_Interval_Max = 150 ms
 * Host can set the Local Name, a Service UUID list and the Peripheral
 * Connection Interval Range.
 * If provided, these data will be inserted into the advertising packet payload
 * as AD data.
 * These parameters are optional in this command. These values can be also set
 * using ACI_GAP_UPDATE_ADV_DATA command separately.
 * The total size of data in advertising packet cannot exceed 31 bytes.
 * With this command, the BLE Stack will also add automatically the following
 * standard AD types:
 * - AD Flags
 * - TX Power Level
 * Note: this command only supports legacy advertising. For extended
 * advertising, refer to ACI_GAP_ADV_SET_CONFIGURATION and
 * ACI_GAP_ADV_SET_ENABLE.
 *
 * @param Advertising_Type Advertising type
 *        Values:
 *        - 0x00: ADV_IND (Connectable undirected advertising)
 *        - 0x02: ADV_SCAN_IND (Scannable undirected advertising)
 *        - 0x03: ADV_NONCONN_IND (Non connectable undirected advertising)
 * @param Advertising_Interval_Min Minimum advertising interval.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x0020 (20.000 ms)  ... 0x4000 (10240.000 ms)
 * @param Advertising_Interval_Max Maximum advertising interval.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x0020 (20.000 ms)  ... 0x4000 (10240.000 ms)
 * @param Own_Address_Type Own address type: if Privacy is disabled, the
 *        address can be public or static random; otherwise, it can be a
 *        resolvable private address or a non-resolvable private address.
 *        Values:
 *        - 0x00: Public address
 *        - 0x01: Static random address
 *        - 0x02: Resolvable private address
 *        - 0x03: Non-resolvable private address
 * @param Advertising_Filter_Policy Advertising filter policy: not applicable
 *        (the value of Advertising_Filter_Policy parameter is not used inside
 *        the Stack)
 * @param Local_Name_Length Length of the local_name field in octets.
 *        If length is set to 0x00, Local_Name parameter is not used.
 * @param Local_Name Local name of the device. First byte must be 0x08 for
 *        Shortened Local Name or 0x09 for Complete Local Name. No NULL
 *        character at the end.
 * @param Service_Uuid_length Length of the Service Uuid List in octets.
 *        If there is no service to be advertised, set this field to 0x00.
 * @param Service_Uuid_List This is the list of the UUIDs as defined in Volume
 *        3, Section 11 of GAP Specification. First byte is the AD Type.
 * @param Conn_Interval_Min Connection interval minimum value suggested by
 *        Peripheral.
 *        If Conn_Interval_Min and Conn_Interval_Max are not 0x0000, Peripheral
 *        Connection Interval Range AD structure will be added in advertising
 *        data.
 *        Connection interval is defined in the following manner:
 *        connIntervalmin = Conn_Interval_Min x 1.25ms.
 *        Values:
 *        - 0x0000 (NaN)
 *        - 0xFFFF (NaN) : No specific minimum
 *        - 0x0006 (7.50 ms)  ... 0x0C80 (4000.00 ms)
 * @param Conn_Interval_Max Connection interval maximum value suggested by
 *        Peripheral.
 *        If Conn_Interval_Min and Conn_Interval_Max are not 0x0000, Peripheral
 *        Connection Interval Range AD structure will be added in advertising
 *        data.
 *        Connection interval is defined in the following manner:
 *        connIntervalmax = Conn_Interval_Max x 1.25ms
 *        Values:
 *        - 0x0000 (NaN)
 *        - 0xFFFF (NaN) : No specific maximum
 *        - 0x0006 (7.50 ms)  ... 0x0C80 (4000.00 ms)
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_set_discoverable( uint8_t Advertising_Type,
                                     uint16_t Advertising_Interval_Min,
                                     uint16_t Advertising_Interval_Max,
                                     uint8_t Own_Address_Type,
                                     uint8_t Advertising_Filter_Policy,
                                     uint8_t Local_Name_Length,
                                     const uint8_t* Local_Name,
                                     uint8_t Service_Uuid_length,
                                     const uint8_t* Service_Uuid_List,
                                     uint16_t Conn_Interval_Min,
                                     uint16_t Conn_Interval_Max );

/**
 * @brief ACI_GAP_SET_DIRECT_CONNECTABLE
 * Sets the device in directed connectable mode (as defined in Core
 * Specification [Vol 3, Part C, 9.3.3]). In this mode, the device advertises
 * using high duty cycle connectable directed advertising events or low duty
 * cycle connectable directed advertising events.
 * The device's own address used in advertising packets is defined by the
 * Own_Address_Type parameter depending on whether privacy is enabled or not.
 * When using high duty cycle connectable directed advertising events, the
 * device stays in directed connectable mode only for 1.28 seconds. If no
 * connection is established within this duration, the device enters non
 * discoverable mode and advertising has to be again enabled explicitly.
 * The controller generates a HCI_LE_CONNECTION_COMPLETE_EVENT event with the
 * status set to HCI_ADVERTISING_TIMEOUT_ERR_CODE if the connection was not
 * established and BLE_STATUS_SUCCESS (0x00) if the connection was successfully
 * established.
 * Note: this command only supports legacy advertising. For extended
 * advertising, refer to ACI_GAP_ADV_SET_CONFIGURATION and
 * ACI_GAP_ADV_SET_ENABLE.
 *
 * @param Own_Address_Type Own address type: if Privacy is disabled, the
 *        address can be public or static random; otherwise, it can be a
 *        resolvable private address.
 *        Values:
 *        - 0x00: Public address
 *        - 0x01: Static random address
 *        - 0x02: Resolvable private address
 * @param Directed_Advertising_Type Advertising type
 *        Values:
 *        - 0x01: High Duty Cycle Directed Advertising
 *        - 0x04: Low Duty Cycle Directed Advertising
 * @param Direct_Address_Type The address type of the peer device.
 *        Values:
 *        - 0x00: Public Device Address
 *        - 0x01: Random Device Address
 * @param Direct_Address Initiator address
 * @param Advertising_Interval_Min Minimum advertising interval.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x0006 (3.750 ms) : for High Duty Cycle Directed Advertising
 *        - 0x0020 (20.000 ms)  ... 0x4000 (10240.000 ms) : for Low Duty Cycle
 *          Directed Advertising
 * @param Advertising_Interval_Max Maximum advertising interval.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x0006 (3.750 ms) : for High Duty Cycle Directed Advertising
 *        - 0x0020 (20.000 ms)  ... 0x4000 (10240.000 ms) : for Low Duty Cycle
 *          Directed Advertising
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_set_direct_connectable( uint8_t Own_Address_Type,
                                           uint8_t Directed_Advertising_Type,
                                           uint8_t Direct_Address_Type,
                                           const uint8_t* Direct_Address,
                                           uint16_t Advertising_Interval_Min,
                                           uint16_t Advertising_Interval_Max );

/**
 * @brief ACI_GAP_SET_IO_CAPABILITY
 * Sets the IO capabilities of the device. This command has to be given only
 * when the device is not in a connected state.
 *
 * @param IO_Capability IO capability of the device.
 *        Values:
 *        - 0x00: IO_CAP_DISPLAY_ONLY
 *        - 0x01: IO_CAP_DISPLAY_YES_NO
 *        - 0x02: IO_CAP_KEYBOARD_ONLY
 *        - 0x03: IO_CAP_NO_INPUT_NO_OUTPUT
 *        - 0x04: IO_CAP_KEYBOARD_DISPLAY
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_set_io_capability( uint8_t IO_Capability );

/**
 * @brief ACI_GAP_SET_AUTHENTICATION_REQUIREMENT
 * Sets the authentication requirements for the device. This command has to be
 * given only when the device is not in a connected state.
 *
 * @param Bonding_Mode Bonding mode.
 *        Only if bonding is enabled (0x01), the bonding information is stored
 *        in flash
 *        Values:
 *        - 0x00: No-bonding mode
 *        - 0x01: Bonding mode
 * @param MITM_Mode MITM mode.
 *        Values:
 *        - 0x00: MITM protection not required
 *        - 0x01: MITM protection required as mandatory
 *        - 0x02: MITM protection required as optional
 * @param SC_Support LE Secure connections support
 *        Values:
 *        - 0x00: Secure Connections Pairing not supported
 *        - 0x01: Secure Connections Pairing supported but optional
 *        - 0x02: Secure Connections Pairing supported and mandatory (SC Only
 *          Mode)
 * @param KeyPress_Notification_Support Keypress notification support
 *        Values:
 *        - 0x00: Keypress notification not supported
 *        - 0x01: Keypress notification supported
 * @param Min_Encryption_Key_Size Minimum encryption key size to be used during
 *        pairing.
 * @param Max_Encryption_Key_Size Maximum encryption key size to be used during
 *        pairing.
 * @param Use_Fixed_Pin Use or not fixed pin.
 *        If set to 0x00, then during the pairing process the application will
 *        not be requested for a pin (Fixed_Pin will be used). If set to 0x01,
 *        then during pairing process if a passkey is required the application
 *        will be notified
 *        Values:
 *        - 0x00: use a fixed pin (deprecated option that should not be used)
 *        - 0x01: do not use a fixed pin
 * @param Fixed_Pin Fixed pin to be used during pairing (deprecated parameter).
 *        Values:
 *        - 0 ... 999999
 * @param Identity_Address_Type Identity address type
 *        Values:
 *        - 0x00: Public Identity Address
 *        - 0x01: Random (static) Identity Address
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_set_authentication_requirement( uint8_t Bonding_Mode,
                                                   uint8_t MITM_Mode,
                                                   uint8_t SC_Support,
                                                   uint8_t KeyPress_Notification_Support,
                                                   uint8_t Min_Encryption_Key_Size,
                                                   uint8_t Max_Encryption_Key_Size,
                                                   uint8_t Use_Fixed_Pin,
                                                   uint32_t Fixed_Pin,
                                                   uint8_t Identity_Address_Type );

/**
 * @brief ACI_GAP_SET_AUTHORIZATION_REQUIREMENT
 * Sets the authorization requirements of the device. This command has to be
 * given when connected to a device if authorization is required to access
 * services which require authorization.
 *
 * @param Connection_Handle Connection handle for which the command applies.
 *        Values:
 *        - 0x0000 ... 0x0EFF
 * @param Authorization_Enable Enable the authorization in the device and when
 *        a remote device tries to read/write a characteristic with
 *        authorization requirements, the stack will send back an error
 *        response with "Insufficient authorization" error code. After pairing
 *        is complete an ACI_GAP_AUTHORIZATION_REQ_EVENT will be sent to the
 *        Host.
 *        Values:
 *        - 0x00: Authorization not required
 *        - 0x01: Authorization required
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_set_authorization_requirement( uint16_t Connection_Handle,
                                                  uint8_t Authorization_Enable );

/**
 * @brief ACI_GAP_PASS_KEY_RESP
 * This command should be sent by the host in response to
 * ACI_GAP_PASS_KEY_REQ_EVENT event. The command parameter contains the pass
 * key which will be used during the pairing process.
 *
 * @param Connection_Handle Connection handle for which the command applies.
 *        Values:
 *        - 0x0000 ... 0x0EFF
 * @param Pass_Key Pass key that will be used during the pairing process.
 *        Must be a six-digit decimal number.
 *        Values:
 *        - 0 ... 999999
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_pass_key_resp( uint16_t Connection_Handle,
                                  uint32_t Pass_Key );

/**
 * @brief ACI_GAP_AUTHORIZATION_RESP
 * Authorizes a device to access attributes. This command should be sent by the
 * host in response to ACI_GAP_AUTHORIZATION_REQ_EVENT event.
 *
 * @param Connection_Handle Connection handle for which the command applies.
 *        Values:
 *        - 0x0000 ... 0x0EFF
 * @param Authorize Authorization response.
 *        Values:
 *        - 0x01: Authorize
 *        - 0x02: Reject
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_authorization_resp( uint16_t Connection_Handle,
                                       uint8_t Authorize );

/**
 * @brief ACI_GAP_INIT
 * Initializes the GAP layer. Register the GAP service with the GATT.
 * All the standard GAP characteristics will also be added:
 * - Device Name
 * - Appearance
 * - Peripheral Preferred Connection Parameters (peripheral role only).
 * Note that if the Peripheral Preferred Connection Parameters characteristic
 * is added, its handle is equal to the Appearance characteristic handle plus
 * 2.
 * Note also that if privacy is enabled, this command automatically unmasks the
 * HCI_LE_ENHANCED_CONNECTION_COMPLETE_EVENT event.
 *
 * @param Role Bitmap of allowed roles.
 *        Flags:
 *        - 0x01: Peripheral
 *        - 0x02: Broadcaster
 *        - 0x04: Central
 *        - 0x08: Observer
 * @param privacy_enabled This parameter specifies if Privacy is enabled or
 *        not. N.B.: only Controller Privacy is supported.
 *        Values:
 *        - 0x00: Privacy disabled
 *        - 0x02: Privacy enabled
 * @param device_name_char_len Length of the device name characteristic
 * @param[out] Service_Handle Handle of the GAP service
 * @param[out] Dev_Name_Char_Handle Device Name Characteristic handle
 * @param[out] Appearance_Char_Handle Appearance Characteristic handle
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_init( uint8_t Role,
                         uint8_t privacy_enabled,
                         uint8_t device_name_char_len,
                         uint16_t* Service_Handle,
                         uint16_t* Dev_Name_Char_Handle,
                         uint16_t* Appearance_Char_Handle );

/**
 * @brief ACI_GAP_SET_NON_CONNECTABLE
 * This command starts advertising in non connectable mode (i.e., this mode
 * does not support connection).
 * Advertiser filter policy is internally set to 0 (i.e., the Filter Accept
 * List is not in use).
 * Note: this command only supports legacy advertising. For extended
 * advertising, refer to ACI_GAP_ADV_SET_CONFIGURATION and
 * ACI_GAP_ADV_SET_ENABLE.
 *
 * @param Advertising_Event_Type Advertising type
 *        Values:
 *        - 0x02: ADV_SCAN_IND (Scannable undirected advertising)
 *        - 0x03: ADV_NONCONN_IND (Non connectable undirected advertising)
 * @param Own_Address_Type Own address type: if Privacy is disabled, the
 *        address can be public or static random; otherwise, it can be a
 *        resolvable private address or a non-resolvable private address.
 *        Values:
 *        - 0x00: Public address
 *        - 0x01: Static random address
 *        - 0x02: Resolvable private address
 *        - 0x03: Non-resolvable private address
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_set_non_connectable( uint8_t Advertising_Event_Type,
                                        uint8_t Own_Address_Type );

/**
 * @brief ACI_GAP_SET_UNDIRECTED_CONNECTABLE
 * This command starts advertising in undirected connectable mode.
 * If privacy is enabled in the device, a resolvable private address is
 * generated and used as the advertiser's address. If not, the address of the
 * type specified in Own_Address_Type is used for advertising.
 * Note: this command only supports legacy advertising. For extended
 * advertising, refer to ACI_GAP_ADV_SET_CONFIGURATION and
 * ACI_GAP_ADV_SET_ENABLE.
 *
 * @param Advertising_Interval_Min Minimum advertising interval.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x0020 (20.000 ms)  ... 0x4000 (10240.000 ms)
 * @param Advertising_Interval_Max Maximum advertising interval.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x0020 (20.000 ms)  ... 0x4000 (10240.000 ms)
 * @param Own_Address_Type Own address type: if Privacy is disabled, the
 *        address can be public or static random; otherwise, it can be a
 *        resolvable private address.
 *        Values:
 *        - 0x00: Public address
 *        - 0x01: Static random address
 *        - 0x02: Resolvable private address
 * @param Adv_Filter_Policy Advertising filter policy.
 *        Values:
 *        - 0x00: Allow Scan Request from Any, Allow Connect Request from Any
 *        - 0x03: Allow Scan Request from Filter Accept List Only, Allow
 *          Connect Request from Filter Accept List Only
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_set_undirected_connectable( uint16_t Advertising_Interval_Min,
                                               uint16_t Advertising_Interval_Max,
                                               uint8_t Own_Address_Type,
                                               uint8_t Adv_Filter_Policy );

/**
 * @brief ACI_GAP_PERIPHERAL_SECURITY_REQ
 * Sends a Peripheral Security Request to the Central.
 * This command has to be issued to notify the Central of the security
 * requirements of the Peripheral. The Central may encrypt the link, initiate
 * the pairing procedure, or reject the request.
 *
 * @param Connection_Handle Connection handle for which the command applies.
 *        Values:
 *        - 0x0000 ... 0x0EFF
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_peripheral_security_req( uint16_t Connection_Handle );

/**
 * @brief ACI_GAP_UPDATE_ADV_DATA
 * This command can be used to update the advertising data for particular AD
 * types. The data argument shall consist of valid advertising data composed of
 * one or more AD types. If one of the specified AD types does not exist in the
 * current advertising data, then it is added to the advertising data; while
 * the AD types already present in the current advertising data are updated
 * with the new values. If the overall advertising data length is more than 31
 * octets after the update, then the command is rejected and the old data is
 * retained.
 * Note: this command only supports legacy advertising. For extended
 * advertising, refer to ACI_GAP_ADV_SET_ADV_DATA.
 *
 * @param AdvDataLen Length of AdvData in octets
 * @param AdvData Advertising data used by the device while advertising.
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_update_adv_data( uint8_t AdvDataLen,
                                    const uint8_t* AdvData );

/**
 * @brief ACI_GAP_DELETE_AD_TYPE
 * This command can be used to delete the specified AD type from the
 * advertisement data if present.
 * Note: this command only supports legacy advertising. For extended
 * advertising, refer to ACI_GAP_ADV_SET_ADV_DATA.
 *
 * @param ADType One of the AD types as in Core Specification [Vol 3, Part C,
 *        11].
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_delete_ad_type( uint8_t ADType );

/**
 * @brief ACI_GAP_GET_SECURITY_LEVEL
 * This command can be used to get the current security settings of the device.
 *
 * @param Connection_Handle Connection handle for which the command applies.
 *        Values:
 *        - 0x0000 ... 0x0EFF
 * @param[out] Security_Mode Security mode.
 *        Values:
 *        - 0x01: Security Mode 1
 * @param[out] Security_Level Security Level.
 *        Values:
 *        - 0x01: Security Level 1
 *        - 0x02: Security Level 2
 *        - 0x03: Security Level 3
 *        - 0x04: Security Level 4
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_get_security_level( uint16_t Connection_Handle,
                                       uint8_t* Security_Mode,
                                       uint8_t* Security_Level );

/**
 * @brief ACI_GAP_SET_EVENT_MASK
 * It allows masking events from the GAP. If the bit in the GAP_Evt_Mask is set
 * to a one, then the event associated with that bit will be enabled.
 *
 * @param GAP_Evt_Mask ACI GAP/L2CAP event mask. Default: 0xFFFF.
 *        Flags:
 *        - 0x0000: No events
 *        - 0x0001: ACI_GAP_LIMITED_DISCOVERABLE_EVENT
 *        - 0x0002: ACI_GAP_PAIRING_COMPLETE_EVENT
 *        - 0x0004: ACI_GAP_PASS_KEY_REQ_EVENT
 *        - 0x0008: ACI_GAP_AUTHORIZATION_REQ_EVENT
 *        - 0x0020: ACI_GAP_BOND_LOST_EVENT
 *        - 0x0080: ACI_GAP_PROC_COMPLETE_EVENT
 *        - 0x0100: ACI_L2CAP_CONNECTION_UPDATE_REQ_EVENT
 *        - 0x0200: ACI_L2CAP_CONNECTION_UPDATE_RESP_EVENT
 *        - 0x0400: ACI_L2CAP_PROC_TIMEOUT_EVENT
 *        - 0x0800: ACI_GAP_ADDR_NOT_RESOLVED_EVENT
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_set_event_mask( uint16_t GAP_Evt_Mask );

/**
 * @brief ACI_GAP_CONFIGURE_FILTER_ACCEPT_LIST
 * This command adds addresses of bonded devices into the controller's Filter
 * Accept List, which is cleared first. It returns an error if it was unable to
 * add all bonded devices into the Filter Accept List.
 * This command shall not be used when the device is advertising, scanning or
 * initiating with a filter policy using the Filter Accept List.
 *
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_configure_filter_accept_list( void );

/**
 * @brief ACI_GAP_TERMINATE
 * Commands the controller to terminate the connection. A
 * HCI_DISCONNECTION_COMPLETE_EVENT event is generated when the link is
 * disconnected.
 *
 * @param Connection_Handle Connection handle for which the command applies.
 *        Values:
 *        - 0x0000 ... 0x0EFF
 * @param Reason The reason for ending the connection.
 *        Values:
 *        - 0x05: Authentication Failure
 *        - 0x13: Remote User Terminated Connection
 *        - 0x14: Remote Device Terminated Connection due to Low Resources
 *        - 0x15: Remote Device Terminated Connection due to Power Off
 *        - 0x1A: Unsupported Remote Feature
 *        - 0x3B: Unacceptable Connection Parameters
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_terminate( uint16_t Connection_Handle,
                              uint8_t Reason );

/**
 * @brief ACI_GAP_CLEAR_SECURITY_DB
 * Clears the bonding table. All the devices in the bonding table are removed.
 * See also ACI_GAP_REMOVE_BONDED_DEVICE to remove only one device.
 * Note: as a fallback mode, in case the bonding table is full, the BLE stack
 * automatically clears the bonding table just before putting into it
 * information about a new bonded device.
 *
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_clear_security_db( void );

/**
 * @brief ACI_GAP_ALLOW_REBOND
 * Allows the security manager to complete the pairing procedure and re-bond
 * with the Central. This command should be given by the application when it
 * receives the ACI_GAP_BOND_LOST_EVENT if it wants the re-bonding to happen
 * successfully. If this command is not given on receiving the event, the
 * bonding procedure will timeout.
 *
 * @param Connection_Handle Connection handle for which the command applies.
 *        Values:
 *        - 0x0000 ... 0x0EFF
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_allow_rebond( uint16_t Connection_Handle );

/**
 * @brief ACI_GAP_START_LIMITED_DISCOVERY_PROC
 * Starts the limited discovery procedure. The controller is commanded to start
 * active scanning.
 * When this procedure is started, only the devices in limited discoverable
 * mode are returned to the upper layers.
 * The procedure is terminated when either the upper layers issue a command to
 * terminate the procedure by issuing the command ACI_GAP_TERMINATE_GAP_PROC
 * with the procedure code set to 0x01 or a timeout happens (the timeout value
 * is fixed at 10.24 s.). When the procedure is terminated due to any of the
 * above  reasons, ACI_GAP_PROC_COMPLETE_EVENT event is returned with the
 * procedure code set to 0x01.
 * The device found when the procedure is ongoing is returned to the upper
 * layers through the event HCI_LE_ADVERTISING_REPORT_EVENT (or via
 * HCI_LE_EXTENDED_ADVERTISING_REPORT_EVENT when the extended advertising
 * feature is supported).
 *
 * @param LE_Scan_Interval This is defined as the time interval from when the
 *        Controller started its last LE scan until it begins the subsequent LE
 *        scan.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x0004 (2.500 ms)  ... 0x4000 (10240.000 ms)
 * @param LE_Scan_Window Amount of time for the duration of the LE scan.
 *        LE_Scan_Window shall be less than or equal to LE_Scan_Interval.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x0004 (2.500 ms)  ... 0x4000 (10240.000 ms)
 * @param Own_Address_Type Own address type: if Privacy is disabled, the
 *        address can be public or static random; otherwise, it can be a
 *        resolvable private address or a non-resolvable private address.
 *        Values:
 *        - 0x00: Public address
 *        - 0x01: Static random address
 *        - 0x02: Resolvable private address
 *        - 0x03: Non-resolvable private address
 * @param Filter_Duplicates Enable/disable duplicate filtering.
 *        Values:
 *        - 0x00: Duplicate filtering disabled
 *        - 0x01: Duplicate filtering enabled
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_start_limited_discovery_proc( uint16_t LE_Scan_Interval,
                                                 uint16_t LE_Scan_Window,
                                                 uint8_t Own_Address_Type,
                                                 uint8_t Filter_Duplicates );

/**
 * @brief ACI_GAP_START_GENERAL_DISCOVERY_PROC
 * Starts the general discovery procedure. The controller is commanded to start
 * active scanning. The procedure is terminated when  either the upper layers
 * issue a command to terminate the procedure by issuing the command
 * ACI_GAP_TERMINATE_GAP_PROC with the procedure code set to 0x02 or a timeout
 * happens (the timeout value is fixed at 10.24 s.). When the procedure is
 * terminated due to any of the above reasons, ACI_GAP_PROC_COMPLETE_EVENT
 * event is returned with the procedure code set to 0x02.
 * The devices found when the procedure is ongoing are returned via
 * HCI_LE_ADVERTISING_REPORT_EVENT (or via
 * HCI_LE_EXTENDED_ADVERTISING_REPORT_EVENT when the extended advertising
 * feature is supported).
 *
 * @param LE_Scan_Interval This is defined as the time interval from when the
 *        Controller started its last LE scan until it begins the subsequent LE
 *        scan.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x0004 (2.500 ms)  ... 0x4000 (10240.000 ms)
 * @param LE_Scan_Window Amount of time for the duration of the LE scan.
 *        LE_Scan_Window shall be less than or equal to LE_Scan_Interval.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x0004 (2.500 ms)  ... 0x4000 (10240.000 ms)
 * @param Own_Address_Type Own address type: if Privacy is disabled, the
 *        address can be public or static random; otherwise, it can be a
 *        resolvable private address or a non-resolvable private address.
 *        Values:
 *        - 0x00: Public address
 *        - 0x01: Static random address
 *        - 0x02: Resolvable private address
 *        - 0x03: Non-resolvable private address
 * @param Filter_Duplicates Enable/disable duplicate filtering.
 *        Values:
 *        - 0x00: Duplicate filtering disabled
 *        - 0x01: Duplicate filtering enabled
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_start_general_discovery_proc( uint16_t LE_Scan_Interval,
                                                 uint16_t LE_Scan_Window,
                                                 uint8_t Own_Address_Type,
                                                 uint8_t Filter_Duplicates );

/**
 * @brief ACI_GAP_START_AUTO_CONNECTION_ESTABLISH_PROC
 * Starts the auto connection establishment procedure. The devices specified
 * are added to the Filter Accept List of the controller and a
 * LE_Create_Connection call will be made to the controller by GAP with the
 * initiator filter policy set to "Filter Accept List is used to determine
 * which advertiser to connect to". When a command is issued to terminate the
 * procedure by upper layer, a LE_Create_Connection_Cancel call will be made to
 * the controller by GAP.
 * The procedure is terminated when either a connection is successfully
 * established with one of the specified devices in the Filter Accept List or
 * the procedure is explicitly terminated by issuing the command
 * ACI_GAP_TERMINATE_GAP_PROC with the procedure code set to 0x08. A
 * ACI_GAP_PROC_COMPLETE_EVENT event is returned with the procedure code set to
 * 0x08.
 * If privacy is enabled and the peer device (advertiser) is in the resolving
 * list then the link layer generates a RPA.
 *
 * @param LE_Scan_Interval This is defined as the time interval from when the
 *        Controller started its last LE scan until it begins the subsequent LE
 *        scan.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x0004 (2.500 ms)  ... 0x4000 (10240.000 ms) : legacy advertising
 *        - 0x0004 (2.500 ms)  ... 0x5DC0 (15000.000 ms) : extended advertising
 * @param LE_Scan_Window Amount of time for the duration of the LE scan.
 *        LE_Scan_Window shall be less than or equal to LE_Scan_Interval.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x0004 (2.500 ms)  ... 0x4000 (10240.000 ms) : legacy advertising
 *        - 0x0004 (2.500 ms)  ... 0x5DC0 (15000.000 ms) : extended advertising
 * @param Own_Address_Type Own address type: if Privacy is disabled, the
 *        address can be public or static random; otherwise, it can be a
 *        resolvable private address.
 *        Values:
 *        - 0x00: Public address
 *        - 0x01: Static random address
 *        - 0x02: Resolvable private address
 * @param Conn_Interval_Min Minimum value for the connection event interval.
 *        Time = N * 1.25 ms.
 *        Values:
 *        - 0x0006 (7.50 ms)  ... 0x0C80 (4000.00 ms)
 * @param Conn_Interval_Max Maximum value for the connection event interval.
 *        Time = N * 1.25 ms.
 *        Values:
 *        - 0x0006 (7.50 ms)  ... 0x0C80 (4000.00 ms)
 * @param Conn_Latency Maximum Peripheral latency for the connection in number
 *        of connection events.
 *        Values:
 *        - 0x0000 ... 0x01F3
 * @param Supervision_Timeout Supervision timeout for the LE Link.
 *        It shall be a multiple of 10 ms and larger than (1 +
 *        connPeripheralLatency) * connInterval * 2.
 *        Time = N * 10 ms.
 *        Values:
 *        - 0x000A (100 ms)  ... 0x0C80 (32000 ms)
 * @param Minimum_CE_Length Information parameter about the minimum length of
 *        connection needed for this LE connection.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x0000 (0.000 ms)  ... 0xFFFF (40959.375 ms)
 * @param Maximum_CE_Length Information parameter about the maximum length of
 *        connection needed for this LE connection.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x0000 (0.000 ms)  ... 0xFFFF (40959.375 ms)
 * @param Num_of_Peer_Entries Number of devices that have to be added to the
 *        Filter Accept List. Each device is defined by Peer_Address_Type and
 *        Peer_Address.
 * @param Peer_Entry See @ref Peer_Entry_t
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_start_auto_connection_establish_proc( uint16_t LE_Scan_Interval,
                                                         uint16_t LE_Scan_Window,
                                                         uint8_t Own_Address_Type,
                                                         uint16_t Conn_Interval_Min,
                                                         uint16_t Conn_Interval_Max,
                                                         uint16_t Conn_Latency,
                                                         uint16_t Supervision_Timeout,
                                                         uint16_t Minimum_CE_Length,
                                                         uint16_t Maximum_CE_Length,
                                                         uint8_t Num_of_Peer_Entries,
                                                         const Peer_Entry_t* Peer_Entry );

/**
 * @brief ACI_GAP_START_GENERAL_CONNECTION_ESTABLISH_PROC
 * Starts a general connection establishment procedure. The host enables
 * scanning in the controller with the scanner filter policy set to "accept all
 * advertising packets" and from the scanning results, all the devices are sent
 * to the upper layer by the event HCI_LE_ADVERTISING_REPORT_EVENT (or by the
 * event HCI_LE_EXTENDED_ADVERTISING_REPORT_EVENT when the extended advertising
 * feature is supported). The upper layer then has to select one of the devices
 * to which it wants to connect by issuing the command
 * ACI_GAP_CREATE_CONNECTION. If privacy is enabled, then either a private
 * resolvable address or a non-resolvable address, based on the address type
 * specified in the command is set as the scanner address but the gap create
 * connection always uses a private resolvable address if the general
 * connection establishment procedure is active.
 * Before the call to ACI_GAP_CREATE_CONNECTION, the procedure can be
 * terminated by issuing the command ACI_GAP_TERMINATE_GAP_PROC with the
 * procedure code set to 0x10.
 * After the call to ACI_GAP_CREATE_CONNECTION, the procedure is terminated
 * when a connection is established, or the upper layer terminates the
 * procedure by issuing the command ACI_GAP_TERMINATE_GAP_PROC with the
 * procedure code set to 0x40. On completion of the procedure a
 * ACI_GAP_PROC_COMPLETE_EVENT event is generated with the procedure code set
 * to 0x40.
 * If privacy is enabled and the peer device (advertiser) is in the resolving
 * list then the link layer generates a RPA.
 *
 * @param LE_Scan_Type Passive or active scanning. With passive scanning, no
 *        scan request PDUs are sent.
 *        Values:
 *        - 0x00: Passive scanning
 *        - 0x01: Active scanning
 * @param LE_Scan_Interval This is defined as the time interval from when the
 *        Controller started its last LE scan until it begins the subsequent LE
 *        scan.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x0004 (2.500 ms)  ... 0x4000 (10240.000 ms) : legacy advertising
 *        - 0x0004 (2.500 ms)  ... 0x5DC0 (15000.000 ms) : extended advertising
 * @param LE_Scan_Window Amount of time for the duration of the LE scan.
 *        LE_Scan_Window shall be less than or equal to LE_Scan_Interval.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x0004 (2.500 ms)  ... 0x4000 (10240.000 ms) : legacy advertising
 *        - 0x0004 (2.500 ms)  ... 0x5DC0 (15000.000 ms) : extended advertising
 * @param Own_Address_Type Own address type: if Privacy is disabled, the
 *        address can be public or static random; otherwise, it can be a
 *        resolvable private address or a non-resolvable private address.
 *        Values:
 *        - 0x00: Public address
 *        - 0x01: Static random address
 *        - 0x02: Resolvable private address
 *        - 0x03: Non-resolvable private address
 * @param Scanning_Filter_Policy The scanning filter policy determines how the
 *        scanner's Link Layer processes advertising and scan response PDUs.
 *        There is a choice of two primary filter policies: unfiltered and
 *        filtered.
 *        Unfiltered: the Link Layer processes all advertising and scan
 *        response PDUs (i.e., the Filter Accept List is not used).
 *        Filtered: the Link Layer processes advertising and scan response PDUs
 *        only from devices in the Filter Accept List.
 *        With extended scanning filter policies, a directed advertising PDU
 *        accepted by the primary filter policy shall nevertheless be ignored
 *        unless either the TargetA field is identical to the scanner's device
 *        address, or TargetA field is a resolvable private address.
 *        Values:
 *        - 0x00: Basic unfiltered scanning filter policy
 *        - 0x01: Basic filtered scanning filter policy
 *        - 0x02: Extended unfiltered scanning filter policy
 *        - 0x03: Extended filtered scanning filter policy
 * @param Filter_Duplicates Enable/disable duplicate filtering.
 *        Values:
 *        - 0x00: Duplicate filtering disabled
 *        - 0x01: Duplicate filtering enabled
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_start_general_connection_establish_proc( uint8_t LE_Scan_Type,
                                                            uint16_t LE_Scan_Interval,
                                                            uint16_t LE_Scan_Window,
                                                            uint8_t Own_Address_Type,
                                                            uint8_t Scanning_Filter_Policy,
                                                            uint8_t Filter_Duplicates );

/**
 * @brief ACI_GAP_START_SELECTIVE_CONNECTION_ESTABLISH_PROC
 * Starts a selective connection establishment procedure. The GAP adds the
 * specified device addresses into Filter Accept List and enables scanning in
 * the controller with a scanning filter policy that should be set to
 * "filtered". All the devices found are sent to the upper layer by the event
 * HCI_LE_ADVERTISING_REPORT_EVENT (or by the event
 * HCI_LE_EXTENDED_ADVERTISING_REPORT_EVENT when the extended advertising
 * feature is supported). The upper layer then has to select one of the devices
 * to which it wants to connect by issuing the command
 * ACI_GAP_CREATE_CONNECTION.
 * Before the call to ACI_GAP_CREATE_CONNECTION, the procedure can be
 * terminated by issuing the command ACI_GAP_TERMINATE_GAP_PROC with the
 * procedure code set to 0x20.
 * After the call to ACI_GAP_CREATE_CONNECTION, the procedure is terminated
 * when a connection is established, or the upper layer terminates the
 * procedure by issuing the command ACI_GAP_TERMINATE_GAP_PROC with the
 * procedure code set to 0x40. On completion of the procedure a
 * ACI_GAP_PROC_COMPLETE_EVENT event is generated with the procedure code set
 * to 0x40.
 * If privacy is enabled and the peer device (advertiser) is in the resolving
 * list then the link layer generates a RPA.
 *
 * @param LE_Scan_Type Passive or active scanning. With passive scanning, no
 *        scan request PDUs are sent.
 *        Values:
 *        - 0x00: Passive scanning
 *        - 0x01: Active scanning
 * @param LE_Scan_Interval This is defined as the time interval from when the
 *        Controller started its last LE scan until it begins the subsequent LE
 *        scan.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x0004 (2.500 ms)  ... 0x4000 (10240.000 ms) : legacy advertising
 *        - 0x0004 (2.500 ms)  ... 0x5DC0 (15000.000 ms) : extended advertising
 * @param LE_Scan_Window Amount of time for the duration of the LE scan.
 *        LE_Scan_Window shall be less than or equal to LE_Scan_Interval.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x0004 (2.500 ms)  ... 0x4000 (10240.000 ms) : legacy advertising
 *        - 0x0004 (2.500 ms)  ... 0x5DC0 (15000.000 ms) : extended advertising
 * @param Own_Address_Type Own address type: if Privacy is disabled, the
 *        address can be public or static random; otherwise, it can be a
 *        resolvable private address or a non-resolvable private address.
 *        Values:
 *        - 0x00: Public address
 *        - 0x01: Static random address
 *        - 0x02: Resolvable private address
 *        - 0x03: Non-resolvable private address
 * @param Scanning_Filter_Policy The scanning filter policy determines how the
 *        scanner's Link Layer processes advertising and scan response PDUs.
 *        There is a choice of two primary filter policies: unfiltered and
 *        filtered.
 *        Unfiltered: the Link Layer processes all advertising and scan
 *        response PDUs (i.e., the Filter Accept List is not used).
 *        Filtered: the Link Layer processes advertising and scan response PDUs
 *        only from devices in the Filter Accept List.
 *        With extended scanning filter policies, a directed advertising PDU
 *        accepted by the primary filter policy shall nevertheless be ignored
 *        unless either the TargetA field is identical to the scanner's device
 *        address, or TargetA field is a resolvable private address.
 *        Values:
 *        - 0x00: Basic unfiltered scanning filter policy
 *        - 0x01: Basic filtered scanning filter policy
 *        - 0x02: Extended unfiltered scanning filter policy
 *        - 0x03: Extended filtered scanning filter policy
 * @param Filter_Duplicates Enable/disable duplicate filtering.
 *        Values:
 *        - 0x00: Duplicate filtering disabled
 *        - 0x01: Duplicate filtering enabled
 * @param Num_of_Peer_Entries Number of devices that have to be added to the
 *        Filter Accept List. Each device is defined by Peer_Address_Type and
 *        Peer_Address.
 * @param Peer_Entry See @ref Peer_Entry_t
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_start_selective_connection_establish_proc( uint8_t LE_Scan_Type,
                                                              uint16_t LE_Scan_Interval,
                                                              uint16_t LE_Scan_Window,
                                                              uint8_t Own_Address_Type,
                                                              uint8_t Scanning_Filter_Policy,
                                                              uint8_t Filter_Duplicates,
                                                              uint8_t Num_of_Peer_Entries,
                                                              const Peer_Entry_t* Peer_Entry );

/**
 * @brief ACI_GAP_CREATE_CONNECTION
 * Starts the direct connection establishment procedure. A LE_Create_Connection
 * call will be made to the controller by GAP with the initiator filter policy
 * set to "Filter Accept List is not used to determine which advertiser to
 * connect to". The procedure can be terminated explicitly by the upper layer
 * by issuing the command ACI_GAP_TERMINATE_GAP_PROC. When a command is issued
 * to terminate the procedure by upper layer, a HCI_LE_CREATE_CONNECTION_CANCEL
 * call will be made to the controller by GAP.
 * On termination of the procedure, a HCI_LE_CONNECTION_COMPLETE_EVENT (or
 * HCI_LE_ENHANCED_CONNECTION_COMPLETE_EVENT if privacy or extended advertising
 * is used) is returned. The procedure can be explicitly terminated by the
 * upper layer by issuing the command ACI_GAP_TERMINATE_GAP_PROC with the
 * Procedure_Code set to 0x40.
 * If privacy is enabled and the peer device (advertiser) is in the resolving
 * list then the link layer generates a RPA.
 *
 * @param LE_Scan_Interval This is defined as the time interval from when the
 *        Controller started its last LE scan until it begins the subsequent LE
 *        scan.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x0004 (2.500 ms)  ... 0x4000 (10240.000 ms) : legacy advertising
 *        - 0x0004 (2.500 ms)  ... 0x5DC0 (15000.000 ms) : extended advertising
 * @param LE_Scan_Window Amount of time for the duration of the LE scan.
 *        LE_Scan_Window shall be less than or equal to LE_Scan_Interval.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x0004 (2.500 ms)  ... 0x4000 (10240.000 ms) : legacy advertising
 *        - 0x0004 (2.500 ms)  ... 0x5DC0 (15000.000 ms) : extended advertising
 * @param Peer_Address_Type The address type of the peer device.
 *        Values:
 *        - 0x00: Public Device Address
 *        - 0x01: Random Device Address
 * @param Peer_Address Public Device Address or Random Device Address of the
 *        device to be connected.
 * @param Own_Address_Type Own address type: if Privacy is disabled, the
 *        address can be public or static random; otherwise, it can be a
 *        resolvable private address.
 *        Values:
 *        - 0x00: Public address
 *        - 0x01: Static random address
 *        - 0x02: Resolvable private address
 * @param Conn_Interval_Min Minimum value for the connection event interval.
 *        Time = N * 1.25 ms.
 *        Values:
 *        - 0x0006 (7.50 ms)  ... 0x0C80 (4000.00 ms)
 * @param Conn_Interval_Max Maximum value for the connection event interval.
 *        Time = N * 1.25 ms.
 *        Values:
 *        - 0x0006 (7.50 ms)  ... 0x0C80 (4000.00 ms)
 * @param Conn_Latency Maximum Peripheral latency for the connection in number
 *        of connection events.
 *        Values:
 *        - 0x0000 ... 0x01F3
 * @param Supervision_Timeout Supervision timeout for the LE Link.
 *        It shall be a multiple of 10 ms and larger than (1 +
 *        connPeripheralLatency) * connInterval * 2.
 *        Time = N * 10 ms.
 *        Values:
 *        - 0x000A (100 ms)  ... 0x0C80 (32000 ms)
 * @param Minimum_CE_Length Information parameter about the minimum length of
 *        connection needed for this LE connection.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x0000 (0.000 ms)  ... 0xFFFF (40959.375 ms)
 * @param Maximum_CE_Length Information parameter about the maximum length of
 *        connection needed for this LE connection.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x0000 (0.000 ms)  ... 0xFFFF (40959.375 ms)
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_create_connection( uint16_t LE_Scan_Interval,
                                      uint16_t LE_Scan_Window,
                                      uint8_t Peer_Address_Type,
                                      const uint8_t* Peer_Address,
                                      uint8_t Own_Address_Type,
                                      uint16_t Conn_Interval_Min,
                                      uint16_t Conn_Interval_Max,
                                      uint16_t Conn_Latency,
                                      uint16_t Supervision_Timeout,
                                      uint16_t Minimum_CE_Length,
                                      uint16_t Maximum_CE_Length );

/**
 * @brief ACI_GAP_TERMINATE_GAP_PROC
 * Terminates the specified GAP procedure. An ACI_GAP_PROC_COMPLETE_EVENT event
 * is returned with the procedure code set to the corresponding procedure.
 * Note: in case of GAP procedure started with ACI_GAP_CREATE_CONNECTION or
 * ACI_GAP_START_AUTO_CONNECTION_ESTABLISH_PROC, a
 * HCI_LE_CREATE_CONNECTION_CANCEL is issued to the Controller.
 *
 * @param Procedure_Code GAP procedure bitmap.
 *        Values:
 *        - 0x00: No events
 *        - 0x01: GAP_LIMITED_DISCOVERY_PROC
 *        - 0x02: GAP_GENERAL_DISCOVERY_PROC
 *        - 0x08: GAP_AUTO_CONNECTION_ESTABLISHMENT_PROC
 *        - 0x10: GAP_GENERAL_CONNECTION_ESTABLISHMENT_PROC
 *        - 0x20: GAP_SELECTIVE_CONNECTION_ESTABLISHMENT_PROC
 *        - 0x40: GAP_DIRECT_CONNECTION_ESTABLISHMENT_PROC
 *        - 0x80: GAP_OBSERVATION_PROC
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_terminate_gap_proc( uint8_t Procedure_Code );

/**
 * @brief ACI_GAP_START_CONNECTION_UPDATE
 * Starts the connection update procedure (only when role is Central). A
 * HCI_LE_CONNECTION_UPDATE is called.
 * On completion of the procedure, an HCI_LE_CONNECTION_UPDATE_COMPLETE_EVENT
 * event is returned to the upper layer.
 *
 * @param Connection_Handle Connection handle for which the command applies.
 *        Values:
 *        - 0x0000 ... 0x0EFF
 * @param Conn_Interval_Min Minimum value for the connection event interval.
 *        Time = N * 1.25 ms.
 *        Values:
 *        - 0x0006 (7.50 ms)  ... 0x0C80 (4000.00 ms)
 * @param Conn_Interval_Max Maximum value for the connection event interval.
 *        Time = N * 1.25 ms.
 *        Values:
 *        - 0x0006 (7.50 ms)  ... 0x0C80 (4000.00 ms)
 * @param Conn_Latency Maximum Peripheral latency for the connection in number
 *        of connection events.
 *        Values:
 *        - 0x0000 ... 0x01F3
 * @param Supervision_Timeout Supervision timeout for the LE Link.
 *        It shall be a multiple of 10 ms and larger than (1 +
 *        connPeripheralLatency) * connInterval * 2.
 *        Time = N * 10 ms.
 *        Values:
 *        - 0x000A (100 ms)  ... 0x0C80 (32000 ms)
 * @param Minimum_CE_Length Information parameter about the minimum length of
 *        connection needed for this LE connection.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x0000 (0.000 ms)  ... 0xFFFF (40959.375 ms)
 * @param Maximum_CE_Length Information parameter about the maximum length of
 *        connection needed for this LE connection.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x0000 (0.000 ms)  ... 0xFFFF (40959.375 ms)
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_start_connection_update( uint16_t Connection_Handle,
                                            uint16_t Conn_Interval_Min,
                                            uint16_t Conn_Interval_Max,
                                            uint16_t Conn_Latency,
                                            uint16_t Supervision_Timeout,
                                            uint16_t Minimum_CE_Length,
                                            uint16_t Maximum_CE_Length );

/**
 * @brief ACI_GAP_SEND_PAIRING_REQ
 * Sends the SM pairing request to start a pairing process. The authentication
 * requirements and IO capabilities should be set before issuing this command
 * using the ACI_GAP_SET_IO_CAPABILITY and
 * ACI_GAP_SET_AUTHENTICATION_REQUIREMENT commands.
 * A ACI_GAP_PAIRING_COMPLETE_EVENT event is returned after the pairing process
 * is completed.
 *
 * @param Connection_Handle Connection handle for which the command applies.
 *        Values:
 *        - 0x0000 ... 0x0EFF
 * @param Force_Rebond If 1, Pairing request will be sent even if the device
 *        was previously bonded, otherwise pairing request is not sent.
 *        Values:
 *        - 0x00: NO
 *        - 0x01: YES
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_send_pairing_req( uint16_t Connection_Handle,
                                     uint8_t Force_Rebond );

/**
 * @brief ACI_GAP_SET_BROADCAST_MODE
 * This command puts the device into broadcast mode. A privacy enabled device
 * uses either a resolvable private address or a non-resolvable private address
 * as specified in the Own_Address_Type parameter of the command.
 * Note: this command only supports legacy advertising. For extended
 * advertising, refer to ACI_GAP_ADV_SET_CONFIGURATION and
 * ACI_GAP_ADV_SET_ENABLE.
 *
 * @param Advertising_Interval_Min Minimum advertising interval.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x0020 (20.000 ms)  ... 0x4000 (10240.000 ms)
 * @param Advertising_Interval_Max Maximum advertising interval.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x0020 (20.000 ms)  ... 0x4000 (10240.000 ms)
 * @param Advertising_Type Advertising type
 *        Values:
 *        - 0x02: ADV_SCAN_IND (Scannable undirected advertising)
 *        - 0x03: ADV_NONCONN_IND (Non connectable undirected advertising)
 * @param Own_Address_Type Own address type: if Privacy is disabled, the
 *        address can be public or static random; otherwise, it can be a
 *        resolvable private address or a non-resolvable private address.
 *        Values:
 *        - 0x00: Public address
 *        - 0x01: Static random address
 *        - 0x02: Resolvable private address
 *        - 0x03: Non-resolvable private address
 * @param Adv_Data_Length Length of the advertising data in the advertising
 *        packet.
 * @param Adv_Data Advertising data used by the device while advertising.
 * @param Num_of_Peer_Entries Number of devices that have to be added to the
 *        Filter Accept List. Each device is defined by Peer_Address_Type and
 *        Peer_Address.
 * @param Peer_Entry See @ref Peer_Entry_t
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_set_broadcast_mode( uint16_t Advertising_Interval_Min,
                                       uint16_t Advertising_Interval_Max,
                                       uint8_t Advertising_Type,
                                       uint8_t Own_Address_Type,
                                       uint8_t Adv_Data_Length,
                                       const uint8_t* Adv_Data,
                                       uint8_t Num_of_Peer_Entries,
                                       const Peer_Entry_t* Peer_Entry );

/**
 * @brief ACI_GAP_START_OBSERVATION_PROC
 * Starts an Observation procedure when the device is in Observer Role. The
 * host enables scanning in the controller. The advertising reports are sent to
 * the upper layer using standard LE Advertising Report Event.
 * If privacy is enabled and the peer device (advertiser) is in the resolving
 * list then the link layer will generate a RPA, if it is not then the RPA/NRPA
 * generated by the Host will be used.
 *
 * @param LE_Scan_Interval This is defined as the time interval from when the
 *        Controller started its last LE scan until it begins the subsequent LE
 *        scan.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x0004 (2.500 ms)  ... 0x4000 (10240.000 ms) : legacy advertising
 *        - 0x0004 (2.500 ms)  ... 0x5DC0 (15000.000 ms) : extended advertising
 * @param LE_Scan_Window Amount of time for the duration of the LE scan.
 *        LE_Scan_Window shall be less than or equal to LE_Scan_Interval.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x0004 (2.500 ms)  ... 0x4000 (10240.000 ms) : legacy advertising
 *        - 0x0004 (2.500 ms)  ... 0x5DC0 (15000.000 ms) : extended advertising
 * @param LE_Scan_Type Passive or active scanning. With passive scanning, no
 *        scan request PDUs are sent.
 *        Values:
 *        - 0x00: Passive scanning
 *        - 0x01: Active scanning
 * @param Own_Address_Type Own address type: if Privacy is disabled, the
 *        address can be public or static random; otherwise, it can be a
 *        resolvable private address or a non-resolvable private address.
 *        Values:
 *        - 0x00: Public address
 *        - 0x01: Static random address
 *        - 0x02: Resolvable private address
 *        - 0x03: Non-resolvable private address
 * @param Filter_Duplicates Enable/disable duplicate filtering.
 *        Values:
 *        - 0x00: Duplicate filtering disabled
 *        - 0x01: Duplicate filtering enabled
 * @param Scanning_Filter_Policy The scanning filter policy determines how the
 *        scanner's Link Layer processes advertising and scan response PDUs.
 *        There is a choice of two primary filter policies: unfiltered and
 *        filtered.
 *        Unfiltered: the Link Layer processes all advertising and scan
 *        response PDUs (i.e., the Filter Accept List is not used).
 *        Filtered: the Link Layer processes advertising and scan response PDUs
 *        only from devices in the Filter Accept List.
 *        With extended scanning filter policies, a directed advertising PDU
 *        accepted by the primary filter policy shall nevertheless be ignored
 *        unless either the TargetA field is identical to the scanner's device
 *        address, or TargetA field is a resolvable private address.
 *        Values:
 *        - 0x00: Basic unfiltered scanning filter policy
 *        - 0x01: Basic filtered scanning filter policy
 *        - 0x02: Extended unfiltered scanning filter policy
 *        - 0x03: Extended filtered scanning filter policy
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_start_observation_proc( uint16_t LE_Scan_Interval,
                                           uint16_t LE_Scan_Window,
                                           uint8_t LE_Scan_Type,
                                           uint8_t Own_Address_Type,
                                           uint8_t Filter_Duplicates,
                                           uint8_t Scanning_Filter_Policy );

/**
 * @brief ACI_GAP_GET_BONDED_DEVICES
 * This command gets the list of the devices which are present in the bonding
 * table. It returns the number of addresses and the corresponding address
 * types and values.
 *
 * @param[out] Num_of_Addresses The number of bonded devices
 * @param[out] Bonded_Device_Entry See @ref Bonded_Device_Entry_t
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_get_bonded_devices( uint8_t* Num_of_Addresses,
                                       Bonded_Device_Entry_t* Bonded_Device_Entry );

/**
 * @brief ACI_GAP_CHECK_BONDED_DEVICE
 * The command finds whether the device, whose address is specified in the
 * command, is present in the bonding table. If the device is found, the
 * command returns "Success".
 * Note: the specified address can be a Resolvable Private Address (RPA). In
 * this case, even if privacy is not enabled, this address is resolved to check
 * the presence of the peer device in the bonding table. If the address is
 * resolved successfully with any one of the IRKs present in the database, the
 * command returns success and also the corresponding public or static random
 * address stored with the IRK in the database (i.e. the peer device identity
 * address distributed during bonding).
 *
 * @param Peer_Address_Type The address type of the peer device.
 *        Values:
 *        - 0x00: Public Device Address
 *        - 0x01: Random Device Address
 * @param Peer_Address Public Device Address or Random Device Address of the
 *        peer device
 * @param[out] Id_Address_Type The address type of the peer device, distributed
 *        during pairing phase.
 * @param[out] Id_Address The public or static random address of the peer
 *        device, distributed during pairing phase.
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_check_bonded_device( uint8_t Peer_Address_Type,
                                        const uint8_t* Peer_Address,
                                        uint8_t* Id_Address_Type,
                                        uint8_t* Id_Address );

/**
 * @brief ACI_GAP_NUMERIC_COMPARISON_VALUE_CONFIRM_YESNO
 * This command allows the User to validate/confirm or not the Numeric
 * Comparison value showed through the ACI_GAP_NUMERIC_COMPARISON_VALUE_EVENT.
 *
 * @param Connection_Handle Connection handle for which the command applies.
 *        Values:
 *        - 0x0000 ... 0x0EFF
 * @param Confirm_Yes_No Indicates if the numeric values showed on both local
 *        and peer device are different or equal
 *        Values:
 *        - 0x00: No (numeric values are different)
 *        - 0x01: Yes (numeric values are equal)
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_numeric_comparison_value_confirm_yesno( uint16_t Connection_Handle,
                                                           uint8_t Confirm_Yes_No );

/**
 * @brief ACI_GAP_PASSKEY_INPUT
 * This command permits to signal to the Stack the input type detected during
 * Passkey input.
 *
 * @param Connection_Handle Connection handle for which the command applies.
 *        Values:
 *        - 0x0000 ... 0x0EFF
 * @param Input_Type Passkey input type detected
 *        Values:
 *        - 0x00: Passkey entry started
 *        - 0x01: Passkey digit entered
 *        - 0x02: Passkey digit erased
 *        - 0x03: Passkey cleared
 *        - 0x04: Passkey entry completed
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_passkey_input( uint16_t Connection_Handle,
                                  uint8_t Input_Type );

/**
 * @brief ACI_GAP_GET_OOB_DATA
 * This command is sent by the User to get (i.e. to extract from the Stack) the
 * OOB data generated by the Stack itself.
 *
 * @param OOB_Data_Type Type of OOB data
 *        Values:
 *        - 0x00: TK (Legacy pairing)
 *        - 0x01: Random (SC)
 *        - 0x02: Confirm (SC)
 * @param[out] Address_Type Identity address type
 *        Values:
 *        - 0x00: Public Identity Address
 *        - 0x01: Random (static) Identity Address
 * @param[out] Address Public or Random (static) address of this device
 * @param[out] OOB_Data_Len Length of OOB data
 *        Values:
 *        - 16
 * @param[out] OOB_Data Local OOB data
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_get_oob_data( uint8_t OOB_Data_Type,
                                 uint8_t* Address_Type,
                                 uint8_t* Address,
                                 uint8_t* OOB_Data_Len,
                                 uint8_t* OOB_Data );

/**
 * @brief ACI_GAP_SET_OOB_DATA
 * This command is sent (by the User) to input the OOB data arrived via OOB
 * communication.
 *
 * @param Device_Type OOB Device type
 *        Values:
 *        - 0x00: Local device (Address_Type and Address are not used)
 *        - 0x01: Remote device
 * @param Address_Type Identity address type
 *        Values:
 *        - 0x00: Public Identity Address
 *        - 0x01: Random (static) Identity Address
 * @param Address Public or Random (static) Identity Address of the peer device
 * @param OOB_Data_Type Type of OOB data
 *        Values:
 *        - 0x00: TK (Legacy pairing)
 *        - 0x01: Random (SC)
 *        - 0x02: Confirm (SC)
 * @param OOB_Data_Len Length of OOB data
 *        Values:
 *        - 0: SC Random/Confirm generation (OOB_Data and OOB_Data_Type are not
 *          used)
 *        - 16
 * @param OOB_Data Either local OOB data or remote OOB data received through
 *        OOB from peer device (see Device_Type)
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_set_oob_data( uint8_t Device_Type,
                                 uint8_t Address_Type,
                                 const uint8_t* Address,
                                 uint8_t OOB_Data_Type,
                                 uint8_t OOB_Data_Len,
                                 const uint8_t* OOB_Data );

/**
 * @brief ACI_GAP_REMOVE_BONDED_DEVICE
 * This command removes a specified device from bonding table; i.e. it removes
 * from bonding table all security and GATT information related to the
 * specified device.
 *
 * @param Peer_Identity_Address_Type Identity address type
 *        Values:
 *        - 0x00: Public Identity Address
 *        - 0x01: Random (static) Identity Address
 * @param Peer_Identity_Address Public or Random (static) Identity Address of
 *        the peer device
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_remove_bonded_device( uint8_t Peer_Identity_Address_Type,
                                         const uint8_t* Peer_Identity_Address );

/**
 * @brief ACI_GAP_ADD_DEVICES_TO_LIST
 * This command is used to add specific device addresses to the Filter Accept
 * List and/or resolving list.
 *
 * @param Num_of_List_Entries Number of devices that have to be added to the
 *        list.
 * @param List_Entry See @ref List_Entry_t
 * @param Mode Mode used for adding devices in the lists.
 *        Values:
 *        - 0x00: Append to the resolving list only
 *        - 0x01: Clear and set the resolving list only
 *        - 0x02: Append to the Filter Accept List only
 *        - 0x03: Clear and set the Filter Accept List only
 *        - 0x04: Append to both resolving list and Filter Accept List
 *        - 0x05: Clear and set both resolving list and Filter Accept List
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_add_devices_to_list( uint8_t Num_of_List_Entries,
                                        const List_Entry_t* List_Entry,
                                        uint8_t Mode );

/**
 * @brief ACI_GAP_PAIRING_REQUEST_REPLY
 * This command is used to reply to ACI_GAP_PAIRING_REQUEST_EVENT. It enables
 * to allow or reject either the Pairing Request from the Central or the
 * Security Request from the Peripheral.
 *
 * @param Connection_Handle Connection handle for which the command applies.
 *        Values:
 *        - 0x0000 ... 0x0EFF
 * @param Accept Enables to accept or reject the pairing request.
 *        Values:
 *        - 0x00: Reject
 *        - 0x01: Accept
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_pairing_request_reply( uint16_t Connection_Handle,
                                          uint8_t Accept );

/**
 * @brief ACI_GAP_ADDITIONAL_BEACON_START
 * This command starts an advertising beacon. It allows additional advertising
 * packets to be transmitted independently of the packets transmitted with GAP
 * advertising commands such as ACI_GAP_SET_DISCOVERABLE or
 * ACI_GAP_SET_LIMITED_DISCOVERABLE.
 *
 * @param Adv_Interval_Min Minimum advertising interval.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x0020 (20.000 ms)  ... 0x4000 (10240.000 ms)
 * @param Adv_Interval_Max Maximum advertising interval.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x0020 (20.000 ms)  ... 0x4000 (10240.000 ms)
 * @param Adv_Channel_Map Advertising channel map.
 *        Flags:
 *        - 0x01: Channel 37 shall be used
 *        - 0x02: Channel 38 shall be used
 *        - 0x04: Channel 39 shall be used
 * @param Own_Address_Type Own address type: public or static random.
 *        Values:
 *        - 0x00: Public Device Address
 *        - 0x01: Random Device Address
 * @param Own_Address Public Device Address or Random Device Address.
 * @param PA_Level Power amplifier output level.
 *        Values:
 *        - 0x00 ... 0x23
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_additional_beacon_start( uint16_t Adv_Interval_Min,
                                            uint16_t Adv_Interval_Max,
                                            uint8_t Adv_Channel_Map,
                                            uint8_t Own_Address_Type,
                                            const uint8_t* Own_Address,
                                            uint8_t PA_Level );

/**
 * @brief ACI_GAP_ADDITIONAL_BEACON_STOP
 * This command stops the advertising beacon started with
 * ACI_GAP_ADDITIONAL_BEACON_START.
 *
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_additional_beacon_stop( void );

/**
 * @brief ACI_GAP_ADDITIONAL_BEACON_SET_DATA
 * This command sets the data transmitted by the advertising beacon started
 * with ACI_GAP_ADDITIONAL_BEACON_START. If the advertising beacon is already
 * started, the new data is used in subsequent beacon advertising events.
 *
 * @param Adv_Data_Length Length of Adv_Data in octets
 * @param Adv_Data Advertising data used by the device while advertising.
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_additional_beacon_set_data( uint8_t Adv_Data_Length,
                                               const uint8_t* Adv_Data );

/**
 * @brief ACI_GAP_ADV_SET_CONFIGURATION
 * This command is used to set the extended advertising configuration for one
 * advertising set.
 * This command, in association with ACI_GAP_ADV_SET_SCAN_RESP_DATA,
 * ACI_GAP_ADV_SET_ADV_DATA and ACI_GAP_ADV_SET_ENABLE, enables to start
 * extended advertising. These commands must be used in replacement of
 * ACI_GAP_SET_DISCOVERABLE, ACI_GAP_SET_LIMITED_DISCOVERABLE,
 * ACI_GAP_SET_DIRECT_CONNECTABLE, ACI_GAP_SET_NON_CONNECTABLE,
 * ACI_GAP_SET_UNDIRECTED_CONNECTABLE and ACI_GAP_SET_BROADCAST_MODE that only
 * support legacy advertising.
 * If bit 0 of Adv_Mode is set, the Own_Address_Type parameter is ignored and
 * the own address shall be set with the ACI_GAP_ADV_SET_RANDOM_ADDRESS
 * command. This mode is only valid for non-connectable advertising.
 *
 * @param Adv_Mode Bitmap of extended advertising modes
 *        Flags:
 *        - 0x01: Use specific random address
 * @param Advertising_Handle Used to identify an advertising set.
 *        Values:
 *        - 0x00 ... 0xEF
 * @param Adv_Event_Properties Type of advertising event.
 *        Flags:
 *        - 0x0001: Connectable advertising
 *        - 0x0002: Scannable advertising
 *        - 0x0004: Directed advertising
 *        - 0x0008: High Duty Cycle Directed Connectable advertising
 *        - 0x0010: Use legacy advertising PDUs
 *        - 0x0020: Anonymous advertising
 *        - 0x0040: Include TxPower in at least one advertising PDU
 * @param Primary_Adv_Interval_Min Minimum advertising interval.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x00000020 (20.000 ms)  ... 0x00FFFFFF (10485759.375 ms)
 * @param Primary_Adv_Interval_Max Maximum advertising interval.
 *        Time = N * 0.625 ms.
 *        Values:
 *        - 0x00000020 (20.000 ms)  ... 0x00FFFFFF (10485759.375 ms)
 * @param Primary_Adv_Channel_Map Advertising channel map.
 *        Flags:
 *        - 0x01: Channel 37 shall be used
 *        - 0x02: Channel 38 shall be used
 *        - 0x04: Channel 39 shall be used
 * @param Own_Address_Type Own address type: if Privacy is disabled, the
 *        address can be public or static random; otherwise, it can be a
 *        resolvable private address or a non-resolvable private address.
 *        Values:
 *        - 0x00: Public address
 *        - 0x01: Static random address
 *        - 0x02: Resolvable private address
 *        - 0x03: Non-resolvable private address
 * @param Peer_Address_Type Address type of the peer device.
 *        Values:
 *        - 0x00: Public Device Address or Public Identity Address
 *        - 0x01: Random Device Address or Random (static) Identity Address
 * @param Peer_Address Public Device Address, Random Device Address, Public
 *        Identity Address, or Random (static) Identity Address of the device
 *        to be connected.
 * @param Adv_Filter_Policy Advertising filter policy
 *        Values:
 *        - 0x00: Process scan and connection requests from all devices (i.e.,
 *          the Filter Accept List is not in use)
 *        - 0x01: Process connection requests from all devices and scan
 *          requests only from devices that are in the Filter Accept List.
 *        - 0x02: Process scan requests from all devices and connection
 *          requests only from devices that are in the Filter Accept List.
 *        - 0x03: Process scan and connection requests only from devices in the
 *          Filter Accept List.
 * @param Adv_TX_Power Advertising TX power. Units: dBm.
 *        Values:
 *        - 127: Host has no preference
 *        - -127 ... 20
 * @param Secondary_Adv_Max_Skip Secondary advertising maximum skip.
 *        Values:
 *        - 0x00: AUX_ADV_IND shall be sent prior to the next advertising event
 *        - 0x01 ... 0xFF: Maximum advertising events the Controller can skip
 *          before sending the AUX_ADV_IND packets on the secondary advertising
 *          physical channel
 * @param Secondary_Adv_PHY Secondary advertising PHY.
 *        Values:
 *        - 0x01: Secondary advertisement PHY is LE 1M
 *        - 0x02: Secondary advertisement PHY is LE 2M
 * @param Adv_SID Value of the Advertising SID subfield in the ADI field of the
 *        PDU.
 *        Values:
 *        - 0x00 ... 0x0F
 * @param Scan_Req_Notification_Enable Scan request notifications.
 *        Values:
 *        - 0x00: Scan request notifications disabled
 *        - 0x01: Scan request notifications enabled
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_adv_set_configuration( uint8_t Adv_Mode,
                                          uint8_t Advertising_Handle,
                                          uint16_t Adv_Event_Properties,
                                          uint32_t Primary_Adv_Interval_Min,
                                          uint32_t Primary_Adv_Interval_Max,
                                          uint8_t Primary_Adv_Channel_Map,
                                          uint8_t Own_Address_Type,
                                          uint8_t Peer_Address_Type,
                                          const uint8_t* Peer_Address,
                                          uint8_t Adv_Filter_Policy,
                                          uint8_t Adv_TX_Power,
                                          uint8_t Secondary_Adv_Max_Skip,
                                          uint8_t Secondary_Adv_PHY,
                                          uint8_t Adv_SID,
                                          uint8_t Scan_Req_Notification_Enable );

/**
 * @brief ACI_GAP_ADV_SET_ENABLE
 * This command is used to request the Controller to enable or disable one or
 * more extended advertising sets.
 *
 * @param Enable Enable/disable advertising.
 *        Values:
 *        - 0x00: Advertising is disabled
 *        - 0x01: Advertising is enabled
 * @param Num_Sets Number of advertising sets.
 *        Values:
 *        - 0x00: Disable all advertising sets
 *        - 0x01 ... 0x3F: Number of advertising sets to enable or disable
 * @param Adv_Set See @ref Adv_Set_t
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_adv_set_enable( uint8_t Enable,
                                   uint8_t Num_Sets,
                                   const Adv_Set_t* Adv_Set );

/**
 * @brief ACI_GAP_ADV_SET_ADV_DATA
 * This command is used to set the data used in extended advertising PDUs that
 * have a data field.
 *
 * @param Advertising_Handle Used to identify an advertising set.
 *        Values:
 *        - 0x00 ... 0xEF
 * @param Operation Advertising operation.
 *        Values:
 *        - 0x00: Intermediate fragment of fragmented extended advertising data
 *        - 0x01: First fragment of fragmented extended advertising data
 *        - 0x02: Last fragment of fragmented extended advertising data
 *        - 0x03: Complete extended advertising data
 *        - 0x04: Unchanged data (just update the Advertising DID)
 * @param Fragment_Preference Fragment preference.
 *        Values:
 *        - 0x00: The Controller may fragment all data
 *        - 0x01: The Controller should not fragment or should minimize
 *          fragmentation of data
 * @param Advertising_Data_Length Length of Advertising_Data in octets
 * @param Advertising_Data Data formatted as defined in Core Specification [Vol
 *        3, Part C, 11].
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_adv_set_adv_data( uint8_t Advertising_Handle,
                                     uint8_t Operation,
                                     uint8_t Fragment_Preference,
                                     uint8_t Advertising_Data_Length,
                                     const uint8_t* Advertising_Data );

/**
 * @brief ACI_GAP_ADV_SET_SCAN_RESP_DATA
 * This command is used to provide scan response data used during extended
 * advertising.
 *
 * @param Advertising_Handle Used to identify an advertising set.
 *        Values:
 *        - 0x00 ... 0xEF
 * @param Operation Scan response operation.
 *        Values:
 *        - 0x00: Intermediate fragment of fragmented scan response data
 *        - 0x01: First fragment of fragmented scan response data
 *        - 0x02: Last fragment of fragmented scan response data
 *        - 0x03: Complete scan response data
 * @param Fragment_Preference Fragment preference.
 *        Values:
 *        - 0x00: The Controller may fragment all data
 *        - 0x01: The Controller should not fragment or should minimize
 *          fragmentation of data
 * @param Scan_Response_Data_Length Length of Scan_Response_Data in octets
 * @param Scan_Response_Data Data formatted as defined in Core Specification
 *        [Vol 3, Part C, 11].
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_adv_set_scan_resp_data( uint8_t Advertising_Handle,
                                           uint8_t Operation,
                                           uint8_t Fragment_Preference,
                                           uint8_t Scan_Response_Data_Length,
                                           const uint8_t* Scan_Response_Data );

/**
 * @brief ACI_GAP_ADV_REMOVE_SET
 * This command is used to remove an advertising set from the Controller.
 *
 * @param Advertising_Handle Used to identify an advertising set.
 *        Values:
 *        - 0x00 ... 0xEF
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_adv_remove_set( uint8_t Advertising_Handle );

/**
 * @brief ACI_GAP_ADV_CLEAR_SETS
 * This command is used to remove all existing advertising sets from the
 * Controller.
 *
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_adv_clear_sets( void );

/**
 * @brief ACI_GAP_ADV_SET_RANDOM_ADDRESS
 * This command is used to set the random device address of an advertising set
 * configured to use specific random address.
 *
 * @param Advertising_Handle Used to identify an advertising set.
 *        Values:
 *        - 0x00 ... 0xEF
 * @param Random_Address Random Device Address.
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_adv_set_random_address( uint8_t Advertising_Handle,
                                           const uint8_t* Random_Address );

/**
 * @brief ACI_GAP_EXT_START_SCAN
 * This command is used to start a scanning procedure when the extended
 * advertising feature is supported.
 * The Scanning_PHYs and subsequent parameters are used to specify the scanning
 * parameters as defined for HCI_LE_SET_EXTENDED_SCAN_PARAMETERS: for more
 * details, refer to Core Specification [Vol 4, Part E, 7.8.64].
 * Note: this more generic command can be used instead of
 * ACI_GAP_START_LIMITED_DISCOVERY_PROC, ACI_GAP_START_GENERAL_DISCOVERY_PROC,
 * ACI_GAP_START_GENERAL_CONNECTION_ESTABLISH_PROC,
 * ACI_GAP_START_SELECTIVE_CONNECTION_ESTABLISH_PROC or
 * ACI_GAP_START_OBSERVATION_PROC.
 *
 * @param Scan_Mode Reserved, shall be set to 0.
 *        Values:
 *        - 0x00
 * @param Procedure Scan procedure.
 *        Values:
 *        - 0x01: GAP_LIMITED_DISCOVERY_PROC
 *        - 0x02: GAP_GENERAL_DISCOVERY_PROC
 *        - 0x10: GAP_GENERAL_CONNECTION_ESTABLISHMENT_PROC
 *        - 0x20: GAP_SELECTIVE_CONNECTION_ESTABLISHMENT_PROC
 *        - 0x80: GAP_OBSERVATION_PROC
 * @param Own_Address_Type Own address type: if Privacy is disabled, the
 *        address can be public or static random; otherwise, it can be a
 *        resolvable private address or a non-resolvable private address.
 *        Values:
 *        - 0x00: Public address
 *        - 0x01: Static random address
 *        - 0x02: Resolvable private address
 *        - 0x03: Non-resolvable private address
 * @param Filter_Duplicates Duplicate filtering.
 *        Values:
 *        - 0x00: Duplicate filtering disabled
 *        - 0x01: Duplicate filtering enabled
 *        - 0x02: Duplicate filtering enabled, reset for each scan period
 * @param Duration Scan duration.
 *        Time = N * 10 ms.
 *        Values:
 *        - 0x0000 (0 ms) : Scan continuously until explicitly disable
 *        - 0x0001 (10 ms)  ... 0xFFFF (655350 ms) : Scan duration
 * @param Period Scan period.
 *        Time = N * 1.28 s.
 *        Values:
 *        - 0x0000 (0 ms) : Scan continuously
 *        - 0x0001 (1280 ms)  ... 0xFFFF (83884800 ms) : Time interval from
 *          when the Controller started its last Scan_Duration until it begins
 *          the subsequent Scan_Duration
 * @param Scanning_Filter_Policy The scanning filter policy determines how the
 *        scanner's Link Layer processes advertising and scan response PDUs.
 *        There is a choice of two primary filter policies: unfiltered and
 *        filtered.
 *        Unfiltered: the Link Layer processes all advertising and scan
 *        response PDUs (i.e., the Filter Accept List is not used).
 *        Filtered: the Link Layer processes advertising and scan response PDUs
 *        only from devices in the Filter Accept List.
 *        With extended scanning filter policies, a directed advertising PDU
 *        accepted by the primary filter policy shall nevertheless be ignored
 *        unless either the TargetA field is identical to the scanner's device
 *        address, or TargetA field is a resolvable private address.
 *        Values:
 *        - 0x00: Basic unfiltered scanning filter policy
 *        - 0x01: Basic filtered scanning filter policy
 *        - 0x02: Extended unfiltered scanning filter policy
 *        - 0x03: Extended filtered scanning filter policy
 * @param Scanning_PHYs Scan PHYs.
 *        Flags:
 *        - 0x01: Scan advertisements on the LE 1M PHY
 * @param Scan_Param_Phy See @ref Scan_Param_Phy_t
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_ext_start_scan( uint8_t Scan_Mode,
                                   uint8_t Procedure,
                                   uint8_t Own_Address_Type,
                                   uint8_t Filter_Duplicates,
                                   uint16_t Duration,
                                   uint16_t Period,
                                   uint8_t Scanning_Filter_Policy,
                                   uint8_t Scanning_PHYs,
                                   const Scan_Param_Phy_t* Scan_Param_Phy );

/**
 * @brief ACI_GAP_EXT_CREATE_CONNECTION
 * This command is used to create a connection with the local device in the
 * Central role to an advertiser when the extended advertising feature is
 * supported.
 * The Advertising_Handle and Subevent parameters are ignored.
 * The Initiating_PHYs and subsequent parameters are used to specify the
 * initiating parameters as defined for HCI_LE_EXTENDED_CREATE_CONNECTION: for
 * more details, refer to Core Specification [Vol 4, Part E, 7.8.66].
 * Note: this more generic command can be used instead of
 * ACI_GAP_CREATE_CONNECTION or ACI_GAP_START_AUTO_CONNECTION_ESTABLISH_PROC.
 *
 * @param Initiating_Mode Reserved, shall be set to 0.
 *        Values:
 *        - 0x00
 * @param Procedure Connection procedure.
 *        Values:
 *        - 0x08: GAP_AUTO_CONNECTION_ESTABLISHMENT_PROC
 *        - 0x40: GAP_DIRECT_CONNECTION_ESTABLISHMENT_PROC
 * @param Own_Address_Type Own address type: if Privacy is disabled, the
 *        address can be public or static random; otherwise, it can be a
 *        resolvable private address.
 *        Values:
 *        - 0x00: Public address
 *        - 0x01: Static random address
 *        - 0x02: Resolvable private address
 * @param Peer_Address_Type The address type of the peer device.
 *        Values:
 *        - 0x00: Public Device Address
 *        - 0x01: Random Device Address
 * @param Peer_Address Public Device Address or Random Device Address of the
 *        device to be connected.
 * @param Advertising_Handle Used to identify the subevent where a connection
 *        request shall be initiated from a periodic advertising train.
 *        Values:
 *        - 0xFF: Parameter not used
 *        - 0x00 ... 0xEF
 * @param Subevent Subevent where the connection request is to be sent.
 *        Values:
 *        - 0xFF: Parameter not used
 *        - 0x00 ... 0x7F
 * @param Initiator_Filter_Policy Initiator filter policy.
 *        Values:
 *        - 0x00: Filter Accept List is not used to determine which advertiser
 *          to connect to
 *        - 0x01: Filter Accept List is used to determine which advertiser to
 *          connect to (Peer_Address_Type and Peer_Address are ignored)
 * @param Initiating_PHYs Initiating PHYs.
 *        Flags:
 *        - 0x01: Scan connectable advertisements on the LE 1M PHY- Connection
 *          parameters for the LE 1M PHY
 *        - 0x02: Connection parameters for the LE 2M PHY
 * @param Init_Param_Phy See @ref Init_Param_Phy_t
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_ext_create_connection( uint8_t Initiating_Mode,
                                          uint8_t Procedure,
                                          uint8_t Own_Address_Type,
                                          uint8_t Peer_Address_Type,
                                          const uint8_t* Peer_Address,
                                          uint8_t Advertising_Handle,
                                          uint8_t Subevent,
                                          uint8_t Initiator_Filter_Policy,
                                          uint8_t Initiating_PHYs,
                                          const Init_Param_Phy_t* Init_Param_Phy );


#endif /* BLE_GAP_ACI_H__ */
