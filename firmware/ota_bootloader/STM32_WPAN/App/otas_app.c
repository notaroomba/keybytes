/*
 * OTA service application side, same wire protocol as ST's BLE_Ota
 * otas_app.c (STM32CubeWB 1.24.0) so ST BLE Toolbox works with it:
 *
 *   Base address char: [cmd, addr23..16, addr15..8, addr7..0]
 *     cmd 0x02 = application upload starting at 0x08000000 + addr
 *     cmd 0x07 = upload finished, we indicate 0x01 on the conf char
 *     cmd 0x00 / 0x08 = stop / cancel
 *   Raw data char: the image bytes, appended in order
 *   Conf char: indication 0x01 "reboot confirmed"; once the client
 *     confirms it we flush and reset into the new app.
 *
 * Differences from ST's, all to keep a bad transfer from hurting the
 * keyword:
 *   - only application uploads, and only into [0x08007000, keymap page);
 *     anything outside is dropped instead of written
 *   - wireless (CPU2) uploads are refused: installing those through FUS
 *     would move SFSA
 *   - a doubleword that won't program (page not erased) aborts the upload
 *     after a few tries instead of looping forever
 */
#include "app_common.h"
#include "main.h"
#include "ble.h"
#include "app_ble.h"
#include "flash_driver.h"

#define DW 8          /* flash programs 64 bits at a time */
#define TRIES 5

typedef enum
{
  ST_IDLE,
  ST_RECEIVING,
  ST_REJECTED,
} ota_state_t;

static ota_state_t state;
static uint32_t address;     /* next doubleword to program */
static uint64_t pending;     /* bytes waiting for a full doubleword */
static uint8_t pending_len;

static uint32_t End_Address(void)
{
  return FLASH_BASE + APP_BLE_EndAppPage() * FLASH_PAGE_SIZE;
}

/* Programs one doubleword at address. 0 on success. */
static int Program(uint64_t value)
{
  if (address < CFG_APP_START_ADDRESS || address + DW > End_Address()) return -1;
  for (int t = 0; t < TRIES; t++)
  {
    while (FD_WriteData(address, &value, 1) != 0);   /* CPU2 busy: try again */
    if (*(volatile uint64_t *)address == value)
    {
      address += DW;
      return 0;
    }
  }
  return -1;
}

static void Raw_Data(const uint8_t *data, uint32_t len)
{
  while (len && state == ST_RECEIVING)
  {
    uint32_t n = DW - pending_len;
    if (n > len) n = len;
    memcpy((uint8_t *)&pending + pending_len, data, n);
    pending_len += n;
    data += n;
    len -= n;
    if (pending_len == DW)
    {
      if (Program(pending) != 0) state = ST_REJECTED;
      pending_len = 0;
    }
  }
}

void OTAS_STM_Notification(OTA_STM_Notification_t *p_notification)
{
  switch (p_notification->ChardId)
  {
    case OTAS_STM_BASE_ADDR_ID:
    {
      const OTA_STM_Base_Addr_Event_Format_t *cmd = (const OTA_STM_Base_Addr_Event_Format_t *)p_notification->pPayload;
      uint32_t addr = FLASH_BASE | (uint32_t)cmd->Base_Addr[0] << 16 | (uint32_t)cmd->Base_Addr[1] << 8 | cmd->Base_Addr[2];
      switch (cmd->Command)
      {
        case OTAS_STM_APPLICATION_UPLOAD:
          address = addr;
          pending_len = 0;
          state = (addr >= CFG_APP_START_ADDRESS && addr < End_Address() && (addr % DW) == 0) ? ST_RECEIVING : ST_REJECTED;
          break;
        case OTAS_STM_WIRELESS_FW_UPLOAD:
          state = ST_REJECTED;
          break;
        case OTAS_STM_UPLOAD_FINISHED:
          if (state == ST_RECEIVING)
          {
            OTAS_STM_Indication_Msg_t msg = OTAS_STM_REBOOT_CONFIRMED;
            OTAS_STM_UpdateChar(OTAS_STM_CONF_ID, (uint8_t *)&msg);
          }
          break;
        default:   /* stop all, cancel */
          state = ST_IDLE;
          break;
      }
      break;
    }

    case OTAS_STM_RAW_DATA_ID:
      Raw_Data(p_notification->pPayload, p_notification->ValueLength);
      break;

    case OTAS_STM_CONF_EVENT_ID:
      /* The client confirmed "reboot": write the last partial doubleword
       * (padded with erased-flash 0xFF) and start the new app. If the
       * image never got its end tag the loader comes back up for another
       * try. */
      if (state == ST_RECEIVING && pending_len)
      {
        memset((uint8_t *)&pending + pending_len, 0xFF, DW - pending_len);
        Program(pending);
      }
      CFG_OTA_REBOOT_VAL_MSG = CFG_REBOOT_ON_FW_APP;
      NVIC_SystemReset();
      break;

    default:
      break;
  }
}
