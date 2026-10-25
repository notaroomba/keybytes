/*****************************************************************************
 * @file    ble_gen_aci.c
 * @brief   STM32WB BLE API (gen_aci)
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

#include "auto/ble_gen_aci.h"

tBleStatus aci_reset( uint8_t Mode,
                      uint32_t Options )
{
  struct hci_request rq;
  uint8_t cmd_buffer[BLE_CMD_MAX_PARAM_LEN];
  aci_reset_cp0 *cp0 = (aci_reset_cp0*)(cmd_buffer);
  tBleStatus status = 0;
  int index_input = 0;
  cp0->Mode = Mode;
  index_input += 1;
  cp0->Options = Options;
  index_input += 4;
  Osal_MemSet( &rq, 0, sizeof(rq) );
  rq.ogf = 0x3f;
  rq.ocf = 0x300;
  rq.cparam = cmd_buffer;
  rq.clen = index_input;
  rq.rparam = &status;
  rq.rlen = 1;
  if ( hci_send_req(&rq, FALSE) < 0 )
    return BLE_STATUS_TIMEOUT;
  return status;
}

tBleStatus aci_get_information( uint32_t* Version,
                                uint32_t* Options,
                                uint32_t* Debug_Info )
{
  struct hci_request rq;
  aci_get_information_rp0 resp;
  Osal_MemSet( &resp, 0, sizeof(resp) );
  Osal_MemSet( &rq, 0, sizeof(rq) );
  rq.ogf = 0x3f;
  rq.ocf = 0x301;
  rq.rparam = &resp;
  rq.rlen = sizeof(resp);
  if ( hci_send_req(&rq, FALSE) < 0 )
    return BLE_STATUS_TIMEOUT;
  if ( resp.Status )
    return resp.Status;
  Osal_MemCpy( (void*)Version, (const void*)resp.Version, 8 );
  *Options = resp.Options;
  Osal_MemCpy( (void*)Debug_Info, (const void*)resp.Debug_Info, 12 );
  return BLE_STATUS_SUCCESS;
}

tBleStatus aci_write_config_data( uint8_t Offset,
                                  uint8_t Length,
                                  const uint8_t* Value )
{
  struct hci_request rq;
  uint8_t cmd_buffer[BLE_CMD_MAX_PARAM_LEN];
  aci_write_config_data_cp0 *cp0 = (aci_write_config_data_cp0*)(cmd_buffer);
  tBleStatus status = 0;
  int index_input = 0;
  cp0->Offset = Offset;
  index_input += 1;
  cp0->Length = Length;
  index_input += 1;
  Osal_MemCpy( (void*)&cp0->Value, (const void*)Value, Length );
  index_input += Length;
  Osal_MemSet( &rq, 0, sizeof(rq) );
  rq.ogf = 0x3f;
  rq.ocf = 0x302;
  rq.cparam = cmd_buffer;
  rq.clen = index_input;
  rq.rparam = &status;
  rq.rlen = 1;
  if ( hci_send_req(&rq, FALSE) < 0 )
    return BLE_STATUS_TIMEOUT;
  return status;
}

tBleStatus aci_read_config_data( uint8_t Offset,
                                 uint8_t* Data_Length,
                                 uint8_t* Data )
{
  struct hci_request rq;
  uint8_t cmd_buffer[BLE_CMD_MAX_PARAM_LEN];
  aci_read_config_data_cp0 *cp0 = (aci_read_config_data_cp0*)(cmd_buffer);
  aci_read_config_data_rp0 resp;
  Osal_MemSet( &resp, 0, sizeof(resp) );
  int index_input = 0;
  cp0->Offset = Offset;
  index_input += 1;
  Osal_MemSet( &rq, 0, sizeof(rq) );
  rq.ogf = 0x3f;
  rq.ocf = 0x303;
  rq.cparam = cmd_buffer;
  rq.clen = index_input;
  rq.rparam = &resp;
  rq.rlen = sizeof(resp);
  if ( hci_send_req(&rq, FALSE) < 0 )
    return BLE_STATUS_TIMEOUT;
  if ( resp.Status )
    return resp.Status;
  *Data_Length = resp.Data_Length;
  Osal_MemCpy( (void*)Data, (const void*)resp.Data, *Data_Length);
  return BLE_STATUS_SUCCESS;
}

