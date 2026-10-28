#include "app_common.h"
#include "ble.h"
#include "stm32_seq.h"
#include "app_ble.h"
#include "nus.h"
#include "proto.h"

#define NUS_CHUNK 20    /* fits the default 23 byte ATT MTU */
#define TX_SIZE   1024
#define RX_SIZE   256

/* 6e40000x-b5a3-f393-e0a9-e50e24dcca9e, little endian, x at byte 12 */
#define NUS_UUID(x) {0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0, 0x93, 0xf3, 0xa3, 0xb5, (x), 0x00, 0x40, 0x6e}

static uint16_t svc_handle, rx_handle, tx_handle;
static uint8_t tx_enabled;

/* Single producer, single consumer, both in the sequencer, so plain indexes. */
static char tx_buf[TX_SIZE];
static uint16_t tx_head, tx_tail;
static uint8_t rx_buf[RX_SIZE];
static uint16_t rx_head, rx_tail;

static proto_port_t nus_port = {.write = NUS_Write};

static void Tx_Drain(void)
{
  while (tx_enabled && tx_tail != tx_head && APP_BLE_ConnHandle() != APP_BLE_NO_CONN)
  {
    uint8_t chunk[NUS_CHUNK];
    uint8_t n = 0;
    for (uint16_t t = tx_tail; n < NUS_CHUNK && t != tx_head; t = (t + 1) % TX_SIZE)
    {
      chunk[n++] = (uint8_t)tx_buf[t];
    }
    /* Out of TX buffers: ACI_GATT_TX_POOL_AVAILABLE calls us again. */
    if (aci_gatt_update_char_value(svc_handle, tx_handle, 0, n, chunk) != BLE_STATUS_SUCCESS) return;
    tx_tail = (tx_tail + n) % TX_SIZE;
  }
}

void NUS_Write(const char *data, uint16_t len)
{
  if (!tx_enabled) return;   /* nobody listening, don't replay stale events later */
  for (uint16_t i = 0; i < len; i++)
  {
    uint16_t next = (tx_head + 1) % TX_SIZE;
    if (next == tx_tail) break;   /* full: drop the rest, the host can ask again */
    tx_buf[tx_head] = data[i];
    tx_head = next;
  }
  Tx_Drain();
}

void NUS_Disconnected(void)
{
  tx_enabled = 0;
  nus_port.connected = 0;
  nus_port.len = 0;
  nus_port.overflow = 0;
  tx_head = tx_tail = 0;
  rx_head = rx_tail = 0;
}

static void Rx_Task(void)
{
  while (rx_tail != rx_head)
  {
    uint16_t end = rx_head;
    uint16_t n = (end > rx_tail ? end : RX_SIZE) - rx_tail;
    PROTO_Input(&nus_port, &rx_buf[rx_tail], n);
    rx_tail = (rx_tail + n) % RX_SIZE;
  }
}

static SVCCTL_EvtAckStatus_t NUS_Event_Handler(void *Event)
{
  hci_event_pckt *event_pckt = (hci_event_pckt *)(((hci_uart_pckt *)Event)->data);
  if (event_pckt->evt != HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE) return SVCCTL_EvtNotAck;

  evt_blecore_aci *blecore_evt = (evt_blecore_aci *)event_pckt->data;
  switch (blecore_evt->ecode)
  {
    case ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE:
    {
      aci_gatt_attribute_modified_event_rp0 *mod = (aci_gatt_attribute_modified_event_rp0 *)blecore_evt->data;
      if (mod->Attr_Handle == tx_handle + 2)   /* TX CCCD */
      {
        tx_enabled = mod->Attr_Data[0] & 1;
        nus_port.connected = tx_enabled;
        Tx_Drain();
        return SVCCTL_EvtAckFlowEnable;
      }
      if (mod->Attr_Handle == rx_handle + 1)   /* RX value */
      {
        for (uint16_t i = 0; i < mod->Attr_Data_Length; i++)
        {
          uint16_t next = (rx_head + 1) % RX_SIZE;
          if (next == rx_tail) break;
          rx_buf[rx_head] = mod->Attr_Data[i];
          rx_head = next;
        }
        UTIL_SEQ_SetTask(1 << CFG_TASK_NUS_RX_ID, CFG_SCH_PRIO_0);
        return SVCCTL_EvtAckFlowEnable;
      }
      break;
    }

    case ACI_GATT_TX_POOL_AVAILABLE_VSEVT_CODE:
      Tx_Drain();
      break;   /* not acked: other services may be waiting on it too */

    default:
      break;
  }
  return SVCCTL_EvtNotAck;
}

void NUS_Init(void)
{
  Char_UUID_t uuid;
  const uint8_t svc_uuid[16] = NUS_UUID(0x01);
  const uint8_t rx_uuid[16] = NUS_UUID(0x02);
  const uint8_t tx_uuid[16] = NUS_UUID(0x03);

  SVCCTL_RegisterSvcHandler(NUS_Event_Handler);
  PROTO_AddPort(&nus_port);
  UTIL_SEQ_RegTask(1 << CFG_TASK_NUS_RX_ID, UTIL_SEQ_RFU, Rx_Task);

  memcpy(uuid.Char_UUID_128, svc_uuid, 16);
  /* service + RX (decl, value) + TX (decl, value, CCCD) */
  aci_gatt_add_service(UUID_TYPE_128, (Service_UUID_t *)&uuid, PRIMARY_SERVICE, 6, &svc_handle);

  /* Writes need an encrypted (bonded) link so a stranger can't remap keys. */
  memcpy(uuid.Char_UUID_128, rx_uuid, 16);
  aci_gatt_add_char(svc_handle, UUID_TYPE_128, &uuid, CFG_BLE_MAX_ATT_MTU - 3,
                    CHAR_PROP_WRITE | CHAR_PROP_WRITE_WITHOUT_RESP,
                    ATTR_PERMISSION_ENCRY_WRITE, GATT_NOTIFY_ATTRIBUTE_WRITE,
                    10, CHAR_VALUE_LEN_VARIABLE, &rx_handle);

  memcpy(uuid.Char_UUID_128, tx_uuid, 16);
  aci_gatt_add_char(svc_handle, UUID_TYPE_128, &uuid, NUS_CHUNK,
                    CHAR_PROP_NOTIFY, ATTR_PERMISSION_NONE, GATT_DONT_NOTIFY_EVENTS,
                    10, CHAR_VALUE_LEN_VARIABLE, &tx_handle);
}
