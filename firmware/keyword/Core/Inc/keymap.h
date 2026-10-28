/*
 * Keymap: what each Keybit sends (table in keytable.h), saved to one
 * flash page just below CPU2's secure area.
 *
 * A 5 ms sequencer task watches keybits[] (touching Keybits) and
 * keybit_remotes[] (Keybits relayed through others) from main.c, sends a
 * HID report whenever the held mapped keys change, and emits the
 * protocol's KEY+/KEY-/PRESS/SIDES events.
 */
#ifndef KEYMAP_H
#define KEYMAP_H

#include <stddef.h>
#include <stdint.h>
#include "keybit_bus.h"
#include "keytable.h"

void KEYMAP_Init(void);

/* 0 on success (also when flash already matched), -1 on a flash error. */
int KEYMAP_Save(void);

/* Touching Keybits as of the last key task run, the set the events describe. */
const keybit_info_t *KEYMAP_Present(int *count);

/* The i-th KEY+ line for KEYS?: touching Keybits first, then the remote
 * ones that are present. 0 once i is past the end. */
int KEYMAP_KeyLine(int i, char *out, size_t n);

#endif /* KEYMAP_H */
