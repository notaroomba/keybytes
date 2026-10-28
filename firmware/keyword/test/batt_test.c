/*
 * Host check for Core/Src/batt.c: BQ25895 register decoding, the LiPo
 * curve, turning on the ADC, and when BATT lines go out. The charger is a
 * fake register file behind keybit_bus_read/keybit_bus_write.
 *   cc -std=c99 -Itest/stub -ICore/Inc test/batt_test.c Core/Src/batt.c -o /tmp/batt_test && /tmp/batt_test
 */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "batt.h"
#include "keybit_bus.h"

I2C_HandleTypeDef hi2c3;
static uint8_t bq[0x15];
static int writes, bus_ok = 1;
static char sent[256];
static uint8_t ble_level = 0xFF;

int keybit_bus_read(I2C_HandleTypeDef *i2c, uint8_t addr7, uint8_t reg, uint8_t *buf, uint8_t len)
{
  assert(i2c == &hi2c3 && addr7 == 0x6A);
  if (!bus_ok) return 0;
  memcpy(buf, &bq[reg], len);
  return 1;
}

int keybit_bus_write(I2C_HandleTypeDef *i2c, uint8_t addr7, uint8_t reg, const uint8_t *buf, uint8_t len)
{
  assert(i2c == &hi2c3 && addr7 == 0x6A && reg == 0x02 && len == 1);
  bq[reg] = buf[0];
  writes++;
  return 1;
}

void PROTO_Send(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(sent + strlen(sent), sizeof(sent) - strlen(sent), fmt, ap);
  va_end(ap);
  strcat(sent, "\n");
}

void APP_BLE_SetBattery(uint8_t pct) { ble_level = pct; }

/* VBAT in mV to the REG0E code */
static uint8_t batv(uint16_t mv) { return (uint8_t)((mv - 2304) / 20); }

int main(void)
{
  /* curve ends, a table point, and between points */
  assert(BATT_Percent(4300) == 100);
  assert(BATT_Percent(4200) == 100);
  assert(BATT_Percent(3870) == 50);
  assert(BATT_Percent(3890) == 55);
  assert(BATT_Percent(3300) == 0);
  assert(BATT_Percent(3000) == 0);
  for (uint16_t mv = 3000; mv < 4300; mv += 10) assert(BATT_Percent(mv) <= BATT_Percent(mv + 10));

  /* REG0B..REG11: fast charging, VBAT 3.904 V, VBUS good */
  uint8_t regs[7] = {0};
  regs[0] = (2 << 3);
  regs[3] = 80;          /* 2304 + 80 * 20 = 3904 mV */
  regs[6] = 0x80 | 24;   /* VBUS_GD, 5.0 V */
  batt_t b;
  BATT_Decode(regs, &b);
  assert(b.valid && b.mv == 3904 && b.chg && b.usb);
  regs[0] = (3 << 3);    /* charge done is not charging */
  regs[6] = 0;
  BATT_Decode(regs, &b);
  assert(!b.chg && !b.usb);
  regs[0] = (1 << 3);    /* pre-charge is */
  BATT_Decode(regs, &b);
  assert(b.chg);

  /* the charger at reset: ADC off, REG02 default 0x3D plus a FORCE_DPDM
   * in progress. First poll turns the ADC on and changes nothing else. */
  bq[0x02] = 0x3D | 0x02;
  bq[0x0E] = batv(4024);   /* on the 20 mV ADC grid */
  bq[0x11] = 0x80;
  BATT_Poll(0);
  assert(writes == 1 && bq[0x02] == (0x3D | 0x40));   /* CONV_RATE on, FORCE_DPDM not re-sent */
  assert(!BATT_Get()->valid && sent[0] == 0);
  BATT_Poll(500);   /* not a second yet */
  assert(!BATT_Get()->valid);
  BATT_Poll(1000);  /* first poll with the ADC on still has the stale value: skipped */
  assert(!BATT_Get()->valid && sent[0] == 0);
  BATT_Poll(2000);
  assert(BATT_Get()->valid && BATT_Get()->pct == 80 && BATT_Get()->usb && !BATT_Get()->chg);
  assert(strcmp(sent, "BATT pct=80 chg=0 usb=1\n") == 0);
  assert(ble_level == 80);
  assert(writes == 1);

  /* small drift: no line, but the BLE level follows */
  sent[0] = 0;
  bq[0x0E] = batv(4004);   /* 78 % */
  BATT_Poll(3000);
  assert(sent[0] == 0 && BATT_Get()->pct < 80 && ble_level == BATT_Get()->pct);
  bq[0x0E] = batv(3944);   /* 69 %: 11 down from the last line */
  BATT_Poll(4000);
  assert(strcmp(sent, "BATT pct=69 chg=0 usb=1\n") == 0);

  /* plugging in and charging send a line straight away */
  sent[0] = 0;
  bq[0x0B] = (2 << 3);
  BATT_Poll(5000);
  assert(strcmp(sent, "BATT pct=69 chg=1 usb=1\n") == 0);

  /* the watchdog reset the charger: ADC back on, one reading skipped */
  sent[0] = 0;
  bq[0x02] = 0x3D;
  BATT_Poll(6000);
  assert(writes == 2 && (bq[0x02] & 0x40));
  BATT_Poll(7000);
  BATT_Poll(8000);
  assert(BATT_Get()->valid && sent[0] == 0);   /* unchanged: no line */

  /* a bus error leaves the last reading alone */
  bus_ok = 0;
  BATT_Poll(9000);
  assert(BATT_Get()->valid && BATT_Get()->pct == 69);

  puts("batt_test: ok");
  return 0;
}
