/*
 * Host check for the keymap's flash image in Core/Src/keytable.c: packing,
 * loading a version 5 map, and the page limit.
 *   cc -std=c99 -ICore/Inc test/keytable_test.c Core/Src/keytable.c -o /tmp/keytable_test && /tmp/keytable_test
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "keytable.h"

static uint8_t page[KEYMAP_PAGE];

static keyid_t Id(uint8_t n)
{
  keyid_t id = {{n, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}, UID_LEN};
  return id;
}

int main(void)
{
  uint8_t seq[KEYACT_MACRO_MAX][2];
  for (int i = 0; i < KEYACT_MACRO_MAX; i++) { seq[i][0] = (uint8_t)i; seq[i][1] = (uint8_t)(4 + i); }

  /* round trip: a plain key packs to 24 bytes, a 64-tap macro to 152 */
  keyid_t a = Id(1), b = Id(2);
  assert(KEYMAP_Set(&a, 0x02, 0x04) == 0);
  assert(KEYMAP_SetMacro(&b, (const uint8_t (*)[2])seq, KEYACT_MACRO_MAX) == 0);
  keytable.magic = 0x50414D4B;
  keytable.version = KEYMAP_VERSION;
  uint32_t n = KEYMAP_Pack(page);
  assert(n == 8 + 24 + 152);
  map_image_t back;
  memset(&back, 0xAA, sizeof(back));
  assert(KEYMAP_Unpack(page, sizeof(page), &back) == n);
  assert(back.count == 2 && back.entries[0].act.code == 0x04 && back.entries[0].act.mods == 0x02);
  assert(back.entries[1].act.macro_len == KEYACT_MACRO_MAX && back.entries[1].act.macro[63][1] == 4 + 63);
  assert(KEYMAP_Unpack(page, n - 1, NULL) == 0);   /* cut off */

  /* a version 5 map: 56 byte entries, 16 taps, same layout up to there */
  memset(page, 0, sizeof(page));
  map_image_t v5 = {.magic = 0x50414D4B, .version = 5, .count = 2};
  memcpy(page, &v5, 8);
  map_entry_t e;
  memset(&e, 0, sizeof(e));
  e.uid[0] = 7; e.uid_len = UID_LEN; e.act.kind = KEYACT_MACRO; e.act.macro_len = 16;
  memcpy(e.act.macro, seq, 32);
  memcpy(page + 8, &e, 56);
  e.uid[0] = 8; e.act.kind = KEYACT_KEY; e.act.code = 0x2C; e.act.macro_len = 0; e.act.hold_key = 0xE1; e.act.hold_ms = 300;
  memset(e.act.macro, 0, sizeof(e.act.macro));
  memcpy(page + 64, &e, 56);
  assert(KEYMAP_Unpack(page, sizeof(page), &keytable) == 8 + 2 * 56);
  assert(keytable.count == 2 && keytable.entries[0].uid[0] == 7 && keytable.entries[0].act.macro_len == 16);
  assert(keytable.entries[0].act.macro[15][1] == 4 + 15 && keytable.entries[0].act.macro[16][1] == 0);
  assert(keytable.entries[1].act.code == 0x2C && keytable.entries[1].act.hold_key == 0xE1 && keytable.entries[1].act.hold_ms == 300);
  page[8 + 22] = 17;   /* more taps than version 5 had */
  assert(KEYMAP_Unpack(page, sizeof(page), NULL) == 0);

  /* the packed map plus header and commit never outgrows the page */
  KEYMAP_Clear();
  int macros = 0;
  for (uint8_t i = 0; i < KEYMAP_MAX; i++)
  {
    keyid_t id = Id(i);
    if (KEYMAP_SetMacro(&id, (const uint8_t (*)[2])seq, KEYACT_MACRO_MAX) == 0) macros++;
  }
  assert(macros == (KEYMAP_PAGE - 16) / 152);   /* 26 */
  assert(KEYMAP_Pack(page) + 8 <= KEYMAP_PAGE);
  keyid_t first = Id(0);
  assert(KEYMAP_SetMacro(&first, (const uint8_t (*)[2])seq, 2) == 0);   /* shrinking always fits */
  keyid_t extra = Id(100);
  assert(KEYMAP_Set(&extra, 0, 0x04) == 0);   /* 24 bytes freed, one plain key fits */
  printf("keytable_test: ok\n");
  return 0;
}
