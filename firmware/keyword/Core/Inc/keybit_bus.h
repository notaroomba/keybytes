/*
 * Keybits on the keyword's side bus (I2C3): hand out addresses and list
 * who is there. See firmware/keybit/main.c for the Keybit's register map.
 */
#ifndef KEYBIT_BUS_H
#define KEYBIT_BUS_H

#include <stdint.h>
#include "stm32wbxx_hal.h"

#define KEYBIT_DEFAULT_ADDR 0x30
#define KEYBIT_MAX 8
#define KEYBIT_REMOTE_MAX 112
#define KEYBIT_REMOTE_TIMEOUT_MS 3000

typedef struct {
  uint8_t addr;
  uint8_t uid[12];   /* factory 96-bit ID, little endian */
  uint8_t fw;
  uint8_t key_down;
  char keybit_side;  /* the Keybit's own side touching us: N/E/S/W */
  char at;           /* where it is on the keyword: L, R or T, '?' if unknown */
  uint8_t backwards; /* its sides with a Keybit on backwards: bit 0 N, 1 E, 2 S, 3 W */
  /* Taps too short for the key poll to see, caught from the Keybit's own
   * queued events: saw_down is set when a poll reads it down, pend when
   * its press event arrived unseen, tap 1 = report a down next, 2 = then up. */
  uint8_t saw_down, pend, tap;
} keybit_info_t;

/* A Keybit further out, known only through events relayed by its
 * neighbours. Entries are never moved, so an index stays valid. */
typedef struct {
  uint32_t uid;        /* low 32 bits of its UID */
  uint8_t present;     /* 0 once it has gone quiet for KEYBIT_REMOTE_TIMEOUT_MS */
  uint8_t key_down;
  uint8_t depth;       /* hops from the keyword, 2 or more */
  uint8_t backwards;   /* it has a neighbour on backwards (side unknown) */
  uint8_t via;         /* address of the touching Keybit it came through */
  uint8_t dirty;       /* set on any change; the consumer clears it */
  uint8_t fresh_down;  /* went down in a relay batch the consumer hasn't seen yet */
  /* Where it is (v11 Keybits): the Keybit it hangs off (low 32 bits of its
   * UID), that Keybit's side it's on and its own side facing it, as
   * N/E/S/W; 0 / '?' until known. moved: these changed since last reported. */
  uint32_t parent;
  char pside, face;
  uint8_t moved;
  uint8_t tap;         /* a press and release came in one batch: 1 = report down, 2 = then up */
  uint32_t last_seen;  /* HAL_GetTick() */
} keybit_remote_t;

/* SDA low with SCL high while we aren't using the bus. A Keybit turned the
 * wrong way round on the keyword does this: its GND lands on our SDA and
 * its 3V3 on our SCL, so every SCL low we drive pulls its supply down
 * through its SCL clamp diode. keybit_bus_held() reports the state;
 * keybit_bus_jammed() is the same but lets one attempt through about once
 * a second, so a Keybit stuck mid-byte still gets clocked free. Check
 * keybit_bus_jammed() before starting any transfer. */
int keybit_bus_held(void);
int keybit_bus_jammed(void);

/* SCL low with the bus idle for over 5 ms means something stopped
 * mid-transfer and is waiting on a clock. Clock it free: I2C3 off, 9 SCL
 * pulses and a STOP by hand, I2C3 back on. Returns 1 when it did that. */
int keybit_bus_unstick(I2C_HandleTypeDef *i2c);

/* Give every Keybit still on the default address its own address. Returns
 * how many were assigned. */
int keybit_bus_assign(I2C_HandleTypeDef *i2c);

/* Read every assigned Keybit into out. Returns the count. Probes all
 * addresses, so it takes tens of ms; run it every few hundred ms. */
int keybit_bus_scan(I2C_HandleTypeDef *i2c, keybit_info_t *out, int max);

/* Fast path for typing: refresh key_down of the n Keybits from the last
 * scan, one 1-byte read each (well under 1 ms apiece at 100 kHz). Returns a
 * bitmask of entries whose key state changed. */
uint32_t keybit_bus_poll_keys(I2C_HandleTypeDef *i2c, keybit_info_t *k, int n);

/* Drain relayed events from the n touching Keybits into the remote table
 * (r, with *rn entries in use) and age out silent ones. Run it with
 * keybit_bus_poll_keys. Returns 1 if any entry changed. */
int keybit_bus_poll_relay(I2C_HandleTypeDef *i2c, keybit_info_t *k, int n,
                          keybit_remote_t *r, int *rn);

/* Plain register access for other devices on I2C3, such as the BQ25895
 * charger (0x6A, never handed to a Keybit). Everything on the bus runs from
 * the main loop, so call these from there too, not from an interrupt.
 * Return 1 on success. */
int keybit_bus_read(I2C_HandleTypeDef *i2c, uint8_t addr7, uint8_t reg, uint8_t *buf, uint8_t len);
int keybit_bus_write(I2C_HandleTypeDef *i2c, uint8_t addr7, uint8_t reg, const uint8_t *buf, uint8_t len);

#endif
