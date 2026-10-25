/*
 * Battery state from the BQ25895 charger (U8) on I2C3, address 0x6A.
 * The only write is turning on its ADC (REG02 CONV_RATE, 1 s continuous);
 * every charger setting stays as it is.
 */
#ifndef BATT_H
#define BATT_H

#include <stdint.h>

#define BATT_POLL_MS 1000

typedef struct
{
  uint8_t valid;   /* 0 until the first real ADC reading */
  uint8_t pct;     /* 0-100, estimated from VBAT */
  uint8_t chg;     /* pre-charge or fast charge */
  uint8_t usb;     /* VBUS attached */
  uint16_t mv;     /* VBAT */
} batt_t;

/* Call often from the main loop or a sequencer task (never an ISR); it
 * reads the charger every BATT_POLL_MS and sends BATT lines on change. */
void BATT_Poll(uint32_t now);

const batt_t *BATT_Get(void);

/* LiPo state of charge from the resting cell voltage. Reads high while
 * charging, since VBAT sits above the resting voltage then. */
uint8_t BATT_Percent(uint16_t mv);

/* REG0B..REG11 (7 bytes) into pct/chg/usb/mv, valid set. */
void BATT_Decode(const uint8_t regs[7], batt_t *out);

#endif /* BATT_H */
