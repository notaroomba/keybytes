/*
 * CDC ACM interface: the config protocol over USB serial.
 *
 * RX: the USB ISR copies each packet into rx_ring and sets a sequencer
 * task that feeds proto.c from the main loop. When rx_ring can't take
 * another full packet the OUT endpoint isn't re-armed, so the host waits
 * instead of us dropping bytes; the task re-arms it once there's room.
 *
 * TX: proto.c writes into tx_ring and Tx_Kick() starts a packet whenever
 * the IN endpoint is idle. It runs from both the main loop and the
 * transmit-complete ISR, so it runs with interrupts off.
 */
#include "app_common.h"
#include "main.h"
#include "stm32_seq.h"
#include "utilities_conf.h"
#include "usbd_cdc_if.h"
#include "proto.h"

#define RX_SIZE 512
#define TX_SIZE 1024
/* 63, not 64: a packet shorter than the max size ends the transfer, so
 * the host never waits for a zero length packet. */
#define TX_CHUNK (CDC_DATA_FS_MAX_PACKET_SIZE - 1)

extern USBD_HandleTypeDef hUsbDeviceFS;

static uint8_t rx_packet[CDC_DATA_FS_MAX_PACKET_SIZE];
static uint8_t tx_packet[CDC_DATA_FS_MAX_PACKET_SIZE];

static uint8_t rx_ring[RX_SIZE];
static volatile uint16_t rx_head, rx_tail;
static volatile uint8_t rx_paused, rx_reset;
static char tx_ring[TX_SIZE];
static volatile uint16_t tx_head, tx_tail;

/* 115200 8N1, only stored and echoed back: the link is USB, not a UART */
static uint8_t line_coding[7] = {0x00, 0xC2, 0x01, 0x00, 0x00, 0x00, 0x08};

static void CDC_Write(const char *data, uint16_t len);
static proto_port_t cdc_port = {.write = CDC_Write, .local = 1};

static uint16_t Rx_Free(void)
{
  return (uint16_t)((rx_tail - rx_head - 1 + RX_SIZE) % RX_SIZE);
}

static uint8_t Configured(void)
{
  return hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED && hUsbDeviceFS.pClassData != NULL;
}

static void Tx_Kick(void)
{
  UTILS_ENTER_CRITICAL_SECTION();
  if (Configured() && tx_tail != tx_head)
  {
    uint16_t n = 0;
    for (uint16_t t = tx_tail; n < TX_CHUNK && t != tx_head; t = (t + 1) % TX_SIZE)
    {
      tx_packet[n++] = (uint8_t)tx_ring[t];
    }
    USBD_CDC_SetTxBuffer(&hUsbDeviceFS, tx_packet, n);
    /* USBD_BUSY while the previous packet is in flight: the complete
     * callback calls us again. */
    if (USBD_CDC_TransmitPacket(&hUsbDeviceFS) == USBD_OK)
    {
      tx_tail = (tx_tail + n) % TX_SIZE;
    }
  }
  UTILS_EXIT_CRITICAL_SECTION();
}

static void CDC_Write(const char *data, uint16_t len)
{
  if (!Configured()) return;
  for (uint16_t i = 0; i < len; i++)
  {
    uint16_t next = (tx_head + 1) % TX_SIZE;
    if (next == tx_tail) break;   /* full: drop the rest, the host can ask again */
    tx_ring[tx_head] = data[i];
    tx_head = next;
  }
  Tx_Kick();
}

static void Rx_Task(void)
{
  if (rx_reset)
  {
    rx_reset = 0;
    rx_tail = rx_head;
    cdc_port.len = 0;
    cdc_port.overflow = 0;
  }
  while (rx_tail != rx_head)
  {
    uint16_t end = rx_head;
    uint16_t n = (end > rx_tail ? end : RX_SIZE) - rx_tail;
    PROTO_Input(&cdc_port, &rx_ring[rx_tail], n);
    rx_tail = (rx_tail + n) % RX_SIZE;
  }
  if (rx_paused && Rx_Free() >= CDC_DATA_FS_MAX_PACKET_SIZE)
  {
    rx_paused = 0;
    if (Configured()) USBD_CDC_ReceivePacket(&hUsbDeviceFS);
  }
}

void CDC_Detached(void)
{
  cdc_port.connected = 0;
  tx_tail = tx_head;
  rx_reset = 1;
  rx_paused = 0;
}

static int8_t CDC_Init_FS(void)
{
  USBD_CDC_SetTxBuffer(&hUsbDeviceFS, tx_packet, 0);
  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, rx_packet);
  return (int8_t)USBD_OK;
}

static int8_t CDC_DeInit_FS(void)
{
  CDC_Detached();
  return (int8_t)USBD_OK;
}

static int8_t CDC_Control_FS(uint8_t cmd, uint8_t *pbuf, uint16_t length)
{
  switch (cmd)
  {
    case CDC_SET_LINE_CODING:
      if (length >= sizeof(line_coding)) memcpy(line_coding, pbuf, sizeof(line_coding));
      break;
    case CDC_GET_LINE_CODING:
      memcpy(pbuf, line_coding, sizeof(line_coding));
      break;
    case CDC_SET_CONTROL_LINE_STATE:
    {
      /* bit 0 of wValue is DTR: a terminal or the app opened the port */
      uint8_t dtr = ((USBD_SetupReqTypedef *)pbuf)->wValue & 1;
      cdc_port.connected = dtr;
      if (!dtr) tx_tail = tx_head;
      break;
    }
    default:
      break;
  }
  return (int8_t)USBD_OK;
}

static int8_t CDC_Receive_FS(uint8_t *Buf, uint32_t *Len)
{
  for (uint32_t i = 0; i < *Len; i++)
  {
    uint16_t next = (rx_head + 1) % RX_SIZE;
    if (next == rx_tail) break;   /* can't happen: we only re-arm with room for a packet */
    rx_ring[rx_head] = Buf[i];
    rx_head = next;
  }
  UTIL_SEQ_SetTask(1 << CFG_TASK_CDC_RX_ID, CFG_SCH_PRIO_0);

  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, rx_packet);
  if (Rx_Free() >= CDC_DATA_FS_MAX_PACKET_SIZE) USBD_CDC_ReceivePacket(&hUsbDeviceFS);
  else rx_paused = 1;
  return (int8_t)USBD_OK;
}

static int8_t CDC_TransmitCplt_FS(uint8_t *Buf, uint32_t *Len, uint8_t epnum)
{
  UNUSED(Buf);
  UNUSED(Len);
  UNUSED(epnum);
  Tx_Kick();
  return (int8_t)USBD_OK;
}

USBD_CDC_ItfTypeDef USBD_Interface_fops_FS =
{
  CDC_Init_FS,
  CDC_DeInit_FS,
  CDC_Control_FS,
  CDC_Receive_FS,
  CDC_TransmitCplt_FS
};

void CDC_PortInit(void)
{
  UTIL_SEQ_RegTask(1 << CFG_TASK_CDC_RX_ID, UTIL_SEQ_RFU, Rx_Task);
  PROTO_AddPort(&cdc_port);
}
