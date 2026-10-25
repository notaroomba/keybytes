/*
 * Bit-banged I2C target for the Keybit's four side buses.
 *
 * The CH32V003 has one hardware I2C, but every side of a Keybit has its own
 * SDA/SCL pair, so all four are done in software. The main loop polls each
 * bus for a START, then serves that whole transaction before going back to
 * polling.
 *
 * Reads arbitrate like a multi-controller bus: while sending a 1 we check
 * that SDA really is high. If another target sharing the address pulled it
 * low, we lost, stop driving, and the transfer carries on with the winner's
 * bytes. That is what lets several unaddressed Keybits answer one read of
 * their UIDs at once.
 */
#ifndef SOFT_I2C_H
#define SOFT_I2C_H

#include <stdint.h>

typedef struct {
  uint8_t sda, scl;   /* ch32fun pin numbers, e.g. PC4 */
  char side;          /* 'N', 'E', 'S' or 'W' */
  uint8_t prev_sda, prev_scl;
} soft_i2c_bus_t;

typedef struct {
  /* Return 1 if we answer to this 7-bit address on this bus. Called for
   * every address seen, so it also tells us who else uses the bus. */
  int (*match)(uint8_t addr, soft_i2c_bus_t *bus);
  /* A byte the controller reads, from register reg. */
  uint8_t (*read)(uint8_t reg, soft_i2c_bus_t *bus);
  /* A byte the controller wrote to register reg. */
  void (*write)(uint8_t reg, uint8_t value, soft_i2c_bus_t *bus);
  /* We lost arbitration while answering on addr. */
  void (*lost)(uint8_t addr);
} soft_i2c_target_t;

/* Make both pins open-drain and released (high through the 10k pull-ups). */
void soft_i2c_init(soft_i2c_bus_t *bus);

/* Look once for a START. If there is one, serve the transaction and return
 * 1. The first byte of a write sets the register pointer; reads and later
 * writes auto-increment it. */
int soft_i2c_poll(soft_i2c_bus_t *bus, const soft_i2c_target_t *t);

#endif
