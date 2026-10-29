/*
 * The Keyword on USB: the serial port the Keyword Monitor app talks to
 * (ST's CDC class, interfaces 0 and 1, unchanged) and a keyboard (HID,
 * interface 2) that types from the keymap in flash, app or no app.
 * ST's library only has CDC, so this class hands everything that isn't
 * the keyboard's to it.
 */
#include <string.h>
#include "usbd_kw.h"
#include "usbd_cdc.h"
#include "usbd_ctlreq.h"

#define HID_IF        2U
#define HID_EP        0x83U
#define HID_EP_SIZE   16U
#define HID_INTERVAL  1U     /* ms */
#define REPORT_KEYS   1U
#define REPORT_MEDIA  2U

#define HID_REQ_GET_REPORT   0x01U
#define HID_REQ_GET_IDLE     0x02U
#define HID_REQ_GET_PROTOCOL 0x03U
#define HID_REQ_SET_REPORT   0x09U
#define HID_REQ_SET_IDLE     0x0AU
#define HID_REQ_SET_PROTOCOL 0x0BU
#define DESC_HID             0x21U
#define DESC_HID_REPORT      0x22U

/* Keyboard (report 1: modifiers, reserved, 6 keys; LED output) and media
 * keys (report 2: one 16-bit consumer usage). */
static const uint8_t report_desc[] = {
  0x05, 0x01, 0x09, 0x06, 0xA1, 0x01, 0x85, REPORT_KEYS,
  0x05, 0x07, 0x19, 0xE0, 0x29, 0xE7, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x08, 0x81, 0x02,
  0x95, 0x01, 0x75, 0x08, 0x81, 0x01,
  0x95, 0x05, 0x75, 0x01, 0x05, 0x08, 0x19, 0x01, 0x29, 0x05, 0x91, 0x02,
  0x95, 0x01, 0x75, 0x03, 0x91, 0x01,
  0x95, 0x06, 0x75, 0x08, 0x15, 0x00, 0x26, 0xFF, 0x00, 0x05, 0x07, 0x19, 0x00, 0x2A, 0xFF, 0x00, 0x81, 0x00,
  0xC0,
  0x05, 0x0C, 0x09, 0x01, 0xA1, 0x01, 0x85, REPORT_MEDIA,
  0x15, 0x00, 0x26, 0xFF, 0x03, 0x19, 0x00, 0x2A, 0xFF, 0x03, 0x75, 0x10, 0x95, 0x01, 0x81, 0x00,
  0xC0,
};

#define HID_DESC(len) 0x09, DESC_HID, 0x11, 0x01, 0x00, 0x01, DESC_HID_REPORT, LOBYTE(len), HIBYTE(len)
#define CFG_SIZE 100U

__ALIGN_BEGIN static uint8_t cfg_desc[CFG_SIZE] __ALIGN_END = {
  0x09, USB_DESC_TYPE_CONFIGURATION, LOBYTE(CFG_SIZE), HIBYTE(CFG_SIZE), 0x03, 0x01, 0x00, 0xC0, USBD_MAX_POWER,
  /* the serial port: interface association, then CDC as ST's class has it */
  0x08, 0x0B, 0x00, 0x02, 0x02, 0x02, 0x01, 0x00,
  0x09, USB_DESC_TYPE_INTERFACE, 0x00, 0x00, 0x01, 0x02, 0x02, 0x01, 0x00,
  0x05, 0x24, 0x00, 0x10, 0x01,
  0x05, 0x24, 0x01, 0x00, 0x01,
  0x04, 0x24, 0x02, 0x02,
  0x05, 0x24, 0x06, 0x00, 0x01,
  0x07, USB_DESC_TYPE_ENDPOINT, CDC_CMD_EP, 0x03, LOBYTE(CDC_CMD_PACKET_SIZE), HIBYTE(CDC_CMD_PACKET_SIZE), CDC_FS_BINTERVAL,
  0x09, USB_DESC_TYPE_INTERFACE, 0x01, 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00,
  0x07, USB_DESC_TYPE_ENDPOINT, CDC_OUT_EP, 0x02, LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE), HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE), 0x00,
  0x07, USB_DESC_TYPE_ENDPOINT, CDC_IN_EP, 0x02, LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE), HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE), 0x00,
  /* the keyboard */
  0x09, USB_DESC_TYPE_INTERFACE, HID_IF, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00,   /* no boot protocol: reports carry IDs */
  HID_DESC(sizeof(report_desc)),
  0x07, USB_DESC_TYPE_ENDPOINT, HID_EP, 0x03, HID_EP_SIZE, 0x00, HID_INTERVAL,
};

static struct {
  uint8_t busy, protocol, idle;
  uint8_t keys[9], media[3];   /* last sent, report ID first */
  uint8_t keys_due, media_due; /* changed while the endpoint was busy */
  uint8_t tx[9];
  uint8_t out[8];              /* LED reports from the host, ignored */
  uint8_t out_due;             /* the EP0 data on its way is one of those */
} hid;

static void send_next(USBD_HandleTypeDef *pdev)
{
  if (hid.busy) return;
  if (hid.keys_due) { hid.keys_due = 0; memcpy(hid.tx, hid.keys, sizeof(hid.keys)); }
  else if (hid.media_due) { hid.media_due = 0; memcpy(hid.tx, hid.media, sizeof(hid.media)); }
  else return;
  hid.busy = 1;
  USBD_LL_Transmit(pdev, HID_EP, hid.tx, hid.tx[0] == REPORT_KEYS ? sizeof(hid.keys) : sizeof(hid.media));
}

static uint8_t KW_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  memset(&hid, 0, sizeof(hid));
  hid.protocol = 1;
  hid.keys[0] = REPORT_KEYS;
  hid.media[0] = REPORT_MEDIA;
  USBD_LL_OpenEP(pdev, HID_EP, USBD_EP_TYPE_INTR, HID_EP_SIZE);
  pdev->ep_in[HID_EP & 0xFU].is_used = 1U;
  return USBD_CDC.Init(pdev, cfgidx);
}

static uint8_t KW_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  USBD_LL_CloseEP(pdev, HID_EP);
  pdev->ep_in[HID_EP & 0xFU].is_used = 0U;
  hid.busy = 0;
  return USBD_CDC.DeInit(pdev, cfgidx);
}

static uint8_t HID_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  static uint8_t one;
  switch (req->bmRequest & USB_REQ_TYPE_MASK) {
  case USB_REQ_TYPE_CLASS:
    switch (req->bRequest) {
    case HID_REQ_SET_PROTOCOL: hid.protocol = LOBYTE(req->wValue); return USBD_OK;
    case HID_REQ_GET_PROTOCOL: one = hid.protocol; USBD_CtlSendData(pdev, &one, 1); return USBD_OK;
    case HID_REQ_SET_IDLE:     hid.idle = HIBYTE(req->wValue); return USBD_OK;
    case HID_REQ_GET_IDLE:     one = hid.idle; USBD_CtlSendData(pdev, &one, 1); return USBD_OK;
    case HID_REQ_GET_REPORT:
      if (LOBYTE(req->wValue) == REPORT_MEDIA) USBD_CtlSendData(pdev, hid.media, MIN(req->wLength, sizeof(hid.media)));
      else USBD_CtlSendData(pdev, hid.keys, MIN(req->wLength, sizeof(hid.keys)));
      return USBD_OK;
    case HID_REQ_SET_REPORT:   /* caps lock and friends: we have no LEDs for them */
      hid.out_due = 1;
      USBD_CtlPrepareRx(pdev, hid.out, MIN(req->wLength, sizeof(hid.out)));
      return USBD_OK;
    default: break;
    }
    break;
  case USB_REQ_TYPE_STANDARD:
    switch (req->bRequest) {
    case USB_REQ_GET_DESCRIPTOR:
      if (HIBYTE(req->wValue) == DESC_HID_REPORT) {
        USBD_CtlSendData(pdev, (uint8_t *)report_desc, MIN(req->wLength, sizeof(report_desc)));
        return USBD_OK;
      }
      if (HIBYTE(req->wValue) == DESC_HID) {
        USBD_CtlSendData(pdev, &cfg_desc[CFG_SIZE - 7 - 9], MIN(req->wLength, 9U));
        return USBD_OK;
      }
      break;
    case USB_REQ_GET_INTERFACE: one = 0; USBD_CtlSendData(pdev, &one, 1); return USBD_OK;
    case USB_REQ_SET_INTERFACE:
    case USB_REQ_GET_STATUS:
    case USB_REQ_CLEAR_FEATURE: return USBD_OK;
    default: break;
    }
    break;
  default: break;
  }
  USBD_CtlError(pdev, req);
  return USBD_FAIL;
}

static uint8_t KW_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  uint8_t to_hid = (req->bmRequest & USB_REQ_RECIPIENT_MASK) == USB_REQ_RECIPIENT_INTERFACE
                   ? LOBYTE(req->wIndex) == HID_IF
                   : (req->bmRequest & USB_REQ_RECIPIENT_MASK) == USB_REQ_RECIPIENT_ENDPOINT && LOBYTE(req->wIndex) == HID_EP;
  return to_hid ? HID_Setup(pdev, req) : USBD_CDC.Setup(pdev, req);
}

static uint8_t KW_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  if (epnum == (HID_EP & 0xFU)) {
    hid.busy = 0;
    send_next(pdev);
    return USBD_OK;
  }
  return USBD_CDC.DataIn(pdev, epnum);
}

static uint8_t KW_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
  if (hid.out_due) { hid.out_due = 0; return USBD_OK; }   /* not a CDC command */
  return USBD_CDC.EP0_RxReady(pdev);
}
static uint8_t KW_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum) { return USBD_CDC.DataOut(pdev, epnum); }
static uint8_t *KW_Qualifier(uint16_t *length) { return USBD_CDC.GetDeviceQualifierDescriptor(length); }

static uint8_t *KW_GetCfgDesc(uint16_t *length)
{
  *length = CFG_SIZE;
  return cfg_desc;
}

USBD_ClassTypeDef USBD_KW = {
  KW_Init,
  KW_DeInit,
  KW_Setup,
  NULL,
  KW_EP0_RxReady,
  KW_DataIn,
  KW_DataOut,
  NULL, NULL, NULL,
  KW_GetCfgDesc,
  KW_GetCfgDesc,
  KW_GetCfgDesc,
  KW_Qualifier,
};

int USBD_KW_Ready(USBD_HandleTypeDef *pdev)
{
  return pdev->dev_state == USBD_STATE_CONFIGURED;
}

/* Called from the main loop; DataIn runs in the USB interrupt. */
void USBD_KW_Keyboard(USBD_HandleTypeDef *pdev, const uint8_t report[8])
{
  if (!USBD_KW_Ready(pdev) || memcmp(&hid.keys[1], report, 8) == 0) return;
  uint32_t pm = __get_PRIMASK();
  __disable_irq();
  memcpy(&hid.keys[1], report, 8);
  hid.keys_due = 1;
  send_next(pdev);
  __set_PRIMASK(pm);
}

void USBD_KW_Media(USBD_HandleTypeDef *pdev, uint16_t usage)
{
  if (!USBD_KW_Ready(pdev) || (hid.media[1] | hid.media[2] << 8) == usage) return;
  uint32_t pm = __get_PRIMASK();
  __disable_irq();
  hid.media[1] = LOBYTE(usage);
  hid.media[2] = HIBYTE(usage);
  hid.media_due = 1;
  send_next(pdev);
  __set_PRIMASK(pm);
}
