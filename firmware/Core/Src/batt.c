#include "batt.h"
#include "keybit_bus.h"
#include "proto.h"

/* Register map from TI's BQ25895 datasheet (SLUSC88C), section 8.4. */
#define BQ_ADDR       0x6A
#define REG02         0x02
#define REG02_CONV_RATE 0x40   /* 1 s continuous ADC */
#define REG02_TRIGGERS  0x82   /* CONV_START, FORCE_DPDM: self clearing, never write back a 1 */
#define REG0B         0x0B     /* VBUS_STAT[7:5] CHRG_STAT[4:3] PG_STAT[2] */
#define REG0E_BATV(r) ((r) & 0x7F)          /* 2.304 V + 20 mV/LSB */
#define REG11_VBUS_GD 0x80

#define PCT_STEP 5   /* send an unsolicited BATT line when pct moves this much */

extern I2C_HandleTypeDef hi2c3;
/* app_ble.c: feeds the BLE Battery Service, no-op until BLE is up. */
void APP_BLE_SetBattery(uint8_t pct);

static batt_t batt;
static batt_t sent;    /* last BATT line we broadcast */
static uint32_t last_poll;
static uint8_t polled, adc_was_on;

/* ponytail: generic single cell LiPo curve at rest, a fuel gauge or a
 * per-cell calibration would do better */
static const struct { uint16_t mv; uint8_t pct; } curve[] = {
  {4200, 100}, {4150, 95}, {4110, 90}, {4020, 80}, {3950, 70}, {3910, 60},
  {3870, 50}, {3840, 40}, {3800, 30}, {3760, 20}, {3730, 10}, {3600, 5}, {3300, 0},
};

uint8_t BATT_Percent(uint16_t mv)
{
  if (mv >= curve[0].mv) return 100;
  for (unsigned i = 1; i < sizeof(curve) / sizeof(curve[0]); i++)
  {
    if (mv >= curve[i].mv)
    {
      /* linear between the two points around mv */
      uint32_t span = curve[i - 1].mv - curve[i].mv;
      uint32_t up = mv - curve[i].mv;
      return (uint8_t)(curve[i].pct + (up * (curve[i - 1].pct - curve[i].pct) + span / 2) / span);
    }
  }
  return 0;
}

void BATT_Decode(const uint8_t regs[7], batt_t *out)
{
  uint8_t chrg = (regs[0] >> 3) & 3;               /* REG0B */
  out->mv = (uint16_t)(2304 + 20 * REG0E_BATV(regs[3]));   /* REG0E */
  out->pct = BATT_Percent(out->mv);
  out->chg = chrg == 1 || chrg == 2;               /* pre-charge, fast charge */
  out->usb = (regs[6] & REG11_VBUS_GD) != 0;       /* REG11 */
  out->valid = 1;
}

const batt_t *BATT_Get(void)
{
  return &batt;
}

static int Diff(uint8_t a, uint8_t b)
{
  return a > b ? a - b : b - a;
}

void BATT_Poll(uint32_t now)
{
  if (polled && (uint32_t)(now - last_poll) < BATT_POLL_MS) return;
  polled = 1;
  last_poll = now;

  /* CONV_RATE is cleared by a REG_RST or the charger's I2C watchdog, so
   * check it every time and turn it back on. A reading taken right after
   * turning it on is still the stale default, so skip that one. */
  uint8_t r02;
  if (!keybit_bus_read(&hi2c3, BQ_ADDR, REG02, &r02, 1)) return;
  if (!(r02 & REG02_CONV_RATE))
  {
    r02 = (uint8_t)((r02 | REG02_CONV_RATE) & ~REG02_TRIGGERS);
    keybit_bus_write(&hi2c3, BQ_ADDR, REG02, &r02, 1);
    adc_was_on = 0;
    return;
  }
  if (!adc_was_on)
  {
    adc_was_on = 1;
    return;
  }

  uint8_t regs[7];
  if (!keybit_bus_read(&hi2c3, BQ_ADDR, REG0B, regs, sizeof(regs))) return;
  BATT_Decode(regs, &batt);
  APP_BLE_SetBattery(batt.pct);

  if (!sent.valid || Diff(batt.pct, sent.pct) >= PCT_STEP || batt.chg != sent.chg || batt.usb != sent.usb)
  {
    sent = batt;
    PROTO_Send("BATT pct=%u chg=%u usb=%u", batt.pct, batt.chg, batt.usb);
  }
}
