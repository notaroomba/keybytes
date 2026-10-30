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
#define KEYMAP_PAGE   4096   /* the flash page the keymap is saved in */
/* Saved keymap format. 2: kind + code, 3: hold, 4: macros, 5: short
 * (32-bit) UIDs, 6: entries cut after their last macro tap. */
#define KEYMAP_VERSION 6
#define UID_LEN       12
#define UID_SHORT_LEN 4

/* uid little endian; with len UID_SHORT_LEN only uid[0..3] count. */
typedef struct
{
  uint8_t uid[UID_LEN];
  uint8_t len;
} keyid_t;

/* Saved as its first bytes, through macro[macro_len), zero padded to
 * doublewords: 24 bytes for a plain key, 152 for a 64-tap macro. Version 5
 * saved all of it with 16 taps, 56 bytes, the same layout up to there. */
typedef struct
{
  uint8_t uid[UID_LEN];
  uint8_t uid_len;
  uint8_t pad;
  keyact_t act;
} map_entry_t;

/* The header is also the flash image's first doubleword, then come the
 * packed entries. */
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
 * full or its flash image would outgrow KEYMAP_PAGE. key may be 0 (mods
 * only). */
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

/* The keytable as its flash image (header from keytable as is) into out,
 * which holds KEYMAP_PAGE bytes. Returns its size. */
uint32_t KEYMAP_Pack(uint8_t *out);
/* Size of a version 5 or 6 image at src that ends within max bytes, 0 if
 * it doesn't parse. With out, also loads it there. */
uint32_t KEYMAP_Unpack(const uint8_t *src, uint32_t max, map_image_t *out);

#endif /* KEYTABLE_H */
