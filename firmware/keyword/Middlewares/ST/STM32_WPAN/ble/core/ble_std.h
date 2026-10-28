/******************************************************************************
 * @file    ble_std.h
 *
 * @brief   BLE standard definitions
 ******************************************************************************
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

#ifndef BLE_STD_H__
#define BLE_STD_H__


/* HCI packet type */
#define HCI_COMMAND_PKT_TYPE             0x01U
#define HCI_ACLDATA_PKT_TYPE             0x02U
#define HCI_EVENT_PKT_TYPE               0x04U
#define HCI_ISODATA_PKT_TYPE             0x05U

/* HCI packet header size */
#define HCI_COMMAND_HDR_SIZE             4
#define HCI_ACLDATA_HDR_SIZE             5
#define HCI_EVENT_HDR_SIZE               3
#define HCI_ISODATA_HDR_SIZE             5

/* HCI parameters length */
#define HCI_COMMAND_MAX_PARAM_LEN        255
#define HCI_ACLDATA_MAX_DATA_LEN         251  /* LE_ACL_Data_Packet_Length */
#define HCI_EVENT_MAX_PARAM_LEN          255
#define HCI_ISODATA_MAX_DATA_LEN         300  /* ISO_Data_Packet_Length */

/* HCI packet maximum size */
#define HCI_COMMAND_PKT_MAX_SIZE \
          (HCI_COMMAND_HDR_SIZE + HCI_COMMAND_MAX_PARAM_LEN)
#define HCI_ACLDATA_PKT_MAX_SIZE \
          (HCI_ACLDATA_HDR_SIZE + HCI_ACLDATA_MAX_DATA_LEN)
#define HCI_EVENT_PKT_MAX_SIZE \
          (HCI_EVENT_HDR_SIZE   + HCI_EVENT_MAX_PARAM_LEN)
#define HCI_ISODATA_PKT_MAX_SIZE \
          (HCI_ISODATA_HDR_SIZE + HCI_ISODATA_MAX_DATA_LEN)

/* HCI event code */
#define HCI_DISCONNECTION_COMPLETE_EVT_CODE                            0x05U
#define HCI_ENCRYPTION_CHANGE_EVT_CODE                                 0x08U
#define HCI_READ_REMOTE_VERSION_INFORMATION_COMPLETE_EVT_CODE          0x0CU
#define HCI_COMMAND_COMPLETE_EVT_CODE                                  0x0EU
#define HCI_COMMAND_STATUS_EVT_CODE                                    0x0FU
#define HCI_HARDWARE_ERROR_EVT_CODE                                    0x10U
#define HCI_NUMBER_OF_COMPLETED_PACKETS_EVT_CODE                       0x13U
#define HCI_DATA_BUFFER_OVERFLOW_EVT_CODE                              0x1AU
#define HCI_ENCRYPTION_KEY_REFRESH_COMPLETE_EVT_CODE                   0x30U
#define HCI_LE_META_EVT_CODE                                           0x3EU
#define HCI_AUTHENTICATED_PAYLOAD_TIMEOUT_EXPIRED_EVT_CODE             0x57U
#define HCI_VENDOR_SPECIFIC_EVT_CODE                                   0xFFU

/* HCI LE subevent code */
#define HCI_LE_CONNECTION_COMPLETE_SUBEVT_CODE                            0x01U
#define HCI_LE_ADVERTISING_REPORT_SUBEVT_CODE                             0x02U
#define HCI_LE_CONNECTION_UPDATE_COMPLETE_SUBEVT_CODE                     0x03U
#define HCI_LE_READ_REMOTE_FEATURES_PAGE_0_COMPLETE_SUBEVT_CODE           0x04U
#define HCI_LE_LONG_TERM_KEY_REQUEST_SUBEVT_CODE                          0x05U
#define HCI_LE_REMOTE_CONNECTION_PARAMETER_REQUEST_SUBEVT_CODE            0x06U
#define HCI_LE_DATA_LENGTH_CHANGE_SUBEVT_CODE                             0x07U
#define HCI_LE_READ_LOCAL_P256_PUBLIC_KEY_COMPLETE_SUBEVT_CODE            0x08U
#define HCI_LE_GENERATE_DHKEY_COMPLETE_SUBEVT_CODE                        0x09U
#define HCI_LE_ENHANCED_CONNECTION_COMPLETE_SUBEVT_CODE                   0x0AU
#define HCI_LE_DIRECTED_ADVERTISING_REPORT_SUBEVT_CODE                    0x0BU
#define HCI_LE_PHY_UPDATE_COMPLETE_SUBEVT_CODE                            0x0CU
#define HCI_LE_EXTENDED_ADVERTISING_REPORT_SUBEVT_CODE                    0x0DU
#define HCI_LE_PERIODIC_ADVERTISING_SYNC_ESTABLISHED_SUBEVT_CODE          0x0EU
#define HCI_LE_PERIODIC_ADVERTISING_REPORT_SUBEVT_CODE                    0x0FU
#define HCI_LE_PERIODIC_ADVERTISING_SYNC_LOST_SUBEVT_CODE                 0x10U
#define HCI_LE_SCAN_TIMEOUT_SUBEVT_CODE                                   0x11U
#define HCI_LE_ADVERTISING_SET_TERMINATED_SUBEVT_CODE                     0x12U
#define HCI_LE_SCAN_REQUEST_RECEIVED_SUBEVT_CODE                          0x13U
#define HCI_LE_CHANNEL_SELECTION_ALGORITHM_SUBEVT_CODE                    0x14U
#define HCI_LE_CONNECTIONLESS_IQ_REPORT_SUBEVT_CODE                       0x15U
#define HCI_LE_CONNECTION_IQ_REPORT_SUBEVT_CODE                           0x16U
#define HCI_LE_CTE_REQUEST_FAILED_SUBEVT_CODE                             0x17U
#define HCI_LE_PERIODIC_ADVERTISING_SYNC_TRANSFER_RECEIVED_SUBEVT_CODE    0x18U
#define HCI_LE_CIS_ESTABLISHED_SUBEVT_CODE                                0x19U
#define HCI_LE_CIS_REQUEST_SUBEVT_CODE                                    0x1AU
#define HCI_LE_CREATE_BIG_COMPLETE_SUBEVT_CODE                            0x1BU
#define HCI_LE_TERMINATE_BIG_COMPLETE_SUBEVT_CODE                         0x1CU
#define HCI_LE_BIG_SYNC_ESTABLISHED_SUBEVT_CODE                           0x1DU
#define HCI_LE_BIG_SYNC_LOST_SUBEVT_CODE                                  0x1EU
#define HCI_LE_REQUEST_PEER_SCA_COMPLETE_SUBEVT_CODE                      0x1FU
#define HCI_LE_PATH_LOSS_THRESHOLD_SUBEVT_CODE                            0x20U
#define HCI_LE_TRANSMIT_POWER_REPORTING_SUBEVT_CODE                       0x21U
#define HCI_LE_BIGINFO_ADVERTISING_REPORT_SUBEVT_CODE                     0x22U
#define HCI_LE_SUBRATE_CHANGE_SUBEVT_CODE                                 0x23U
#define HCI_LE_PERIODIC_ADVERTISING_SYNC_ESTABLISHED_V2_SUBEVT_CODE       0x24U
#define HCI_LE_PERIODIC_ADVERTISING_REPORT_V2_SUBEVT_CODE                 0x25U
#define HCI_LE_PERIODIC_ADVERTISING_SYNC_TRANSFER_RECEIVED_V2_SUBEVT_CODE 0x26U
#define HCI_LE_PERIODIC_ADVERTISING_SUBEVENT_DATA_REQUEST_SUBEVT_CODE     0x27U
#define HCI_LE_PERIODIC_ADVERTISING_RESPONSE_REPORT_SUBEVT_CODE           0x28U
#define HCI_LE_ENHANCED_CONNECTION_COMPLETE_V2_SUBEVT_CODE                0x29U
#define HCI_LE_CIS_ESTABLISHED_V2_SUBEVT_CODE                             0x2AU
#define HCI_LE_READ_ALL_REMOTE_FEATURES_COMPLETE_SUBEVT_CODE              0x2BU
#define HCI_LE_CS_READ_REMOTE_SUPPORTED_CAPABILITIES_COMPLETE_SUBEVT_CODE 0x2CU
#define HCI_LE_CS_READ_REMOTE_FAE_TABLE_COMPLETE_SUBEVT_CODE              0x2DU
#define HCI_LE_CS_SECURITY_ENABLE_COMPLETE_SUBEVT_CODE                    0x2EU
#define HCI_LE_CS_CONFIG_COMPLETE_SUBEVT_CODE                             0x2FU
#define HCI_LE_CS_PROCEDURE_ENABLE_COMPLETE_SUBEVT_CODE                   0x30U
#define HCI_LE_CS_SUBEVENT_RESULT_SUBEVT_CODE                             0x31U
#define HCI_LE_CS_SUBEVENT_RESULT_CONTINUE_SUBEVT_CODE                    0x32U
#define HCI_LE_CS_TEST_END_COMPLETE_SUBEVT_CODE                           0x33U
#define HCI_LE_MONITORED_ADVERTISERS_REPORT_SUBEVT_CODE                   0x34U
#define HCI_LE_FRAME_SPACE_UPDATE_COMPLETE_SUBEVT_CODE                    0x35U

/* HCI error code */
#define HCI_SUCCESS_ERR_CODE                                           0x00U
#define HCI_UNKNOWN_HCI_COMMAND_ERR_CODE                               0x01U
#define HCI_UNKNOWN_CONNECTION_IDENTIFIER_ERR_CODE                     0x02U
#define HCI_HARDWARE_FAILURE_ERR_CODE                                  0x03U
#define HCI_AUTHENTICATION_FAILURE_ERR_CODE                            0x05U
#define HCI_PIN_OR_KEY_MISSING_ERR_CODE                                0x06U
#define HCI_MEMORY_CAPACITY_EXCEEDED_ERR_CODE                          0x07U
#define HCI_CONNECTION_TIMEOUT_ERR_CODE                                0x08U
#define HCI_CONNECTION_LIMIT_EXCEEDED_ERR_CODE                         0x09U
#define HCI_CONNECTION_ALREADY_EXISTS_ERR_CODE                         0x0BU
#define HCI_COMMAND_DISALLOWED_ERR_CODE                                0x0CU
#define HCI_UNSUPPORTED_FEATURE_OR_PARAMETER_VALUE_ERR_CODE            0x11U
#define HCI_INVALID_HCI_COMMAND_PARAMETERS_ERR_CODE                    0x12U
#define HCI_REMOTE_USER_TERMINATED_CONNECTION_ERR_CODE                 0x13U
#define HCI_REMOTE_TERMINATED_CONNECTION_DUE_TO_LOW_RESOURCES_ERR_CODE 0x14U
#define HCI_REMOTE_TERMINATED_CONNECTION_DUE_TO_POWER_OFF_ERR_CODE     0x15U
#define HCI_CONNECTION_TERMINATED_BY_LOCAL_HOST_ERR_CODE               0x16U
#define HCI_UNSUPPORTED_REMOTE_FEATURE_ERR_CODE                        0x1AU
#define HCI_INVALID_LL_PARAMETERS_ERR_CODE                             0x1EU
#define HCI_UNSPECIFIED_ERROR_ERR_CODE                                 0x1FU
#define HCI_UNSUPPORTED_LL_PARAMETER_VALUE_ERR_CODE                    0x20U
#define HCI_LL_RESPONSE_TIMEOUT_ERR_CODE                               0x22U
#define HCI_LL_PROCEDURE_COLLISION_ERR_CODE                            0x23U
#define HCI_LMP_PDU_NOT_ALLOWED_ERR_CODE                               0x24U
#define HCI_INSTANT_PASSED_ERR_CODE                                    0x28U
#define HCI_DIFFERENT_TRANSACTION_COLLISION_ERR_CODE                   0x2AU
#define HCI_PARAMETER_OUT_OF_MANDATORY_RANGE_ERR_CODE                  0x30U
#define HCI_HOST_BUSY_PAIRING_ERR_CODE                                 0x38U
#define HCI_CONTROLLER_BUSY_ERR_CODE                                   0x3AU
#define HCI_UNACCEPTABLE_CONNECTION_PARAMETERS_ERR_CODE                0x3BU
#define HCI_ADVERTISING_TIMEOUT_ERR_CODE                               0x3CU
#define HCI_CONNECTION_TERMINATED_DUE_TO_MIC_FAILURE_ERR_CODE          0x3DU
#define HCI_CONNECTION_FAILED_TO_BE_ESTABLISHED_ERR_CODE               0x3EU
#define HCI_UNKNOWN_ADVERTISING_IDENTIFIER_ERR_CODE                    0x42U
#define HCI_ADVERTISING_LIMIT_REACHED_ERR_CODE                         0x43U
#define HCI_PACKET_TOO_LONG_ERR_CODE                                   0x45U

/* HCI_LE_Set_Advertising_Parameters: Advertising_Type */
#define HCI_ADV_TYPE_ADV_IND                           0x00U
#define HCI_ADV_TYPE_ADV_DIRECT_IND_HDC                0x01U
#define HCI_ADV_TYPE_ADV_SCAN_IND                      0x02U
#define HCI_ADV_TYPE_ADV_NONCONN_IND                   0x03U
#define HCI_ADV_TYPE_ADV_DIRECT_IND_LDC                0x04U

/* HCI_LE_Set_Advertising_Parameters: Advertising_Filter_Policy */
#define HCI_ADV_FILTER_NO                              0x00U
#define HCI_ADV_FILTER_ACC_LIST_USED_FOR_SCAN          0x01U
#define HCI_ADV_FILTER_ACC_LIST_USED_FOR_CONNECT       0x02U
#define HCI_ADV_FILTER_ACC_LIST_USED_FOR_ALL           0x03U

/* HCI_LE_Set_[Advertising/Scan]_Parameters: Own_Address_Type */
#define HCI_OWN_ADDR_TYPE_PUBLIC                       0x00U
#define HCI_OWN_ADDR_TYPE_RANDOM                       0x01U
#define HCI_OWN_ADDR_TYPE_RP_OR_PUBLIC                 0x02U
#define HCI_OWN_ADDR_TYPE_RP_OR_RANDOM                 0x03U

/* HCI_LE_Set_Scan_Parameters: LE_Scan_Type */
#define HCI_SCAN_TYPE_PASSIVE                          0x00U
#define HCI_SCAN_TYPE_ACTIVE                           0x01U

/* HCI_LE_Set_Scan_Parameters: Scanning_Filter_Policy */
#define HCI_SCAN_FILTER_NO                             0x00U
#define HCI_SCAN_FILTER_ACC_LIST_USED                  0x01U
#define HCI_SCAN_FILTER_NO_EXT                         0x02U
#define HCI_SCAN_FILTER_ACC_LIST_USED_EXT              0x03U

/* HCI_LE_Create_Connection: Initiator_Filter_Policy */
#define HCI_INIT_FILTER_NO                             0x00U
#define HCI_INIT_FILTER_ACC_LIST_USED                  0x01U

/* HCI_LE_Read_PHY: TX_PHY */
#define HCI_TX_PHY_LE_1M                               0x01U
#define HCI_TX_PHY_LE_2M                               0x02U
#define HCI_TX_PHY_LE_CODED                            0x03U

/* HCI_LE_Read_PHY: RX_PHY */
#define HCI_RX_PHY_LE_1M                               0x01U
#define HCI_RX_PHY_LE_2M                               0x02U
#define HCI_RX_PHY_LE_CODED                            0x03U

/* HCI_LE_Set_PHY: ALL_PHYS */
#define HCI_ALL_PHYS_TX_NO_PREF                        0x01U
#define HCI_ALL_PHYS_RX_NO_PREF                        0x02U

/* HCI_LE_Set_PHY: TX_PHYS */
#define HCI_TX_PHYS_LE_1M_PREF                         0x01U
#define HCI_TX_PHYS_LE_2M_PREF                         0x02U
#define HCI_TX_PHYS_LE_CODED_PREF                      0x04U

/* HCI_LE_Set_PHY: RX_PHYS */
#define HCI_RX_PHYS_LE_1M_PREF                         0x01U
#define HCI_RX_PHYS_LE_2M_PREF                         0x02U
#define HCI_RX_PHYS_LE_CODED_PREF                      0x04U

/* HCI_LE_Set_Extended_Advertising_Parameters: Advertising_Event_Properties */
#define HCI_ADV_EVENT_PROP_CONNECTABLE                 0x0001U
#define HCI_ADV_EVENT_PROP_SCANNABLE                   0x0002U
#define HCI_ADV_EVENT_PROP_DIRECTED                    0x0004U
#define HCI_ADV_EVENT_PROP_HDC_DIRECTED                0x0008U
#define HCI_ADV_EVENT_PROP_LEGACY                      0x0010U
#define HCI_ADV_EVENT_PROP_ANONYMOUS                   0x0020U
#define HCI_ADV_EVENT_PROP_TXPOWER_INC                 0x0040U

/* HCI_LE_Set_Extended_Advertising_Parameters: Primary_Advertising_PHY */
#define HCI_PRIMARY_ADV_PHY_LE_1M                      0x01U
#define HCI_PRIMARY_ADV_PHY_LE_CODED                   0x03U

/* HCI_LE_Set_Extended_Advertising_Data: Operation */
#define HCI_SET_ADV_DATA_OPERATION_INTERMEDIATE        0x00U
#define HCI_SET_ADV_DATA_OPERATION_FIRST               0x01U
#define HCI_SET_ADV_DATA_OPERATION_LAST                0x02U
#define HCI_SET_ADV_DATA_OPERATION_COMPLETE            0x03U
#define HCI_SET_ADV_DATA_OPERATION_UNCHANGED           0x04U

/* HCI_LE_Advertising_Report: Event_Type */
#define HCI_ADV_EVT_TYPE_ADV_IND                       0x00U
#define HCI_ADV_EVT_TYPE_ADV_DIRECT_IND                0x01U
#define HCI_ADV_EVT_TYPE_ADV_SCAN_IND                  0x02U
#define HCI_ADV_EVT_TYPE_ADV_NONCONN_IND               0x03U
#define HCI_ADV_EVT_TYPE_SCAN_RSP                      0x04U

/* HCI_LE_Set_Extended_Scan_Parameters: Scanning_PHYs */
#define HCI_SCANNING_PHYS_LE_1M                        0x01U
#define HCI_SCANNING_PHYS_LE_CODED                     0x04U

/* HCI_LE_Extended_Create_Connection: Initiating_PHYs */
#define HCI_INIT_PHYS_SCAN_CONN_LE_1M                  0x01U
#define HCI_INIT_PHYS_CONN_LE_2M                       0x02U
#define HCI_INIT_PHYS_SCAN_CONN_LE_CODED               0x04U

/* HCI_LE_Receiver_Test/HCI_LE_Transmitter_Test [v2]: PHY */
#define HCI_TEST_PHY_LE_1M                             0x01U
#define HCI_TEST_PHY_LE_2M                             0x02U

/* HCI_LE_Connection_Complete/HCI_LE_Enhanced_Connection_Complete: Role */
#define HCI_ROLE_CENTRAL                               0x00U
#define HCI_ROLE_PERIPHERAL                            0x01U

/* HCI_LE_Set_Privacy_Mode: Privacy_Mode */
#define HCI_PRIV_MODE_NETWORK                          0x00U
#define HCI_PRIV_MODE_DEVICE                           0x01U

/* Bluetooth Core Specification versions
 */
#define BLE_CORE_5_2                                   11
#define BLE_CORE_5_3                                   12
#define BLE_CORE_5_4                                   13
#define BLE_CORE_6_0                                   14
#define BLE_CORE_6_1                                   15

/* AD types for advertising data and scan response data
 */
#define AD_TYPE_FLAGS                                  0x01U
#define AD_TYPE_16_BIT_SERV_UUID                       0x02U
#define AD_TYPE_16_BIT_SERV_UUID_CMPLT_LIST            0x03U
#define AD_TYPE_32_BIT_SERV_UUID                       0x04U
#define AD_TYPE_32_BIT_SERV_UUID_CMPLT_LIST            0x05U
#define AD_TYPE_128_BIT_SERV_UUID                      0x06U
#define AD_TYPE_128_BIT_SERV_UUID_CMPLT_LIST           0x07U
#define AD_TYPE_SHORTENED_LOCAL_NAME                   0x08U
#define AD_TYPE_COMPLETE_LOCAL_NAME                    0x09U
#define AD_TYPE_TX_POWER_LEVEL                         0x0AU
#define AD_TYPE_CLASS_OF_DEVICE                        0x0DU
#define AD_TYPE_SEC_MGR_TK_VALUE                       0x10U
#define AD_TYPE_SEC_MGR_OOB_FLAGS                      0x11U
#define AD_TYPE_PERIPHERAL_CONN_INTERVAL               0x12U
#define AD_TYPE_SERV_SOLICIT_16_BIT_UUID_LIST          0x14U
#define AD_TYPE_SERV_SOLICIT_128_BIT_UUID_LIST         0x15U
#define AD_TYPE_SERVICE_DATA                           0x16U
#define AD_TYPE_APPEARANCE                             0x19U
#define AD_TYPE_ADVERTISING_INTERVAL                   0x1AU
#define AD_TYPE_LE_ROLE                                0x1CU
#define AD_TYPE_SERV_SOLICIT_32_BIT_UUID_LIST          0x1FU
#define AD_TYPE_URI                                    0x24U
#define AD_TYPE_MANUFACTURER_SPECIFIC_DATA             0xFFU

/* Flag bits for Flags AD Type
 */
#define FLAG_BIT_LE_LIMITED_DISCOVERABLE_MODE          0x01U
#define FLAG_BIT_LE_GENERAL_DISCOVERABLE_MODE          0x02U
#define FLAG_BIT_BR_EDR_NOT_SUPPORTED                  0x04U
#define FLAG_BIT_LE_BR_EDR_CONTROLLER                  0x08U
#define FLAG_BIT_LE_BR_EDR_HOST                        0x10U

/* Appearance values
 */
#define GAP_APPEARANCE_UNKNOWN                                 0x0000
#define GAP_APPEARANCE_GENERIC_PHONE                           0x0040
#define GAP_APPEARANCE_GENERIC_COMPUTER                        0x0080
#define GAP_APPEARANCE_GENERIC_WATCH                           0x00C0
#define GAP_APPEARANCE_WATCH_SPORT_WATCH                       0x00C1
#define GAP_APPEARANCE_GENERIC_CLOCK                           0x0100
#define GAP_APPEARANCE_GENERIC_DISPLAY                         0x0140
#define GAP_APPEARANCE_GENERIC_REMOTE_CONTROL                  0x0180
#define GAP_APPEARANCE_GENERIC_EYE_GLASSES                     0x01C0
#define GAP_APPEARANCE_GENERIC_TAG                             0x0200
#define GAP_APPEARANCE_GENERIC_KEYRING                         0x0240
#define GAP_APPEARANCE_GENERIC_MEDIA_PLAYER                    0x0280
#define GAP_APPEARANCE_GENERIC_BARCODE_SCANNER                 0x02C0
#define GAP_APPEARANCE_GENERIC_THERMOMETER                     0x0300
#define GAP_APPEARANCE_THERMOMETER_EAR                         0x0301
#define GAP_APPEARANCE_GENERIC_HEART_RATE_SENSOR               0x0340
#define GAP_APPEARANCE_HEART_RATE_SENSOR_HEART_RATE_BELT       0x0341
#define GAP_APPEARANCE_GENERIC_BLOOD_PRESSURE                  0x0380
#define GAP_APPEARANCE_BLOOD_PRESSURE_ARM                      0x0381
#define GAP_APPEARANCE_BLOOD_PRESSURE_WRIST                    0x0382
#define GAP_APPEARANCE_HUMAN_INTERFACE_DEVICE                  0x03C0
#define GAP_APPEARANCE_KEYBOARD                                0x03C1
#define GAP_APPEARANCE_MOUSE                                   0x03C2
#define GAP_APPEARANCE_JOYSTICK                                0x03C3
#define GAP_APPEARANCE_GAMEPAD                                 0x03C4
#define GAP_APPEARANCE_DIGITIZER_TABLET                        0x03C5
#define GAP_APPEARANCE_CARD_READER                             0x03C6
#define GAP_APPEARANCE_DIGITAL_PEN                             0x03C7
#define GAP_APPEARANCE_BARCODE_SCANNER                         0x03C8
#define GAP_APPEARANCE_GENERIC_GLUCOSE_METER                   0x0400
#define GAP_APPEARANCE_GENERIC_RUNNING_WALKING_SENSOR          0x0440
#define GAP_APPEARANCE_RUNNING_WALKING_IN_SHOE                 0x0441
#define GAP_APPEARANCE_RUNNING_WALKING_ON_SHOE                 0x0442
#define GAP_APPEARANCE_RUNNING_WALKING_ON_HIP                  0x0443
#define GAP_APPEARANCE_GENERIC_CYCLING                         0x0480
#define GAP_APPEARANCE_CYCLING_CYCLING_COMPUTER                0x0481
#define GAP_APPEARANCE_CYCLING_SPEED_SENSOR                    0x0482
#define GAP_APPEARANCE_CYCLING_CADENCE_SENSOR                  0x0483
#define GAP_APPEARANCE_CYCLING_POWER_SENSOR                    0x0484
#define GAP_APPEARANCE_CYCLING_SPEED_AND_CADENCE_SENSOR        0x0485
#define GAP_APPEARANCE_GENERIC_PULSE_OXYMETER                  0x0C40
#define GAP_APPEARANCE_FINGERTIP                               0x0C41
#define GAP_APPEARANCE_WRIST_WORN                              0x0C42
#define GAP_APPEARANCE_GENERIC_WEIGHT_SCALE                    0x0C80
#define GAP_APPEARANCE_GENERIC_OUTDOOR_SPORT_ACTIVITY          0x1440
#define GAP_APPEARANCE_LOCATION_DISPLAY_DEVICE                 0x1441
#define GAP_APPEARANCE_LOCATION_AND_NAVIGATION_DISPLAY_DEVICE  0x1442
#define GAP_APPEARANCE_LOCATION_POD                            0x1443
#define GAP_APPEARANCE_LOCATION_AND_NAVIGATION_POD             0x1444
#define GAP_APPEARANCE_GENERIC_ENVIRONMENTAL_SENSOR            0x1640

/* GATT UUIDs
 */
#define GATT_SERVICE_UUID                        0x1801U
#define PRIMARY_SERVICE_UUID                     0x2800U
#define SECONDARY_SERVICE_UUID                   0x2801U
#define INCLUDE_SERVICE_UUID                     0x2802U
#define CHARACTERISTIC_UUID                      0x2803U
#define CHAR_EXTENDED_PROP_DESC_UUID             0x2900U
#define CHAR_USER_DESC_UUID                      0x2901U
#define CHAR_CLIENT_CONFIG_DESC_UUID             0x2902U
#define CHAR_SERVER_CONFIG_DESC_UUID             0x2903U
#define CHAR_FORMAT_DESC_UUID                    0x2904U
#define CHAR_AGGR_FMT_DESC_UUID                  0x2905U
#define SERVICE_CHANGED_UUID                     0x2A05U
#define CLIENT_SUPPORTED_FEATURES_UUID           0X2B29U
#define DATABASE_HASH_UUID                       0X2B2AU
#define SERVER_SUPPORTED_FEATURES_UUID           0X2B3AU

/* GAP UUIDs
 */
#define GAP_SERVICE_UUID                         0x1800U
#define DEVICE_NAME_UUID                         0x2A00U
#define APPEARANCE_UUID                          0x2A01U
#define PERIPHERAL_PREFERRED_CONN_PARAMS_UUID    0x2A04U
#define CENTRAL_ADDRESS_RESOLUTION_UUID          0x2AA6U
#define RESOLVABLE_PRIVATE_ADDRESS_ONLY_UUID     0x2AC9U
#define ENCRYPTED_DATA_KEY_MATERIAL_UUID         0x2B88U
#define LE_GATT_SECURITY_LEVELS_UUID             0x2BF5U


#endif /* BLE_STD_H__ */
