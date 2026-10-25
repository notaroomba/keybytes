/*****************************************************************************
 * @file    ble_types.h
 * @brief   STM32WB BLE command/event types
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

#ifndef BLE_TYPES_H__
#define BLE_TYPES_H__


#include <stdint.h>
#include "ble_const.h"

/* Type used for function return value */
typedef uint8_t tBleStatus;

/* Definition of Host_Nb_Of_Completed_Pkt_Pair_t */
typedef __PACKED_STRUCT
{
  /**
   * Connection_Handle[i].
   * Values:
   * - 0x0000 ... 0x0EFF
   */
  uint16_t Connection_Handle;
  /**
   * The number of HCI Data Packets [i] that have been completed for the
   * associated Connection_Handle since the previous time the event was
   * returned.
   * Values:
   * - 0x0000 ... 0xFFFF
   */
  uint16_t Host_Num_Of_Completed_Packets;
} Host_Nb_Of_Completed_Pkt_Pair_t;

/* Definition of Adv_Set_t */
typedef __PACKED_STRUCT
{
  /**
   * Used to identify an advertising set.
   * Values:
   * - 0x00 ... 0xEF
   */
  uint8_t Advertising_Handle;
  /**
   * Duration of advertising set.
   * Time = N * 10 ms.
   * Values:
   * - 0x0000 (0 ms) : No advertising duration.
   * - 0x0001 (10 ms)  ... 0xFFFF (655350 ms) : Advertising duration
   */
  uint16_t Duration;
  /**
   * Maximum number of advertising events.
   * Values:
   * - 0x00: No maximum number of advertising events
   * - 0x01 ... 0xFF: Maximum number of extended advertising events the
   *   Controller shall attempt to send prior to terminating the extended
   *   advertising
   */
  uint8_t Max_Extended_Advertising_Events;
} Adv_Set_t;

/* Definition of Scan_Param_Phy_t */
typedef __PACKED_STRUCT
{
  /**
   * Passive or active scanning. With passive scanning, no scan request PDUs
   * are sent.
   * Values:
   * - 0x00: Passive scanning
   * - 0x01: Active scanning
   */
  uint8_t Scan_Type;
  /**
   * Time interval from when the Controller started its last scan until it
   * begins the subsequent scan on the primary advertising physical channel.
   * Time = N * 0.625 ms.
   * Values:
   * - 0x0004 (2.500 ms)  ... 0x5DC0 (15000.000 ms) : only supported range
   */
  uint16_t Scan_Interval;
  /**
   * Duration of the scan on the primary advertising physical channel.
   * Time = N * 0.625 ms.
   * Values:
   * - 0x0004 (2.500 ms)  ... 0x5DC0 (15000.000 ms) : only supported range
   */
  uint16_t Scan_Window;
} Scan_Param_Phy_t;

/* Definition of Init_Param_Phy_t */
typedef __PACKED_STRUCT
{
  /**
   * Time interval from when the Controller started its last scan until it
   * begins the subsequent scan on the primary advertising physical channel.
   * Time = N * 0.625 ms.
   * Values:
   * - 0x0004 (2.500 ms)  ... 0x5DC0 (15000.000 ms) : only supported range
   */
  uint16_t Scan_Interval;
  /**
   * Duration of the scan on the primary advertising physical channel.
   * Time = N * 0.625 ms.
   * Values:
   * - 0x0004 (2.500 ms)  ... 0x5DC0 (15000.000 ms) : only supported range
   */
  uint16_t Scan_Window;
  /**
   * Minimum value for the connection event interval.
   * Time = N * 1.25 ms.
   * Values:
   * - 0x0006 (7.50 ms)  ... 0x0C80 (4000.00 ms)
   */
  uint16_t Conn_Interval_Min;
  /**
   * Maximum value for the connection event interval.
   * Time = N * 1.25 ms.
   * Values:
   * - 0x0006 (7.50 ms)  ... 0x0C80 (4000.00 ms)
   */
  uint16_t Conn_Interval_Max;
  /**
   * Maximum Peripheral latency for the connection in number of connection
   * events.
   * Values:
   * - 0x0000 ... 0x01F3
   */
  uint16_t Conn_Latency;
  /**
   * Supervision timeout for the LE Link.
   * It shall be a multiple of 10 ms and larger than (1 +
   * connPeripheralLatency) * connInterval * 2.
   * Time = N * 10 ms.
   * Values:
   * - 0x000A (100 ms)  ... 0x0C80 (32000 ms)
   */
  uint16_t Supervision_Timeout;
  /**
   * Information parameter about the minimum length of connection needed for
   * this LE connection.
   * Time = N * 0.625 ms.
   * Values:
   * - 0x0000 (0.000 ms)  ... 0xFFFF (40959.375 ms)
   */
  uint16_t Min_CE_Length;
  /**
   * Information parameter about the maximum length of connection needed for
   * this LE connection.
   * Time = N * 0.625 ms.
   * Values:
   * - 0x0000 (0.000 ms)  ... 0xFFFF (40959.375 ms)
   */
  uint16_t Max_CE_Length;
} Init_Param_Phy_t;

/* Definition of Peer_Entry_t */
typedef __PACKED_STRUCT
{
  /**
   * Address type.
   * Values:
   * - 0x00: Public Device Address
   * - 0x01: Random Device Address
   */
  uint8_t Peer_Address_Type;
  /**
   * Public Device Address or Random Device Address.
   */
  uint8_t Peer_Address[6];
} Peer_Entry_t;

/* Definition of Bonded_Device_Entry_t */
typedef __PACKED_STRUCT
{
  /**
   * Address type.
   * Values:
   * - 0x00: Public Device Address
   * - 0x01: Random Device Address
   */
  uint8_t Address_Type;
  /**
   * Public Device Address or Random Device Address.
   */
  uint8_t Address[6];
} Bonded_Device_Entry_t;

/* Definition of List_Entry_t */
typedef __PACKED_STRUCT
{
  /**
   * Address type.
   * Values:
   * - 0x00: Public Device Address
   * - 0x01: Random Device Address
   */
  uint8_t Address_Type;
  /**
   * Public Device Address or Random Device Address.
   */
  uint8_t Address[6];
} List_Entry_t;

/* Definition of Service_UUID_t */
typedef __PACKED_UNION
{
  /**
   * 16-bit UUID
   */
  uint16_t Service_UUID_16;
  /**
   * 128-bit UUID
   */
  uint8_t Service_UUID_128[16];
} Service_UUID_t;

/* Definition of Include_UUID_t */
typedef __PACKED_UNION
{
  /**
   * 16-bit UUID
   */
  uint16_t Include_UUID_16;
  /**
   * 128-bit UUID
   */
  uint8_t Include_UUID_128[16];
} Include_UUID_t;

/* Definition of Char_UUID_t */
typedef __PACKED_UNION
{
  /**
   * 16-bit UUID
   */
  uint16_t Char_UUID_16;
  /**
   * 128-bit UUID
   */
  uint8_t Char_UUID_128[16];
} Char_UUID_t;

/* Definition of Char_Desc_Uuid_t */
typedef __PACKED_UNION
{
  /**
   * 16-bit UUID
   */
  uint16_t Char_UUID_16;
  /**
   * 128-bit UUID
   */
  uint8_t Char_UUID_128[16];
} Char_Desc_Uuid_t;

/* Definition of UUID_t */
typedef __PACKED_UNION
{
  /**
   * 16-bit UUID
   */
  uint16_t UUID_16;
  /**
   * 128-bit UUID
   */
  uint8_t UUID_128[16];
} UUID_t;

/* Definition of Handle_Entry_t */
typedef __PACKED_STRUCT
{
  /**
   * Attribute handle
   */
  uint16_t Handle;
} Handle_Entry_t;

/* Definition of Handle_Packets_Pair_Entry_t */
typedef __PACKED_STRUCT
{
  /**
   * Connection handle
   */
  uint16_t Connection_Handle;
  /**
   * The number of HCI Data Packets that have been completed (transmitted or
   * flushed) for the associated Connection_Handle since the previous time the
   * event was returned.
   */
  uint16_t HC_Num_Of_Completed_Packets;
} Handle_Packets_Pair_Entry_t;

/* Definition of Advertising_Report_t */
typedef __PACKED_STRUCT
{
  /**
   * Type of advertising report event:
   * ADV_IND: Connectable undirected advertising',
   * ADV_DIRECT_IND: Connectable directed advertising,
   * ADV_SCAN_IND: Scannable undirected advertising,
   * ADV_NONCONN_IND: Non connectable undirected advertising,
   * SCAN_RSP: Scan response.
   * Values:
   * - 0x00: ADV_IND
   * - 0x01: ADV_DIRECT_IND
   * - 0x02: ADV_SCAN_IND
   * - 0x03: ADV_NONCONN_IND
   * - 0x04: SCAN_RSP
   */
  uint8_t Event_Type;
  /**
   * Address type.
   * Values:
   * - 0x00: Public Device Address
   * - 0x01: Random Device Address
   * - 0x02: Public Identity Address (corresponds to the Resolved Private
   *   Address)
   * - 0x03: Random (static) Identity Address (corresponds to the Resolved
   *   Private Address)
   */
  uint8_t Address_Type;
  /**
   * Public Device Address or Random Device Address of the device to be
   * connected.
   */
  uint8_t Address[6];
  /**
   * Length of the Data field for each device which responded.
   * Values:
   * - 0 ... 31
   */
  uint8_t Length_Data;
  /**
   * Octets of advertising or scan response data formatted as defined in Core
   * Specification [Vol 3, Part C, 11].
   */
  const uint8_t* Data;
  /**
   * RSSI (signed integer).
   * Units: dBm.
   * Values:
   * - 127: RSSI not available
   * - -127 ... 20
   */
  uint8_t RSSI;
} Advertising_Report_t;

/* Definition of Direct_Advertising_Report_t */
typedef __PACKED_STRUCT
{
  /**
   * Advertising type
   * Values:
   * - 0x01: Connectable directed advertising (ADV_DIRECT_IND)
   */
  uint8_t Event_Type;
  /**
   * Address type.
   * Values:
   * - 0x00: Public Device Address
   * - 0x01: Random Device Address
   * - 0x02: Public Identity Address (corresponds to the Resolved Private
   *   Address)
   * - 0x03: Random (static) Identity Address (corresponds to the Resolved
   *   Private Address)
   */
  uint8_t Address_Type;
  /**
   * Public Device Address, Random Device Address, Public Identity Address or
   * Random (static) Identity Address of the advertising device.
   */
  uint8_t Address[6];
  /**
   * 0x01 Random Device Address
   * Values:
   * - 0x01: Random Device Address
   */
  uint8_t Direct_Address_Type;
  /**
   * Random Device Address
   */
  uint8_t Direct_Address[6];
  /**
   * RSSI (signed integer).
   * Units: dBm.
   * Values:
   * - 127: RSSI not available
   * - -127 ... 20
   */
  uint8_t RSSI;
} Direct_Advertising_Report_t;

/* Definition of Attribute_Group_Handle_Pair_t */
typedef __PACKED_STRUCT
{
  /**
   * Found Attribute handle
   */
  uint16_t Found_Attribute_Handle;
  /**
   * Group End handle
   */
  uint16_t Group_End_Handle;
} Attribute_Group_Handle_Pair_t;

/* Definition of Handle_Item_t */
typedef __PACKED_STRUCT
{
  uint16_t Handle;
} Handle_Item_t;

/* Internal types used by process functions */

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Reason;
} hci_disconnect_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_disconnect_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
} hci_read_remote_version_information_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_read_remote_version_information_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Event_Mask[8];
} hci_set_event_mask_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_set_event_mask_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_reset_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Type;
} hci_read_transmit_power_level_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint16_t Connection_Handle;
  uint8_t Transmit_Power_Level;
} hci_read_transmit_power_level_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Flow_Control_Enable;
} hci_set_controller_to_host_flow_control_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_set_controller_to_host_flow_control_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Host_ACL_Data_Packet_Length;
  uint8_t Host_Synchronous_Data_Packet_Length;
  uint16_t Host_Total_Num_ACL_Data_Packets;
  uint16_t Host_Total_Num_Synchronous_Data_Packets;
} hci_host_buffer_size_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_host_buffer_size_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Number_Of_Handles;
  Host_Nb_Of_Completed_Pkt_Pair_t Host_Nb_Of_Completed_Pkt_Pair[(BLE_CMD_MAX_PARAM_LEN - 1)/sizeof(Host_Nb_Of_Completed_Pkt_Pair_t)];
} hci_host_number_of_completed_packets_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_host_number_of_completed_packets_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint8_t HCI_Version;
  uint16_t HCI_Subversion;
  uint8_t LMP_Version;
  uint16_t Company_Identifier;
  uint16_t LMP_Subversion;
} hci_read_local_version_information_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint8_t Supported_Commands[64];
} hci_read_local_supported_commands_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint8_t LMP_Features[8];
} hci_read_local_supported_features_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint8_t BD_ADDR[6];
} hci_read_bd_addr_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
} hci_read_rssi_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint16_t Connection_Handle;
  uint8_t RSSI;
} hci_read_rssi_rp0;

typedef __PACKED_STRUCT
{
  uint8_t LE_Event_Mask[8];
} hci_le_set_event_mask_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_set_event_mask_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint16_t HC_LE_ACL_Data_Packet_Length;
  uint8_t HC_Total_Num_LE_ACL_Data_Packets;
} hci_le_read_buffer_size_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint8_t LE_Features[8];
} hci_le_read_local_supported_features_page_0_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Random_Address[6];
} hci_le_set_random_address_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_set_random_address_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Advertising_Interval_Min;
  uint16_t Advertising_Interval_Max;
  uint8_t Advertising_Type;
  uint8_t Own_Address_Type;
  uint8_t Peer_Address_Type;
  uint8_t Peer_Address[6];
  uint8_t Advertising_Channel_Map;
  uint8_t Advertising_Filter_Policy;
} hci_le_set_advertising_parameters_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_set_advertising_parameters_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint8_t Transmit_Power_Level;
} hci_le_read_advertising_physical_channel_tx_power_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Advertising_Data_Length;
  uint8_t Advertising_Data[31];
} hci_le_set_advertising_data_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_set_advertising_data_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Scan_Response_Data_Length;
  uint8_t Scan_Response_Data[31];
} hci_le_set_scan_response_data_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_set_scan_response_data_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Advertising_Enable;
} hci_le_set_advertising_enable_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_set_advertising_enable_rp0;

typedef __PACKED_STRUCT
{
  uint8_t LE_Scan_Type;
  uint16_t LE_Scan_Interval;
  uint16_t LE_Scan_Window;
  uint8_t Own_Address_Type;
  uint8_t Scanning_Filter_Policy;
} hci_le_set_scan_parameters_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_set_scan_parameters_rp0;

typedef __PACKED_STRUCT
{
  uint8_t LE_Scan_Enable;
  uint8_t Filter_Duplicates;
} hci_le_set_scan_enable_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_set_scan_enable_rp0;

typedef __PACKED_STRUCT
{
  uint16_t LE_Scan_Interval;
  uint16_t LE_Scan_Window;
  uint8_t Initiator_Filter_Policy;
  uint8_t Peer_Address_Type;
  uint8_t Peer_Address[6];
  uint8_t Own_Address_Type;
  uint16_t Conn_Interval_Min;
  uint16_t Conn_Interval_Max;
  uint16_t Conn_Latency;
  uint16_t Supervision_Timeout;
  uint16_t Minimum_CE_Length;
  uint16_t Maximum_CE_Length;
} hci_le_create_connection_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_create_connection_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_create_connection_cancel_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint8_t Filter_Accept_List_Size;
} hci_le_read_filter_accept_list_size_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_clear_filter_accept_list_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Address_Type;
  uint8_t Address[6];
} hci_le_add_device_to_filter_accept_list_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_add_device_to_filter_accept_list_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Address_Type;
  uint8_t Address[6];
} hci_le_remove_device_from_filter_accept_list_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_remove_device_from_filter_accept_list_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Conn_Interval_Min;
  uint16_t Conn_Interval_Max;
  uint16_t Conn_Latency;
  uint16_t Supervision_Timeout;
  uint16_t Minimum_CE_Length;
  uint16_t Maximum_CE_Length;
} hci_le_connection_update_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_connection_update_rp0;

typedef __PACKED_STRUCT
{
  uint8_t LE_Channel_Map[5];
} hci_le_set_host_channel_classification_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_set_host_channel_classification_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
} hci_le_read_channel_map_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint16_t Connection_Handle;
  uint8_t LE_Channel_Map[5];
} hci_le_read_channel_map_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
} hci_le_read_remote_features_page_0_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_read_remote_features_page_0_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Key[16];
  uint8_t Plaintext_Data[16];
} hci_le_encrypt_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint8_t Encrypted_Data[16];
} hci_le_encrypt_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint8_t Random_Number[8];
} hci_le_rand_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Random_Number[8];
  uint16_t Encrypted_Diversifier;
  uint8_t Long_Term_Key[16];
} hci_le_enable_encryption_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_enable_encryption_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Long_Term_Key[16];
} hci_le_long_term_key_request_reply_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint16_t Connection_Handle;
} hci_le_long_term_key_request_reply_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
} hci_le_long_term_key_request_negative_reply_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint16_t Connection_Handle;
} hci_le_long_term_key_request_negative_reply_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint8_t LE_States[8];
} hci_le_read_supported_states_rp0;

typedef __PACKED_STRUCT
{
  uint8_t RX_Frequency;
} hci_le_receiver_test_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_receiver_test_rp0;

typedef __PACKED_STRUCT
{
  uint8_t TX_Frequency;
  uint8_t Length_Of_Test_Data;
  uint8_t Packet_Payload;
} hci_le_transmitter_test_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_transmitter_test_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint16_t Number_Of_Packets;
} hci_le_test_end_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t TxOctets;
  uint16_t TxTime;
} hci_le_set_data_length_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint16_t Connection_Handle;
} hci_le_set_data_length_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint16_t SuggestedMaxTxOctets;
  uint16_t SuggestedMaxTxTime;
} hci_le_read_suggested_default_data_length_rp0;

typedef __PACKED_STRUCT
{
  uint16_t SuggestedMaxTxOctets;
  uint16_t SuggestedMaxTxTime;
} hci_le_write_suggested_default_data_length_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_write_suggested_default_data_length_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_read_local_p256_public_key_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Remote_P256_Public_Key[64];
} hci_le_generate_dhkey_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_generate_dhkey_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Peer_Identity_Address_Type;
  uint8_t Peer_Identity_Address[6];
  uint8_t Peer_IRK[16];
  uint8_t Local_IRK[16];
} hci_le_add_device_to_resolving_list_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_add_device_to_resolving_list_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Peer_Identity_Address_Type;
  uint8_t Peer_Identity_Address[6];
} hci_le_remove_device_from_resolving_list_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_remove_device_from_resolving_list_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_clear_resolving_list_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint8_t Resolving_List_Size;
} hci_le_read_resolving_list_size_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Peer_Identity_Address_Type;
  uint8_t Peer_Identity_Address[6];
} hci_le_read_peer_resolvable_address_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint8_t Peer_Resolvable_Address[6];
} hci_le_read_peer_resolvable_address_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Peer_Identity_Address_Type;
  uint8_t Peer_Identity_Address[6];
} hci_le_read_local_resolvable_address_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint8_t Local_Resolvable_Address[6];
} hci_le_read_local_resolvable_address_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Address_Resolution_Enable;
} hci_le_set_address_resolution_enable_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_set_address_resolution_enable_rp0;

typedef __PACKED_STRUCT
{
  uint16_t RPA_Timeout;
} hci_le_set_resolvable_private_address_timeout_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_set_resolvable_private_address_timeout_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint16_t supportedMaxTxOctets;
  uint16_t supportedMaxTxTime;
  uint16_t supportedMaxRxOctets;
  uint16_t supportedMaxRxTime;
} hci_le_read_maximum_data_length_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
} hci_le_read_phy_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint16_t Connection_Handle;
  uint8_t TX_PHY;
  uint8_t RX_PHY;
} hci_le_read_phy_rp0;

typedef __PACKED_STRUCT
{
  uint8_t ALL_PHYS;
  uint8_t TX_PHYS;
  uint8_t RX_PHYS;
} hci_le_set_default_phy_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_set_default_phy_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t ALL_PHYS;
  uint8_t TX_PHYS;
  uint8_t RX_PHYS;
  uint16_t PHY_options;
} hci_le_set_phy_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_set_phy_rp0;

typedef __PACKED_STRUCT
{
  uint8_t RX_Frequency;
  uint8_t PHY;
  uint8_t Modulation_Index;
} hci_le_receiver_test_v2_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_receiver_test_v2_rp0;

typedef __PACKED_STRUCT
{
  uint8_t TX_Frequency;
  uint8_t Length_Of_Test_Data;
  uint8_t Packet_Payload;
  uint8_t PHY;
} hci_le_transmitter_test_v2_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_transmitter_test_v2_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Advertising_Handle;
  uint8_t Random_Address[6];
} hci_le_set_advertising_set_random_address_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_set_advertising_set_random_address_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Advertising_Handle;
  uint16_t Adv_Event_Properties;
  uint8_t Primary_Adv_Interval_Min[3];
  uint8_t Primary_Adv_Interval_Max[3];
  uint8_t Primary_Adv_Channel_Map;
  uint8_t Own_Address_Type;
  uint8_t Peer_Address_Type;
  uint8_t Peer_Address[6];
  uint8_t Adv_Filter_Policy;
  uint8_t Adv_TX_Power;
  uint8_t Primary_Adv_PHY;
  uint8_t Secondary_Adv_Max_Skip;
  uint8_t Secondary_Adv_PHY;
  uint8_t Adv_SID;
  uint8_t Scan_Req_Notification_Enable;
} hci_le_set_extended_advertising_parameters_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint8_t Selected_TX_Power;
} hci_le_set_extended_advertising_parameters_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Advertising_Handle;
  uint8_t Operation;
  uint8_t Fragment_Preference;
  uint8_t Advertising_Data_Length;
  uint8_t Advertising_Data[BLE_CMD_MAX_PARAM_LEN - 4];
} hci_le_set_extended_advertising_data_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_set_extended_advertising_data_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Advertising_Handle;
  uint8_t Operation;
  uint8_t Fragment_Preference;
  uint8_t Scan_Response_Data_Length;
  uint8_t Scan_Response_Data[BLE_CMD_MAX_PARAM_LEN - 4];
} hci_le_set_extended_scan_response_data_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_set_extended_scan_response_data_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Enable;
  uint8_t Num_Sets;
  Adv_Set_t Adv_Set[(BLE_CMD_MAX_PARAM_LEN - 2)/sizeof(Adv_Set_t)];
} hci_le_set_extended_advertising_enable_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_set_extended_advertising_enable_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint16_t Max_Advertising_Data_Length;
} hci_le_read_maximum_advertising_data_length_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint8_t Num_Supported_Advertising_Sets;
} hci_le_read_number_of_supported_advertising_sets_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Advertising_Handle;
} hci_le_remove_advertising_set_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_remove_advertising_set_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_clear_advertising_sets_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Own_Address_Type;
  uint8_t Scanning_Filter_Policy;
  uint8_t Scanning_PHYs;
  Scan_Param_Phy_t Scan_Param_Phy[2];
} hci_le_set_extended_scan_parameters_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_set_extended_scan_parameters_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Enable;
  uint8_t Filter_Duplicates;
  uint16_t Duration;
  uint16_t Period;
} hci_le_set_extended_scan_enable_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_set_extended_scan_enable_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Initiator_Filter_Policy;
  uint8_t Own_Address_Type;
  uint8_t Peer_Address_Type;
  uint8_t Peer_Address[6];
  uint8_t Initiating_PHYs;
  Init_Param_Phy_t Init_Param_Phy[3];
} hci_le_extended_create_connection_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_extended_create_connection_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint8_t Min_TX_Power;
  uint8_t Max_TX_Power;
} hci_le_read_transmit_power_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint16_t RF_TX_Path_Compensation;
  uint16_t RF_RX_Path_Compensation;
} hci_le_read_rf_path_compensation_rp0;

typedef __PACKED_STRUCT
{
  uint16_t RF_TX_Path_Compensation;
  uint16_t RF_RX_Path_Compensation;
} hci_le_write_rf_path_compensation_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_write_rf_path_compensation_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Peer_Identity_Address_Type;
  uint8_t Peer_Identity_Address[6];
  uint8_t Privacy_Mode;
} hci_le_set_privacy_mode_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_set_privacy_mode_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Remote_P256_Public_Key[64];
  uint8_t Key_Type;
} hci_le_generate_dhkey_v2_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_generate_dhkey_v2_rp0;

typedef __PACKED_STRUCT
{
  uint16_t RPA_Timeout_Min;
  uint16_t RPA_Timeout_Max;
} hci_le_set_resolvable_private_address_timeout_v2_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} hci_le_set_resolvable_private_address_timeout_v2_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Offset;
  uint8_t Length;
  uint8_t Value[BLE_CMD_MAX_PARAM_LEN - 2];
} aci_hal_write_config_data_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_hal_write_config_data_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Offset;
} aci_hal_read_config_data_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint8_t Data_Length;
  uint8_t Data[(BLE_EVT_MAX_PARAM_LEN - 3) - 2];
} aci_hal_read_config_data_rp0;

typedef __PACKED_STRUCT
{
  uint8_t En_High_Power;
  uint8_t PA_Level;
} aci_hal_set_tx_power_level_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_hal_set_tx_power_level_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint32_t Number_Of_Packets;
} aci_hal_le_tx_test_packet_number_rp0;

typedef __PACKED_STRUCT
{
  uint8_t RF_Channel;
  uint8_t Freq_offset;
} aci_hal_tone_start_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_hal_tone_start_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_hal_tone_stop_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint8_t Link_Status[8];
  uint16_t Link_Connection_Handle[8];
} aci_hal_get_link_status_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Radio_Activity_Mask;
} aci_hal_set_radio_activity_mask_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_hal_set_radio_activity_mask_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint32_t Anchor_Period;
  uint32_t Max_Free_Slot;
} aci_hal_get_anchor_period_rp0;

typedef __PACKED_STRUCT
{
  uint32_t Event_Mask;
} aci_hal_set_event_mask_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_hal_set_event_mask_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Enable;
} aci_hal_set_peripheral_latency_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_hal_set_peripheral_latency_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint8_t RSSI;
} aci_hal_read_rssi_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Mode;
  uint8_t Key[16];
  uint8_t IV[8];
  uint16_t In_Data_Length;
  uint8_t In_Data[BLE_CMD_MAX_PARAM_LEN - 27];
} aci_hal_ead_encrypt_decrypt_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint16_t Out_Data_Length;
  uint8_t Out_Data[(BLE_EVT_MAX_PARAM_LEN - 3) - 3];
} aci_hal_ead_encrypt_decrypt_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Register_Address;
} aci_hal_read_radio_reg_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint8_t reg_val;
} aci_hal_read_radio_reg_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Register_Address;
  uint8_t Register_Value;
} aci_hal_write_radio_reg_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_hal_write_radio_reg_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint8_t Value[3];
} aci_hal_read_raw_rssi_rp0;

typedef __PACKED_STRUCT
{
  uint8_t RF_Channel;
} aci_hal_rx_start_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_hal_rx_start_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_hal_rx_stop_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_set_non_discoverable_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Advertising_Type;
  uint16_t Advertising_Interval_Min;
  uint16_t Advertising_Interval_Max;
  uint8_t Own_Address_Type;
  uint8_t Advertising_Filter_Policy;
  uint8_t Local_Name_Length;
  uint8_t Local_Name[BLE_CMD_MAX_PARAM_LEN - 13];
} aci_gap_set_limited_discoverable_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Service_Uuid_length;
  uint8_t Service_Uuid_List[BLE_CMD_MAX_PARAM_LEN - 13];
} aci_gap_set_limited_discoverable_cp1;

typedef __PACKED_STRUCT
{
  uint16_t Conn_Interval_Min;
  uint16_t Conn_Interval_Max;
} aci_gap_set_limited_discoverable_cp2;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_set_limited_discoverable_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Advertising_Type;
  uint16_t Advertising_Interval_Min;
  uint16_t Advertising_Interval_Max;
  uint8_t Own_Address_Type;
  uint8_t Advertising_Filter_Policy;
  uint8_t Local_Name_Length;
  uint8_t Local_Name[BLE_CMD_MAX_PARAM_LEN - 13];
} aci_gap_set_discoverable_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Service_Uuid_length;
  uint8_t Service_Uuid_List[BLE_CMD_MAX_PARAM_LEN - 13];
} aci_gap_set_discoverable_cp1;

typedef __PACKED_STRUCT
{
  uint16_t Conn_Interval_Min;
  uint16_t Conn_Interval_Max;
} aci_gap_set_discoverable_cp2;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_set_discoverable_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Own_Address_Type;
  uint8_t Directed_Advertising_Type;
  uint8_t Direct_Address_Type;
  uint8_t Direct_Address[6];
  uint16_t Advertising_Interval_Min;
  uint16_t Advertising_Interval_Max;
} aci_gap_set_direct_connectable_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_set_direct_connectable_rp0;

typedef __PACKED_STRUCT
{
  uint8_t IO_Capability;
} aci_gap_set_io_capability_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_set_io_capability_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Bonding_Mode;
  uint8_t MITM_Mode;
  uint8_t SC_Support;
  uint8_t KeyPress_Notification_Support;
  uint8_t Min_Encryption_Key_Size;
  uint8_t Max_Encryption_Key_Size;
  uint8_t Use_Fixed_Pin;
  uint32_t Fixed_Pin;
  uint8_t Identity_Address_Type;
} aci_gap_set_authentication_requirement_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_set_authentication_requirement_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Authorization_Enable;
} aci_gap_set_authorization_requirement_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_set_authorization_requirement_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint32_t Pass_Key;
} aci_gap_pass_key_resp_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_pass_key_resp_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Authorize;
} aci_gap_authorization_resp_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_authorization_resp_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Role;
  uint8_t privacy_enabled;
  uint8_t device_name_char_len;
} aci_gap_init_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint16_t Service_Handle;
  uint16_t Dev_Name_Char_Handle;
  uint16_t Appearance_Char_Handle;
} aci_gap_init_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Advertising_Event_Type;
  uint8_t Own_Address_Type;
} aci_gap_set_non_connectable_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_set_non_connectable_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Advertising_Interval_Min;
  uint16_t Advertising_Interval_Max;
  uint8_t Own_Address_Type;
  uint8_t Adv_Filter_Policy;
} aci_gap_set_undirected_connectable_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_set_undirected_connectable_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
} aci_gap_peripheral_security_req_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_peripheral_security_req_rp0;

typedef __PACKED_STRUCT
{
  uint8_t AdvDataLen;
  uint8_t AdvData[BLE_CMD_MAX_PARAM_LEN - 1];
} aci_gap_update_adv_data_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_update_adv_data_rp0;

typedef __PACKED_STRUCT
{
  uint8_t ADType;
} aci_gap_delete_ad_type_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_delete_ad_type_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
} aci_gap_get_security_level_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint8_t Security_Mode;
  uint8_t Security_Level;
} aci_gap_get_security_level_rp0;

typedef __PACKED_STRUCT
{
  uint16_t GAP_Evt_Mask;
} aci_gap_set_event_mask_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_set_event_mask_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_configure_filter_accept_list_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Reason;
} aci_gap_terminate_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_terminate_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_clear_security_db_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
} aci_gap_allow_rebond_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_allow_rebond_rp0;

typedef __PACKED_STRUCT
{
  uint16_t LE_Scan_Interval;
  uint16_t LE_Scan_Window;
  uint8_t Own_Address_Type;
  uint8_t Filter_Duplicates;
} aci_gap_start_limited_discovery_proc_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_start_limited_discovery_proc_rp0;

typedef __PACKED_STRUCT
{
  uint16_t LE_Scan_Interval;
  uint16_t LE_Scan_Window;
  uint8_t Own_Address_Type;
  uint8_t Filter_Duplicates;
} aci_gap_start_general_discovery_proc_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_start_general_discovery_proc_rp0;

typedef __PACKED_STRUCT
{
  uint16_t LE_Scan_Interval;
  uint16_t LE_Scan_Window;
  uint8_t Own_Address_Type;
  uint16_t Conn_Interval_Min;
  uint16_t Conn_Interval_Max;
  uint16_t Conn_Latency;
  uint16_t Supervision_Timeout;
  uint16_t Minimum_CE_Length;
  uint16_t Maximum_CE_Length;
  uint8_t Num_of_Peer_Entries;
  Peer_Entry_t Peer_Entry[(BLE_CMD_MAX_PARAM_LEN - 18)/sizeof(Peer_Entry_t)];
} aci_gap_start_auto_connection_establish_proc_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_start_auto_connection_establish_proc_rp0;

typedef __PACKED_STRUCT
{
  uint8_t LE_Scan_Type;
  uint16_t LE_Scan_Interval;
  uint16_t LE_Scan_Window;
  uint8_t Own_Address_Type;
  uint8_t Scanning_Filter_Policy;
  uint8_t Filter_Duplicates;
} aci_gap_start_general_connection_establish_proc_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_start_general_connection_establish_proc_rp0;

typedef __PACKED_STRUCT
{
  uint8_t LE_Scan_Type;
  uint16_t LE_Scan_Interval;
  uint16_t LE_Scan_Window;
  uint8_t Own_Address_Type;
  uint8_t Scanning_Filter_Policy;
  uint8_t Filter_Duplicates;
  uint8_t Num_of_Peer_Entries;
  Peer_Entry_t Peer_Entry[(BLE_CMD_MAX_PARAM_LEN - 9)/sizeof(Peer_Entry_t)];
} aci_gap_start_selective_connection_establish_proc_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_start_selective_connection_establish_proc_rp0;

typedef __PACKED_STRUCT
{
  uint16_t LE_Scan_Interval;
  uint16_t LE_Scan_Window;
  uint8_t Peer_Address_Type;
  uint8_t Peer_Address[6];
  uint8_t Own_Address_Type;
  uint16_t Conn_Interval_Min;
  uint16_t Conn_Interval_Max;
  uint16_t Conn_Latency;
  uint16_t Supervision_Timeout;
  uint16_t Minimum_CE_Length;
  uint16_t Maximum_CE_Length;
} aci_gap_create_connection_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_create_connection_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Procedure_Code;
} aci_gap_terminate_gap_proc_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_terminate_gap_proc_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Conn_Interval_Min;
  uint16_t Conn_Interval_Max;
  uint16_t Conn_Latency;
  uint16_t Supervision_Timeout;
  uint16_t Minimum_CE_Length;
  uint16_t Maximum_CE_Length;
} aci_gap_start_connection_update_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_start_connection_update_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Force_Rebond;
} aci_gap_send_pairing_req_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_send_pairing_req_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Advertising_Interval_Min;
  uint16_t Advertising_Interval_Max;
  uint8_t Advertising_Type;
  uint8_t Own_Address_Type;
  uint8_t Adv_Data_Length;
  uint8_t Adv_Data[BLE_CMD_MAX_PARAM_LEN - 8];
} aci_gap_set_broadcast_mode_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Num_of_Peer_Entries;
  Peer_Entry_t Peer_Entry[(BLE_CMD_MAX_PARAM_LEN - 8)/sizeof(Peer_Entry_t)];
} aci_gap_set_broadcast_mode_cp1;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_set_broadcast_mode_rp0;

typedef __PACKED_STRUCT
{
  uint16_t LE_Scan_Interval;
  uint16_t LE_Scan_Window;
  uint8_t LE_Scan_Type;
  uint8_t Own_Address_Type;
  uint8_t Filter_Duplicates;
  uint8_t Scanning_Filter_Policy;
} aci_gap_start_observation_proc_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_start_observation_proc_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint8_t Num_of_Addresses;
  Bonded_Device_Entry_t Bonded_Device_Entry[((BLE_EVT_MAX_PARAM_LEN - 3) - 2)/sizeof(Bonded_Device_Entry_t)];
} aci_gap_get_bonded_devices_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Peer_Address_Type;
  uint8_t Peer_Address[6];
} aci_gap_check_bonded_device_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint8_t Id_Address_Type;
  uint8_t Id_Address[6];
} aci_gap_check_bonded_device_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Confirm_Yes_No;
} aci_gap_numeric_comparison_value_confirm_yesno_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_numeric_comparison_value_confirm_yesno_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Input_Type;
} aci_gap_passkey_input_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_passkey_input_rp0;

typedef __PACKED_STRUCT
{
  uint8_t OOB_Data_Type;
} aci_gap_get_oob_data_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint8_t Address_Type;
  uint8_t Address[6];
  uint8_t OOB_Data_Type;
  uint8_t OOB_Data_Len;
  uint8_t OOB_Data[16];
} aci_gap_get_oob_data_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Device_Type;
  uint8_t Address_Type;
  uint8_t Address[6];
  uint8_t OOB_Data_Type;
  uint8_t OOB_Data_Len;
  uint8_t OOB_Data[16];
} aci_gap_set_oob_data_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_set_oob_data_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Peer_Identity_Address_Type;
  uint8_t Peer_Identity_Address[6];
} aci_gap_remove_bonded_device_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_remove_bonded_device_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Num_of_List_Entries;
  List_Entry_t List_Entry[(BLE_CMD_MAX_PARAM_LEN - 2)/sizeof(List_Entry_t)];
} aci_gap_add_devices_to_list_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Mode;
} aci_gap_add_devices_to_list_cp1;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_add_devices_to_list_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Accept;
} aci_gap_pairing_request_reply_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_pairing_request_reply_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Adv_Interval_Min;
  uint16_t Adv_Interval_Max;
  uint8_t Adv_Channel_Map;
  uint8_t Own_Address_Type;
  uint8_t Own_Address[6];
  uint8_t PA_Level;
} aci_gap_additional_beacon_start_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_additional_beacon_start_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_additional_beacon_stop_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Adv_Data_Length;
  uint8_t Adv_Data[BLE_CMD_MAX_PARAM_LEN - 1];
} aci_gap_additional_beacon_set_data_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_additional_beacon_set_data_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Adv_Mode;
  uint8_t Advertising_Handle;
  uint16_t Adv_Event_Properties;
  uint32_t Primary_Adv_Interval_Min;
  uint32_t Primary_Adv_Interval_Max;
  uint8_t Primary_Adv_Channel_Map;
  uint8_t Own_Address_Type;
  uint8_t Peer_Address_Type;
  uint8_t Peer_Address[6];
  uint8_t Adv_Filter_Policy;
  uint8_t Adv_TX_Power;
  uint8_t Secondary_Adv_Max_Skip;
  uint8_t Secondary_Adv_PHY;
  uint8_t Adv_SID;
  uint8_t Scan_Req_Notification_Enable;
} aci_gap_adv_set_configuration_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_adv_set_configuration_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Enable;
  uint8_t Num_Sets;
  Adv_Set_t Adv_Set[(BLE_CMD_MAX_PARAM_LEN - 2)/sizeof(Adv_Set_t)];
} aci_gap_adv_set_enable_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_adv_set_enable_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Advertising_Handle;
  uint8_t Operation;
  uint8_t Fragment_Preference;
  uint8_t Advertising_Data_Length;
  uint8_t Advertising_Data[BLE_CMD_MAX_PARAM_LEN - 4];
} aci_gap_adv_set_adv_data_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_adv_set_adv_data_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Advertising_Handle;
  uint8_t Operation;
  uint8_t Fragment_Preference;
  uint8_t Scan_Response_Data_Length;
  uint8_t Scan_Response_Data[BLE_CMD_MAX_PARAM_LEN - 4];
} aci_gap_adv_set_scan_resp_data_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_adv_set_scan_resp_data_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Advertising_Handle;
} aci_gap_adv_remove_set_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_adv_remove_set_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_adv_clear_sets_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Advertising_Handle;
  uint8_t Random_Address[6];
} aci_gap_adv_set_random_address_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_adv_set_random_address_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Scan_Mode;
  uint8_t Procedure;
  uint8_t Own_Address_Type;
  uint8_t Filter_Duplicates;
  uint16_t Duration;
  uint16_t Period;
  uint8_t Scanning_Filter_Policy;
  uint8_t Scanning_PHYs;
  Scan_Param_Phy_t Scan_Param_Phy[2];
} aci_gap_ext_start_scan_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_ext_start_scan_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Initiating_Mode;
  uint8_t Procedure;
  uint8_t Own_Address_Type;
  uint8_t Peer_Address_Type;
  uint8_t Peer_Address[6];
  uint8_t Advertising_Handle;
  uint8_t Subevent;
  uint8_t Initiator_Filter_Policy;
  uint8_t Initiating_PHYs;
  Init_Param_Phy_t Init_Param_Phy[3];
} aci_gap_ext_create_connection_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gap_ext_create_connection_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_init_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Service_UUID_Type;
  Service_UUID_t Service_UUID;
} aci_gatt_add_service_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Service_Type;
  uint8_t Max_Attribute_Records;
} aci_gatt_add_service_cp1;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint16_t Service_Handle;
} aci_gatt_add_service_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Service_Handle;
  uint16_t Include_Start_Handle;
  uint16_t Include_End_Handle;
  uint8_t Include_UUID_Type;
  Include_UUID_t Include_UUID;
} aci_gatt_include_service_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint16_t Include_Handle;
} aci_gatt_include_service_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Service_Handle;
  uint8_t Char_UUID_Type;
  Char_UUID_t Char_UUID;
} aci_gatt_add_char_cp0;

typedef __PACKED_STRUCT
{
  uint16_t Char_Value_Length;
  uint8_t Char_Properties;
  uint8_t Security_Permissions;
  uint8_t GATT_Evt_Mask;
  uint8_t Enc_Key_Size;
  uint8_t Is_Variable;
} aci_gatt_add_char_cp1;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint16_t Char_Handle;
} aci_gatt_add_char_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Service_Handle;
  uint16_t Char_Handle;
  uint8_t Char_Desc_Uuid_Type;
  Char_Desc_Uuid_t Char_Desc_Uuid;
} aci_gatt_add_char_desc_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Char_Desc_Value_Max_Len;
  uint8_t Char_Desc_Value_Length;
  uint8_t Char_Desc_Value[BLE_CMD_MAX_PARAM_LEN - 12];
} aci_gatt_add_char_desc_cp1;

typedef __PACKED_STRUCT
{
  uint8_t Security_Permissions;
  uint8_t Access_Permissions;
  uint8_t GATT_Evt_Mask;
  uint8_t Enc_Key_Size;
  uint8_t Is_Variable;
} aci_gatt_add_char_desc_cp2;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint16_t Char_Desc_Handle;
} aci_gatt_add_char_desc_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Service_Handle;
  uint16_t Char_Handle;
  uint8_t Val_Offset;
  uint8_t Char_Value_Length;
  uint8_t Char_Value[BLE_CMD_MAX_PARAM_LEN - 6];
} aci_gatt_update_char_value_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_update_char_value_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Serv_Handle;
  uint16_t Char_Handle;
} aci_gatt_del_char_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_del_char_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Serv_Handle;
} aci_gatt_del_service_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_del_service_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Serv_Handle;
  uint16_t Include_Handle;
} aci_gatt_del_include_service_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_del_include_service_rp0;

typedef __PACKED_STRUCT
{
  uint32_t GATT_Evt_Mask;
} aci_gatt_set_event_mask_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_set_event_mask_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
} aci_gatt_exchange_config_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_exchange_config_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Start_Handle;
  uint16_t End_Handle;
} aci_att_find_info_req_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_att_find_info_req_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Start_Handle;
  uint16_t End_Handle;
  uint16_t UUID;
  uint8_t Attribute_Val_Length;
  uint8_t Attribute_Val[BLE_CMD_MAX_PARAM_LEN - 9];
} aci_att_find_by_type_value_req_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_att_find_by_type_value_req_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Start_Handle;
  uint16_t End_Handle;
  uint8_t UUID_Type;
  UUID_t UUID;
} aci_att_read_by_type_req_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_att_read_by_type_req_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Start_Handle;
  uint16_t End_Handle;
  uint8_t UUID_Type;
  UUID_t UUID;
} aci_att_read_by_group_type_req_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_att_read_by_group_type_req_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Attr_Handle;
  uint16_t Val_Offset;
  uint8_t Attribute_Val_Length;
  uint8_t Attribute_Val[BLE_CMD_MAX_PARAM_LEN - 7];
} aci_att_prepare_write_req_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_att_prepare_write_req_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Execute;
} aci_att_execute_write_req_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_att_execute_write_req_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
} aci_gatt_disc_all_primary_services_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_disc_all_primary_services_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t UUID_Type;
  UUID_t UUID;
} aci_gatt_disc_primary_service_by_uuid_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_disc_primary_service_by_uuid_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Start_Handle;
  uint16_t End_Handle;
} aci_gatt_find_included_services_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_find_included_services_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Start_Handle;
  uint16_t End_Handle;
} aci_gatt_disc_all_char_of_service_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_disc_all_char_of_service_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Start_Handle;
  uint16_t End_Handle;
  uint8_t UUID_Type;
  UUID_t UUID;
} aci_gatt_disc_char_by_uuid_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_disc_char_by_uuid_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Char_Handle;
  uint16_t End_Handle;
} aci_gatt_disc_all_char_desc_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_disc_all_char_desc_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Attr_Handle;
} aci_gatt_read_char_value_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_read_char_value_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Start_Handle;
  uint16_t End_Handle;
  uint8_t UUID_Type;
  UUID_t UUID;
} aci_gatt_read_using_char_uuid_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_read_using_char_uuid_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Attr_Handle;
  uint16_t Val_Offset;
} aci_gatt_read_long_char_value_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_read_long_char_value_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Number_of_Handles;
  Handle_Entry_t Handle_Entry[(BLE_CMD_MAX_PARAM_LEN - 3)/sizeof(Handle_Entry_t)];
} aci_gatt_read_multiple_char_value_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_read_multiple_char_value_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Attr_Handle;
  uint8_t Attribute_Val_Length;
  uint8_t Attribute_Val[BLE_CMD_MAX_PARAM_LEN - 5];
} aci_gatt_write_char_value_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_write_char_value_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Attr_Handle;
  uint16_t Val_Offset;
  uint8_t Attribute_Val_Length;
  uint8_t Attribute_Val[BLE_CMD_MAX_PARAM_LEN - 7];
} aci_gatt_write_long_char_value_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_write_long_char_value_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Attr_Handle;
  uint16_t Val_Offset;
  uint8_t Attribute_Val_Length;
  uint8_t Attribute_Val[BLE_CMD_MAX_PARAM_LEN - 7];
} aci_gatt_write_char_reliable_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_write_char_reliable_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Attr_Handle;
  uint8_t Attribute_Val_Length;
  uint8_t Attribute_Val[BLE_CMD_MAX_PARAM_LEN - 5];
} aci_gatt_write_without_resp_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_write_without_resp_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Attr_Handle;
  uint8_t Attribute_Val_Length;
  uint8_t Attribute_Val[BLE_CMD_MAX_PARAM_LEN - 5];
} aci_gatt_signed_write_without_resp_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_signed_write_without_resp_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
} aci_gatt_confirm_indication_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_confirm_indication_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Attr_Handle;
  uint8_t Write_status;
  uint8_t Error_Code;
  uint8_t Attribute_Val_Length;
  uint8_t Attribute_Val[BLE_CMD_MAX_PARAM_LEN - 7];
} aci_gatt_permit_write_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_permit_write_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Read_status;
  uint8_t Error_Code;
  uint16_t Attr_Handle;
} aci_gatt_permit_read_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_permit_read_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Serv_Handle;
  uint16_t Attr_Handle;
  uint8_t Security_Permissions;
} aci_gatt_set_security_permission_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_set_security_permission_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Serv_Handle;
  uint16_t Char_Handle;
  uint16_t Char_Desc_Handle;
  uint16_t Val_Offset;
  uint8_t Char_Desc_Value_Length;
  uint8_t Char_Desc_Value[BLE_CMD_MAX_PARAM_LEN - 9];
} aci_gatt_set_desc_value_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_set_desc_value_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Attr_Handle;
  uint16_t Offset;
  uint16_t Value_Length_Requested;
} aci_gatt_read_handle_value_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint16_t Length;
  uint16_t Value_Length;
  uint8_t Value[(BLE_EVT_MAX_PARAM_LEN - 3) - 5];
} aci_gatt_read_handle_value_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Conn_Handle_To_Notify;
  uint16_t Service_Handle;
  uint16_t Char_Handle;
  uint8_t Update_Type;
  uint16_t Char_Length;
  uint16_t Value_Offset;
  uint8_t Value_Length;
  uint8_t Value[BLE_CMD_MAX_PARAM_LEN - 12];
} aci_gatt_update_char_value_ext_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_update_char_value_ext_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Serv_Handle;
  uint16_t Attr_Handle;
  uint8_t Access_Permissions;
} aci_gatt_set_access_permission_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_set_access_permission_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_store_db_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Number_of_Handles;
  Handle_Entry_t Handle_Entry[(BLE_CMD_MAX_PARAM_LEN - 3)/sizeof(Handle_Entry_t)];
} aci_gatt_send_mult_notification_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_send_mult_notification_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Number_of_Handles;
  Handle_Entry_t Handle_Entry[(BLE_CMD_MAX_PARAM_LEN - 3)/sizeof(Handle_Entry_t)];
} aci_gatt_read_multiple_var_char_value_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_read_multiple_var_char_value_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Attr_Handle;
  uint8_t Signed_Mode;
  uint16_t Data_Length;
  uint32_t Data_Pointer;
} aci_gatt_write_without_resp_ext_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_write_without_resp_ext_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Attr_Handle;
  uint8_t Write_Mode;
  uint16_t Val_Offset;
  uint16_t Data_Length;
  uint32_t Data_Pointer;
} aci_gatt_write_with_resp_ext_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_gatt_write_with_resp_ext_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Conn_Interval_Min;
  uint16_t Conn_Interval_Max;
  uint16_t Latency;
  uint16_t Timeout_Multiplier;
} aci_l2cap_connection_parameter_update_req_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_l2cap_connection_parameter_update_req_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Conn_Interval_Min;
  uint16_t Conn_Interval_Max;
  uint16_t Latency;
  uint16_t Timeout_Multiplier;
  uint16_t Minimum_CE_Length;
  uint16_t Maximum_CE_Length;
  uint8_t Identifier;
  uint8_t Accept;
} aci_l2cap_connection_parameter_update_resp_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_l2cap_connection_parameter_update_resp_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t SPSM;
  uint16_t MTU;
  uint16_t MPS;
  uint16_t Initial_Credits;
  uint8_t Channel_Number;
} aci_l2cap_coc_connect_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_l2cap_coc_connect_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t MTU;
  uint16_t MPS;
  uint16_t Initial_Credits;
  uint16_t Result;
  uint8_t Max_Channel_Number;
} aci_l2cap_coc_connect_confirm_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint8_t Channel_Number;
  uint8_t Channel_Index_List[(BLE_EVT_MAX_PARAM_LEN - 3) - 2];
} aci_l2cap_coc_connect_confirm_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t MTU;
  uint16_t MPS;
  uint8_t Channel_Number;
  uint8_t Channel_Index_List[BLE_CMD_MAX_PARAM_LEN - 7];
} aci_l2cap_coc_reconf_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_l2cap_coc_reconf_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Result;
} aci_l2cap_coc_reconf_confirm_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_l2cap_coc_reconf_confirm_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Channel_Index;
} aci_l2cap_coc_disconnect_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_l2cap_coc_disconnect_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Channel_Index;
  uint16_t Credits;
} aci_l2cap_coc_flow_control_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_l2cap_coc_flow_control_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Channel_Index;
  uint16_t Length;
  uint8_t Data[BLE_CMD_MAX_PARAM_LEN - 3];
} aci_l2cap_coc_tx_data_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_l2cap_coc_tx_data_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Mode;
  uint32_t Options;
} aci_reset_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_reset_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint32_t Version[2];
  uint32_t Options;
  uint32_t Debug_Info[3];
} aci_get_information_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Offset;
  uint8_t Length;
  uint8_t Value[BLE_CMD_MAX_PARAM_LEN - 2];
} aci_write_config_data_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
} aci_write_config_data_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Offset;
} aci_read_config_data_cp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint8_t Data_Length;
  uint8_t Data[(BLE_EVT_MAX_PARAM_LEN - 3) - 2];
} aci_read_config_data_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint16_t Connection_Handle;
  uint8_t Reason;
} hci_disconnection_complete_event_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint16_t Connection_Handle;
  uint8_t Encryption_Enabled;
} hci_encryption_change_event_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint16_t Connection_Handle;
  uint8_t Version;
  uint16_t Manufacturer_Name;
  uint16_t Subversion;
} hci_read_remote_version_information_complete_event_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Hardware_Code;
} hci_hardware_error_event_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Number_of_Handles;
  Handle_Packets_Pair_Entry_t Handle_Packets_Pair_Entry[(BLE_EVT_MAX_PARAM_LEN - 1)/sizeof(Handle_Packets_Pair_Entry_t)];
} hci_number_of_completed_packets_event_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint16_t Connection_Handle;
} hci_encryption_key_refresh_complete_event_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint16_t Connection_Handle;
  uint8_t Role;
  uint8_t Peer_Address_Type;
  uint8_t Peer_Address[6];
  uint16_t Conn_Interval;
  uint16_t Conn_Latency;
  uint16_t Supervision_Timeout;
  uint8_t Central_Clock_Accuracy;
} hci_le_connection_complete_event_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Num_Reports;
  Advertising_Report_t Advertising_Report[((BLE_EVT_MAX_PARAM_LEN - 1) - 1)/sizeof(Advertising_Report_t)];
} hci_le_advertising_report_event_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint16_t Connection_Handle;
  uint16_t Conn_Interval;
  uint16_t Conn_Latency;
  uint16_t Supervision_Timeout;
} hci_le_connection_update_complete_event_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint16_t Connection_Handle;
  uint8_t LE_Features[8];
} hci_le_read_remote_features_page_0_complete_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Random_Number[8];
  uint16_t Encrypted_Diversifier;
} hci_le_long_term_key_request_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t MaxTxOctets;
  uint16_t MaxTxTime;
  uint16_t MaxRxOctets;
  uint16_t MaxRxTime;
} hci_le_data_length_change_event_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint8_t Local_P256_Public_Key[64];
} hci_le_read_local_p256_public_key_complete_event_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint8_t DHKey[32];
} hci_le_generate_dhkey_complete_event_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint16_t Connection_Handle;
  uint8_t Role;
  uint8_t Peer_Address_Type;
  uint8_t Peer_Address[6];
  uint8_t Local_Resolvable_Private_Address[6];
  uint8_t Peer_Resolvable_Private_Address[6];
  uint16_t Conn_Interval;
  uint16_t Conn_Latency;
  uint16_t Supervision_Timeout;
  uint8_t Central_Clock_Accuracy;
} hci_le_enhanced_connection_complete_event_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Num_Reports;
  Direct_Advertising_Report_t Direct_Advertising_Report[((BLE_EVT_MAX_PARAM_LEN - 1) - 1)/sizeof(Direct_Advertising_Report_t)];
} hci_le_directed_advertising_report_event_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint16_t Connection_Handle;
  uint8_t TX_PHY;
  uint8_t RX_PHY;
} hci_le_phy_update_complete_event_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Num_Reports;
  uint16_t Event_Type;
  uint8_t Address_Type;
  uint8_t Address[6];
  uint8_t Primary_PHY;
  uint8_t Secondary_PHY;
  uint8_t Advertising_SID;
  uint8_t TX_Power;
  uint8_t RSSI;
  uint16_t Periodic_Adv_Interval;
  uint8_t Direct_Address_Type;
  uint8_t Direct_Address[6];
  uint8_t Data_Length;
  uint8_t Data[(BLE_EVT_MAX_PARAM_LEN - 1) - 25];
} hci_le_extended_advertising_report_event_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Status;
  uint8_t Advertising_Handle;
  uint16_t Connection_Handle;
  uint8_t Num_Completed_Ext_Adv_Events;
} hci_le_advertising_set_terminated_event_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Advertising_Handle;
  uint8_t Scanner_Address_Type;
  uint8_t Scanner_Address[6];
} hci_le_scan_request_received_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Channel_Selection_Algorithm;
} hci_le_channel_selection_algorithm_event_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Warning_Type;
  uint8_t Data_Length;
  uint8_t Data[(BLE_EVT_MAX_PARAM_LEN - 2) - 2];
} aci_warning_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Status;
  uint8_t Reason;
} aci_gap_pairing_complete_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
} aci_gap_pass_key_req_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
} aci_gap_authorization_req_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
} aci_gap_bond_lost_event_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Procedure_Code;
  uint8_t Status;
  uint8_t Data_Length;
  uint8_t Data[(BLE_EVT_MAX_PARAM_LEN - 2) - 3];
} aci_gap_proc_complete_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
} aci_gap_addr_not_resolved_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint32_t Numeric_Value;
} aci_gap_numeric_comparison_value_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Notification_Type;
} aci_gap_keypress_notification_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Bonded;
  uint8_t Auth_Req;
} aci_gap_pairing_request_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Result;
} aci_l2cap_connection_update_resp_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Data_Length;
  uint8_t Data[(BLE_EVT_MAX_PARAM_LEN - 2) - 3];
} aci_l2cap_proc_timeout_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Identifier;
  uint16_t L2CAP_Length;
  uint16_t Interval_Min;
  uint16_t Interval_Max;
  uint16_t Latency;
  uint16_t Timeout_Multiplier;
} aci_l2cap_connection_update_req_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Identifier;
  uint16_t Reason;
  uint8_t Data_Length;
  uint8_t Data[(BLE_EVT_MAX_PARAM_LEN - 2) - 6];
} aci_l2cap_command_reject_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t SPSM;
  uint16_t MTU;
  uint16_t MPS;
  uint16_t Initial_Credits;
  uint8_t Channel_Number;
} aci_l2cap_coc_connect_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t MTU;
  uint16_t MPS;
  uint16_t Initial_Credits;
  uint16_t Result;
  uint8_t Channel_Number;
  uint8_t Channel_Index_List[(BLE_EVT_MAX_PARAM_LEN - 2) - 11];
} aci_l2cap_coc_connect_confirm_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t MTU;
  uint16_t MPS;
  uint8_t Channel_Number;
  uint8_t Channel_Index_List[(BLE_EVT_MAX_PARAM_LEN - 2) - 7];
} aci_l2cap_coc_reconf_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Result;
} aci_l2cap_coc_reconf_confirm_event_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Channel_Index;
} aci_l2cap_coc_disconnect_event_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Channel_Index;
  uint16_t Credits;
} aci_l2cap_coc_flow_control_event_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Channel_Index;
  uint16_t Length;
  uint8_t Data[(BLE_EVT_MAX_PARAM_LEN - 2) - 3];
} aci_l2cap_coc_rx_data_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Attr_Handle;
  uint16_t Offset;
  uint16_t Attr_Data_Length;
  uint8_t Attr_Data[(BLE_EVT_MAX_PARAM_LEN - 2) - 8];
} aci_gatt_attribute_modified_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
} aci_gatt_proc_timeout_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Server_RX_MTU;
} aci_att_exchange_mtu_resp_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Format;
  uint8_t Event_Data_Length;
  uint8_t Handle_UUID_Pair[(BLE_EVT_MAX_PARAM_LEN - 2) - 4];
} aci_att_find_info_resp_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Num_of_Handle_Pair;
  Attribute_Group_Handle_Pair_t Attribute_Group_Handle_Pair[((BLE_EVT_MAX_PARAM_LEN - 2) - 3)/sizeof(Attribute_Group_Handle_Pair_t)];
} aci_att_find_by_type_value_resp_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Handle_Value_Pair_Length;
  uint8_t Data_Length;
  uint8_t Handle_Value_Pair_Data[(BLE_EVT_MAX_PARAM_LEN - 2) - 4];
} aci_att_read_by_type_resp_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Event_Data_Length;
  uint8_t Attribute_Value[(BLE_EVT_MAX_PARAM_LEN - 2) - 3];
} aci_att_read_resp_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Event_Data_Length;
  uint8_t Attribute_Value[(BLE_EVT_MAX_PARAM_LEN - 2) - 3];
} aci_att_read_blob_resp_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Event_Data_Length;
  uint8_t Set_Of_Values[(BLE_EVT_MAX_PARAM_LEN - 2) - 3];
} aci_att_read_multiple_resp_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Attribute_Data_Length;
  uint8_t Data_Length;
  uint8_t Attribute_Data_List[(BLE_EVT_MAX_PARAM_LEN - 2) - 4];
} aci_att_read_by_group_type_resp_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Attribute_Handle;
  uint16_t Offset;
  uint8_t Part_Attribute_Value_Length;
  uint8_t Part_Attribute_Value[(BLE_EVT_MAX_PARAM_LEN - 2) - 7];
} aci_att_prepare_write_resp_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
} aci_att_exec_write_resp_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Attribute_Handle;
  uint8_t Attribute_Value_Length;
  uint8_t Attribute_Value[(BLE_EVT_MAX_PARAM_LEN - 2) - 5];
} aci_gatt_indication_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Attribute_Handle;
  uint8_t Attribute_Value_Length;
  uint8_t Attribute_Value[(BLE_EVT_MAX_PARAM_LEN - 2) - 5];
} aci_gatt_notification_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Error_Code;
} aci_gatt_proc_complete_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Req_Opcode;
  uint16_t Attribute_Handle;
  uint8_t Error_Code;
} aci_gatt_error_resp_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Attribute_Handle;
  uint8_t Attribute_Value_Length;
  uint8_t Attribute_Value[(BLE_EVT_MAX_PARAM_LEN - 2) - 5];
} aci_gatt_disc_read_char_by_uuid_resp_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Attribute_Handle;
  uint8_t Data_Length;
  uint8_t Data[(BLE_EVT_MAX_PARAM_LEN - 2) - 5];
} aci_gatt_write_permit_req_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Attribute_Handle;
  uint16_t Offset;
} aci_gatt_read_permit_req_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Number_of_Handles;
  Handle_Item_t Handle_Item[((BLE_EVT_MAX_PARAM_LEN - 2) - 3)/sizeof(Handle_Item_t)];
} aci_gatt_read_multi_permit_req_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Available_Buffers;
} aci_gatt_tx_pool_available_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
} aci_gatt_server_confirmation_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Attribute_Handle;
  uint16_t Offset;
  uint8_t Data_Length;
  uint8_t Data[(BLE_EVT_MAX_PARAM_LEN - 2) - 7];
} aci_gatt_prepare_write_permit_req_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint8_t Channel_Index;
  uint8_t EAB_State;
  uint16_t MTU;
} aci_gatt_eatt_bearer_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Offset;
  uint16_t Data_Length;
  uint8_t Data[(BLE_EVT_MAX_PARAM_LEN - 2) - 6];
} aci_gatt_mult_notification_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Attr_Handle;
} aci_gatt_notification_complete_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Offset;
  uint16_t Event_Data_Length;
  uint8_t Attribute_Value[(BLE_EVT_MAX_PARAM_LEN - 2) - 6];
} aci_gatt_read_ext_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Attribute_Handle;
  uint16_t Offset;
  uint16_t Attribute_Value_Length;
  uint8_t Attribute_Value[(BLE_EVT_MAX_PARAM_LEN - 2) - 8];
} aci_gatt_indication_ext_event_rp0;

typedef __PACKED_STRUCT
{
  uint16_t Connection_Handle;
  uint16_t Attribute_Handle;
  uint16_t Offset;
  uint16_t Attribute_Value_Length;
  uint8_t Attribute_Value[(BLE_EVT_MAX_PARAM_LEN - 2) - 8];
} aci_gatt_notification_ext_event_rp0;

typedef __PACKED_STRUCT
{
  uint8_t Last_State;
  uint8_t Next_State;
  uint32_t Next_State_SysTime;
  uint8_t Last_State_Slot;
  uint8_t Next_State_Slot;
} aci_hal_end_of_radio_activity_event_rp0;

typedef __PACKED_STRUCT
{
  uint8_t RSSI;
  uint8_t Peer_Address_Type;
  uint8_t Peer_Address[6];
} aci_hal_scan_req_report_event_rp0;


#endif /* BLE_TYPES_H__ */
