#include "app_common.h"
#include "ble.h"
#include "app_ble.h"
#include "hids_app.h"

#define REPORT_IDX_KEYBOARD 0   /* report ID 1, see the patch in hids.c */
#define REPORT_IDX_CONSUMER 1   /* report ID 2 */

static const uint8_t report_map[] =
{
  0x05, 0x01,       /* Usage Page (Generic Desktop) */
  0x09, 0x06,       /* Usage (Keyboard) */
  0xA1, 0x01,       /* Collection (Application) */
  0x85, 0x01,       /*   Report ID (1) */
  0x05, 0x07,       /*   Usage Page (Keyboard/Keypad) */
  0x19, 0xE0,       /*   Usage Minimum (Left Control) */
  0x29, 0xE7,       /*   Usage Maximum (Right GUI) */
  0x15, 0x00,       /*   Logical Minimum (0) */
  0x25, 0x01,       /*   Logical Maximum (1) */
  0x75, 0x01,       /*   Report Size (1) */
  0x95, 0x08,       /*   Report Count (8) */
  0x81, 0x02,       /*   Input (Data, Var, Abs): modifiers */
  0x75, 0x08,       /*   Report Size (8) */
  0x95, 0x01,       /*   Report Count (1) */
  0x81, 0x01,       /*   Input (Const): reserved */
  0x75, 0x08,       /*   Report Size (8) */
  0x95, 0x06,       /*   Report Count (6) */
  0x15, 0x00,       /*   Logical Minimum (0) */
  0x25, 0xFF,       /*   Logical Maximum (255) */
  0x19, 0x00,       /*   Usage Minimum (0) */
  0x29, 0xFF,       /*   Usage Maximum (255) */
  0x81, 0x00,       /*   Input (Data, Array): 6 keys */
  0xC0,             /* End Collection */

  0x05, 0x0C,       /* Usage Page (Consumer) */
  0x09, 0x01,       /* Usage (Consumer Control) */
  0xA1, 0x01,       /* Collection (Application) */
  0x85, 0x02,       /*   Report ID (2) */
  0x15, 0x00,       /*   Logical Minimum (0) */
  0x26, 0xFF, 0x03, /*   Logical Maximum (0x3FF) */
  0x19, 0x00,       /*   Usage Minimum (0) */
  0x2A, 0xFF, 0x03, /*   Usage Maximum (0x3FF) */
  0x75, 0x10,       /*   Report Size (16) */
  0x95, 0x01,       /*   Report Count (1) */
  0x81, 0x00,       /*   Input (Data, Array) */
  0xC0,             /* End Collection */
};

/* bcdHID 1.11, country 0, flags: RemoteWake | NormallyConnectable */
static const uint8_t hid_info[4] = {0x11, 0x01, 0x00, 0x03};

static uint8_t last_keyboard[8];
static uint16_t last_consumer;

void HIDSAPP_Init(void)
{
  HIDS_Update_Char(REPORT_MAP_CHAR_UUID, 0, 0, sizeof(report_map), (uint8_t *)report_map);
  HIDS_Update_Char(HID_INFORMATION_CHAR_UUID, 0, 0, sizeof(hid_info), (uint8_t *)hid_info);
  HIDS_Update_Char(REPORT_CHAR_UUID, 0, REPORT_IDX_KEYBOARD, sizeof(last_keyboard), last_keyboard);
  HIDS_Update_Char(REPORT_CHAR_UUID, 0, REPORT_IDX_CONSUMER, sizeof(last_consumer), (uint8_t *)&last_consumer);
}

/* A host reconnecting expects every key released, so reset the cache and
 * let the next change go out. */
void HIDSAPP_Disconnected(void)
{
  memset(last_keyboard, 0, sizeof(last_keyboard));
  last_consumer = 0;
}

void HIDSAPP_SendKeyboard(const uint8_t report[8])
{
  if (memcmp(report, last_keyboard, sizeof(last_keyboard)) == 0) return;
  if (APP_BLE_ConnHandle() == APP_BLE_NO_CONN) return;
  /* Only remember what actually went out, so a report dropped for lack of
   * TX buffers is retried on the next 5 ms tick instead of leaving a key
   * stuck down on the host. */
  if (HIDS_Update_Char(REPORT_CHAR_UUID, 0, REPORT_IDX_KEYBOARD, 8, (uint8_t *)report) == BLE_STATUS_SUCCESS)
  {
    memcpy(last_keyboard, report, sizeof(last_keyboard));
  }
}

void HIDSAPP_SendConsumer(uint16_t usage)
{
  if (usage == last_consumer) return;
  if (APP_BLE_ConnHandle() == APP_BLE_NO_CONN) return;
  if (HIDS_Update_Char(REPORT_CHAR_UUID, 0, REPORT_IDX_CONSUMER, sizeof(usage), (uint8_t *)&usage) == BLE_STATUS_SUCCESS)
  {
    last_consumer = usage;
  }
}

void HIDS_Notification(HIDS_App_Notification_evt_t *pNotification)
{
  UNUSED(pNotification);
}
