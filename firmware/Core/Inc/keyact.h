/*
 * What a Keybit does when pressed, and the state machine that turns held
 * keys into HID reports. No HAL in here so it runs in the host test
 * (test/keyact_test.c).
 */
#ifndef KEYACT_H
#define KEYACT_H

#include <stdint.h>

#define KEYACT_TAP_MS       10    /* how long a tap stays in the reports */
#define KEYACT_HOLD_MS_DEF  200

enum { KEYACT_NONE = 0, KEYACT_KEY = 1, KEYACT_CONSUMER = 2 };

/* Stored per Keybit in the keymap (and in flash, so keep it 4-byte sized).
 * code is the keyboard usage for KEYACT_KEY and the consumer usage for
 * KEYACT_CONSUMER. With a hold set (hold_mods or hold_key non zero) a
 * press shorter than hold_ms fires the normal action as a tap on release,
 * a longer one holds hold_mods/hold_key down until release. */
typedef struct
{
  uint8_t kind;
  uint8_t mods;
  uint16_t code;
  uint8_t hold_mods;
  uint8_t hold_key;
  uint16_t hold_ms;
} keyact_t;

/* Per key runtime state, kept by the caller across calls (by UID). The
 * caller only writes down; zero it when a key first appears. */
typedef struct
{
  uint8_t down;
  uint8_t was;
  uint8_t state;
  uint32_t seq;
  uint32_t t0;
} keyact_key_t;

/* One step: keys[i] is driven by acts[i] (NULL when unmapped). Builds the
 * 8 byte keyboard report and the consumer usage to send now. */
void KEYACT_Step(keyact_key_t *keys, const keyact_t *const *acts, int n, uint32_t now,
                 uint8_t report[8], uint16_t *consumer);

void KEYACT_Reset(void);

#endif /* KEYACT_H */
