/*****************************************************************************
 * @file    ble_hal_aci.h
 * @brief   STM32WB BLE API (HAL_ACI)
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

#ifndef BLE_HAL_ACI_H__
#define BLE_HAL_ACI_H__


#include "auto/ble_types.h"

/**
 * @brief ACI_HAL_WRITE_CONFIG_DATA
 * This command writes a value to a configure data structure. It is useful to
 * setup directly some parameters for the BLE stack.
 * Refer to Annex for details on the different parameters that can be
 * configured.
 * Note: this command is an alias of ACI_WRITE_CONFIG_DATA.
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
tBleStatus aci_hal_write_config_data( uint8_t Offset,
                                      uint8_t Length,
                                      const uint8_t* Value );

/**
 * @brief ACI_HAL_READ_CONFIG_DATA
 * This command requests the value in the configure data structure. The number
 * of read bytes changes for different Offset.
 * Note: this command is an alias of ACI_READ_CONFIG_DATA.
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
tBleStatus aci_hal_read_config_data( uint8_t Offset,
                                     uint8_t* Data_Length,
                                     uint8_t* Data );

/**
 * @brief ACI_HAL_SET_TX_POWER_LEVEL
 * This command sets the TX power level of the device. By controlling the PA
 * level, that determines the output power level (dBm) at the IC pin.
 * When the system starts up or reboots, the default TX power level is used,
 * which is the maximum value. Once this command is given, the output power
 * changes instantly, regardless if there is BLE communication going on or not.
 * For example, for debugging purpose, the device can be set to advertise all
 * the time. By using this command, one can then observe the evolution of the
 * TX signal strength.
 * The system keeps the last received TX power level from the command, i.e. the
 * 2nd command overwrites the previous TX power level. The new TX power level
 * remains until another ACI_HAL_SET_TX_POWER_LEVEL command, or the system
 * reboots. However, note that the advertising extensions commands allow, per
 * advertising set, to override the value of TX power determined by
 * ACI_HAL_SET_TX_POWER_LEVEL command (e.g. see ACI_GAP_ADV_SET_CONFIGURATION).
 * Refer to Annex for the dBm corresponding values of PA_Level parameter.
 *
 * @param En_High_Power Enable High Power mode - Deprecated and ignored
 *        Values:
 *        - 0x00: Standard Power
 *        - 0x01: High Power
 * @param PA_Level Power amplifier output level.
 *        Values:
 *        - 0x00 ... 0x23
 * @return Value indicating success or error code.
 */
tBleStatus aci_hal_set_tx_power_level( uint8_t En_High_Power,
                                       uint8_t PA_Level );

/**
 * @brief ACI_HAL_LE_TX_TEST_PACKET_NUMBER
 * This command returns the number of packets sent in Direct Test Mode.
 * When the Direct TX test is started, a 16-bit counter is used to count how
 * many packets have been transmitted.
 * This command can be used to check how many packets have been sent during the
 * Direct TX test.
 * The counter starts from 0 and counts upwards. The counter can wrap and start
 * from 0 again. The counter is not cleared until the next Direct TX test
 * starts.
 *
 * @param[out] Number_Of_Packets Number of packets sent during the last Direct
 *        TX test.
 * @return Value indicating success or error code.
 */
tBleStatus aci_hal_le_tx_test_packet_number( uint32_t* Number_Of_Packets );

/**
 * @brief ACI_HAL_TONE_START
 * This command starts a carrier frequency, i.e. a tone, on a specific channel.
 * The frequency sine wave at the specific channel may be used for debugging
 * purpose only. The channel ID is a parameter from 0x00 to 0x27 for the 40 BLE
 * channels, e.g. 0x00 for 2.402 GHz, 0x01 for 2.404 GHz etc.
 * This command should not be used when normal BLE activities are ongoing.
 * The tone should be stopped by ACI_HAL_TONE_STOP command.
 *
 * @param RF_Channel BLE Channel ID, from 0x00 to 0x27 meaning (2.402 +
 *        0.002*0xXX) GHz
 *        Device will continuously emit 0s, that means that the tone will be at
 *        the channel center frequency minus the maximum frequency deviation
 *        (250 kHz).
 *        Values:
 *        - 0x00 ... 0x27
 * @param Freq_offset Frequency Offset for tone channel
 *        Values:
 *        - 0x00 ... 0xFF
 * @return Value indicating success or error code.
 */
tBleStatus aci_hal_tone_start( uint8_t RF_Channel,
                               uint8_t Freq_offset );

/**
 * @brief ACI_HAL_TONE_STOP
 * This command is used to stop the previously started ACI_HAL_TONE_START
 * command.
 *
 * @return Value indicating success or error code.
 */
tBleStatus aci_hal_tone_stop( void );

/**
 * @brief ACI_HAL_GET_LINK_STATUS
 * This command returns the status of the 8 BLE links managed by the device.
 *
 * @param[out] Link_Status Array of link status (8 links). Each link status is
 *        1 byte.
 *        Values:
 *        - 0x00: Idle
 *        - 0x01: Advertising
 *        - 0x02: Connected in Peripheral role
 *        - 0x03: Scanning
 *        - 0x04: Reserved
 *        - 0x05: Connected in Central role
 *        - 0x06: TX test mode
 *        - 0x07: RX test mode
 *        - 0x81: Advertising with Additional Beacon
 * @param[out] Link_Connection_Handle Array of connection handles (2 bytes) for
 *        8 links. Valid only if the link status is "connected" (0x02 or 0x05)
 * @return Value indicating success or error code.
 */
tBleStatus aci_hal_get_link_status( uint8_t* Link_Status,
                                    uint16_t* Link_Connection_Handle );

/**
 * @brief ACI_HAL_SET_RADIO_ACTIVITY_MASK
 * This command set the bitmask associated to
 * ACI_HAL_END_OF_RADIO_ACTIVITY_EVENT.
 * Only the radio activities enabled in the mask will be reported to
 * application by ACI_HAL_END_OF_RADIO_ACTIVITY_EVENT
 *
 * @param Radio_Activity_Mask Bitmask of radio events
 *        Flags:
 *        - 0x0001: Idle
 *        - 0x0002: Advertising
 *        - 0x0004: Peripheral connection
 *        - 0x0008: Scanning
 *        - 0x0020: Central connection
 *        - 0x0040: TX test mode
 *        - 0x0080: RX test mode
 * @return Value indicating success or error code.
 */
tBleStatus aci_hal_set_radio_activity_mask( uint16_t Radio_Activity_Mask );

/**
 * @brief ACI_HAL_GET_ANCHOR_PERIOD
 * This command returns information about the Anchor Period to help application
 * in selecting slot timings when operating in multi-link scenarios.
 *
 * @param[out] Anchor_Period Current anchor period.
 *        T = N * 0.625 ms.
 * @param[out] Max_Free_Slot Maximum available time that can be allocated for a
 *        new slot.
 *        T = N * 0.625 ms.
 * @return Value indicating success or error code.
 */
tBleStatus aci_hal_get_anchor_period( uint32_t* Anchor_Period,
                                      uint32_t* Max_Free_Slot );

/**
 * @brief ACI_HAL_SET_EVENT_MASK
 * This command is used to enable/disable the generation of HAL events. If the
 * bit in the Event_Mask is set to a one, then the event associated with that
 * bit will be enabled.
 *
 * @param Event_Mask ACI HAL event mask. Default: 0x00000000.
 *        Flags:
 *        - 0x00000000: No events specified (Default)
 *        - 0x00000001: ACI_HAL_SCAN_REQ_REPORT_EVENT
 * @return Value indicating success or error code.
 */
tBleStatus aci_hal_set_event_mask( uint32_t Event_Mask );

/**
 * @brief ACI_HAL_SET_PERIPHERAL_LATENCY
 * This command is used to disable/enable the Peripheral latency feature during
 * a connection. Note that, by default, the Peripheral latency is enabled at
 * connection time.
 *
 * @param Enable Enable/disable Peripheral latency.
 *        Values:
 *        - 0x00: Peripheral latency is disabled
 *        - 0x01: Peripheral latency is enabled
 * @return Value indicating success or error code.
 */
tBleStatus aci_hal_set_peripheral_latency( uint8_t Enable );

/**
 * @brief ACI_HAL_READ_RSSI
 * This command returns the value of the RSSI.
 *
 * @param[out] RSSI RSSI (signed integer).
 *        Units: dBm.
 *        Values:
 *        - 127: RSSI not available
 *        - -127 ... 20
 * @return Value indicating success or error code.
 */
tBleStatus aci_hal_read_rssi( uint8_t* RSSI );

/**
 * @brief ACI_HAL_EAD_ENCRYPT_DECRYPT
 * This command encrypts or decrypts data following the Encrypted Advertising
 * Data scheme.
 * When encryption mode is selected, In_Data shall only contain the Payload
 * field to encrypt. The command adds the Randomizer and MIC fields in the
 * result. The result data length (Out_Data_Length) is equal to the input
 * length plus 9.
 * When decryption mode is selected, In_Data shall contain the full Encrypted
 * Data (Randomizer + Payload + MIC). The result data length (Out_Data_Length)
 * is equal to the input length minus 9.
 * If the decryption fails, the returned status is BLE_STATUS_FAILED, otherwise
 * it is BLE_STATUS_SUCCESS.
 * Note: the In_Data_Length value must not exceed (BLE_CMD_MAX_PARAM_LEN - 27)
 * i.e. 228 for BLE_CMD_MAX_PARAM_LEN default value.
 *
 * @param Mode EAD operation mode: encryption or decryption.
 *        Values:
 *        - 0x00: Encryption
 *        - 0x01: Decryption
 * @param Key Session key used for EAD operation (in Little Endian format).
 * @param IV Initialization vector used for EAD operation (in Little Endian
 *        format).
 * @param In_Data_Length Length of input data
 * @param In_Data Input data
 * @param[out] Out_Data_Length Length of result data
 * @param[out] Out_Data Result data
 * @return Value indicating success or error code.
 */
tBleStatus aci_hal_ead_encrypt_decrypt( uint8_t Mode,
                                        const uint8_t* Key,
                                        const uint8_t* IV,
                                        uint16_t In_Data_Length,
                                        const uint8_t* In_Data,
                                        uint16_t* Out_Data_Length,
                                        uint8_t* Out_Data );

/**
 * @brief ACI_HAL_READ_RADIO_REG
 * This command Reads Register value from the RF module.
 *
 * @param Register_Address Address of the register to be read
 * @param[out] reg_val Register value
 * @return Value indicating success or error code.
 */
tBleStatus aci_hal_read_radio_reg( uint8_t Register_Address,
                                   uint8_t* reg_val );

/**
 * @brief ACI_HAL_WRITE_RADIO_REG
 * This command writes Register value to the RF module.
 *
 * @param Register_Address Address of the register to be written
 * @param Register_Value Value to be written
 * @return Value indicating success or error code.
 */
tBleStatus aci_hal_write_radio_reg( uint8_t Register_Address,
                                    uint8_t Register_Value );

/**
 * @brief ACI_HAL_READ_RAW_RSSI
 * This command returns the raw value of the RSSI.
 *
 * @param[out] Value RAW RSSI value
 * @return Value indicating success or error code.
 */
tBleStatus aci_hal_read_raw_rssi( uint8_t* Value );

/**
 * @brief ACI_HAL_RX_START
 * This command does set up the RF to listen to a specific RF channel.
 *
 * @param RF_Channel BLE Channel ID, from 0x00 to 0x27 meaning (2.402 +
 *        0.002*0xXX) GHz
 *        Device will continuously emit 0s, that means that the tone will be at
 *        the channel center frequency minus the maximum frequency deviation
 *        (250 kHz).
 *        Values:
 *        - 0x00 ... 0x27
 * @return Value indicating success or error code.
 */
tBleStatus aci_hal_rx_start( uint8_t RF_Channel );

/**
 * @brief ACI_HAL_RX_STOP
 * This command stops a previous ACI_HAL_RX_START command.
 *
 * @return Value indicating success or error code.
 */
tBleStatus aci_hal_rx_stop( void );


#endif /* BLE_HAL_ACI_H__ */
