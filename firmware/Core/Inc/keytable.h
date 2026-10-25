/*
 * The keymap table: one keyact_t per Keybit, keyed by UID. No HAL, so the
 * host tests link the real thing (keymap.c adds flash and the key task).
 *
 * Direct Keybits are known by their full 96-bit UID. Keybits relayed
 * through others only report the low 32 bits, so an entry can also be
 * keyed by those alone (MAP uid=<8hex>). A short id and a full one match
 * when the low 32 bits agree; setting a full id on a short entry upgrades
 * it in place.
 */
#ifndef KEYTABLE_H
#define KEYTABLE_H

#include <stdint.h>
#include "keyact.h"

#define KEYMAP_MAX    32
#define UID_LEN       12
#define UID_SHORT_LEN 4

/* uid little endian; with len UID_SHORT_LEN only uid[0..3] count. */
typedef struct
{
  uint8_t uid[UID_LEN];
  uint8_t len;
} keyid_t;

/* 56 bytes, seven flash doublewords. */
typedef struct
{
  uint8_t uid[UID_LEN];
  uint8_t uid_len;
  uint8_t pad;
  keyact_t act;
} map_entry_t;

/* Also the flash image: one header doubleword, then the entries. */
typedef struct
{
  uint32_t magic;
  uint16_t version;
  uint16_t count;
  map_entry_t entries[KEYMAP_MAX];
} map_image_t;

extern map_image_t keytable;

/* A Keybit maps to a key, a consumer usage or a macro: each setter
 * replaces the others (a hold stays). 0 on success, -1 when the table is
 * full. key may be 0 (mods only). */
int KEYMAP_Set(const keyid_t *id, uint8_t mods, uint8_t key);
int KEYMAP_SetConsumer(const keyid_t *id, uint16_t usage);
/* Up to KEYACT_MACRO_MAX (mods, key) taps. */
int KEYMAP_SetMacro(const keyid_t *id, const uint8_t seq[][2], uint8_t len);
/* Hold action, see keyact.h. mods 0 and key 0 clears it. */
int KEYMAP_SetHold(const keyid_t *id, uint8_t mods, uint8_t key, uint16_t ms);
/* NULL when the Keybit has no entry. */
const keyact_t *KEYMAP_Lookup(const keyid_t *id);
void KEYMAP_Unmap(const keyid_t *id);
void KEYMAP_Clear(void);
/* Full UID of an entry whose low 32 bits are low, NULL if none. */
const uint8_t *KEYMAP_FullUid(uint32_t low);

#endif /* KEYTABLE_H */
