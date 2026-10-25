/*
 * Keymap: which HID modifiers and key each Keybit sends, keyed by the
 * Keybit's 96-bit UID. Lives in RAM, remapped over the config protocol,
 * saved to one flash page just below CPU2's secure area.
 *
 * A 5 ms sequencer task watches keybits[] from main.c, sends a HID report
 * whenever the set of held mapped keys changes, and emits the protocol's
 * KEY+/KEY-/PRESS/SIDES events.
 */
#ifndef KEYMAP_H
#define KEYMAP_H

#include <stdint.h>
#include "keybit_bus.h"

#define KEYMAP_MAX 32
#define UID_LEN 12

void KEYMAP_Init(void);

enum { KEYMAP_NONE = 0, KEYMAP_KEY = 1, KEYMAP_CONSUMER = 2 };

/* A Keybit maps to a keyboard key or a consumer usage, never both: each
 * setter replaces whatever was there. 0 on success, -1 when the table is
 * full. key may be 0 (mods only). */
int KEYMAP_Set(const uint8_t uid[UID_LEN], uint8_t mods, uint8_t key);
int KEYMAP_SetConsumer(const uint8_t uid[UID_LEN], uint16_t usage);
/* Returns the kind. code is the keyboard usage or the consumer usage,
 * mods and code are 0 when unmapped. */
int KEYMAP_Get(const uint8_t uid[UID_LEN], uint8_t *mods, uint16_t *code);
void KEYMAP_Unmap(const uint8_t uid[UID_LEN]);
void KEYMAP_Clear(void);
/* 0 on success (also when flash already matched), -1 on a flash error. */
int KEYMAP_Save(void);

/* Keybits as of the last key task run, the same set the events describe. */
const keybit_info_t *KEYMAP_Present(int *count);

#endif /* KEYMAP_H */
