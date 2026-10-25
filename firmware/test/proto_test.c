/*
 * Host check for the config protocol parser in Core/Src/proto.c, with the
 * keymap and NUS replaced by small fakes.
 *   cc -std=c99 -Itest/stub -ICore/Inc -ISTM32_WPAN/App test/proto_test.c Core/Src/proto.c -o /tmp/proto_test && /tmp/proto_test
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "keymap.h"
#include "nus.h"
#include "proto.h"

static char out[2048];

void NUS_Write(const char *data, uint16_t len) { strncat(out, data, len); }

static struct { uint8_t uid[UID_LEN], kind, mods; uint16_t code; } table[KEYMAP_MAX];
static int n;
static keybit_info_t present[2];

static int find(const uint8_t *uid)
{
  for (int i = 0; i < n; i++) if (!memcmp(table[i].uid, uid, UID_LEN)) return i;
  return -1;
}
static int put(const uint8_t *uid, uint8_t kind, uint8_t mods, uint16_t code)
{
  int i = find(uid);
  if (i < 0) { if (n == KEYMAP_MAX) return -1; i = n++; memcpy(table[i].uid, uid, UID_LEN); }
  table[i].kind = kind; table[i].mods = mods; table[i].code = code;
  return 0;
}
int KEYMAP_Set(const uint8_t *uid, uint8_t mods, uint8_t key) { return put(uid, KEYMAP_KEY, mods, key); }
int KEYMAP_SetConsumer(const uint8_t *uid, uint16_t usage) { return put(uid, KEYMAP_CONSUMER, 0, usage); }
int KEYMAP_Get(const uint8_t *uid, uint8_t *mods, uint16_t *code)
{
  int i = find(uid);
  *mods = i < 0 ? 0 : table[i].mods; *code = i < 0 ? 0 : table[i].code;
  return i < 0 ? KEYMAP_NONE : table[i].kind;
}
void KEYMAP_Unmap(const uint8_t *uid) { int i = find(uid); if (i >= 0) table[i] = table[--n]; }
void KEYMAP_Clear(void) { n = 0; }
int KEYMAP_Save(void) { return 0; }
const keybit_info_t *KEYMAP_Present(int *count) { *count = 2; return present; }

static void expect(const char *in, const char *want)
{
  out[0] = 0;
  PROTO_Input((const uint8_t *)in, (uint16_t)strlen(in));
  if (strcmp(out, want) != 0) { printf("in:   %s\nwant: %s\ngot:  %s\n", in, want, out); assert(0); }
}

#define U  "0102030405060708090A0B0C"

int main(void)
{
  for (int i = 0; i < UID_LEN; i++) present[0].uid[i] = (uint8_t)(12 - i);   /* prints as U */
  present[0].at = 'L';
  memset(present[1].uid, 0xAB, UID_LEN);
  present[1].at = 'T';

  expect("HELLO\n", "KEYWORD v=" KEYWORD_FW_VERSION " proto=1\n");
  expect("HELLO\r\n", "KEYWORD v=" KEYWORD_FW_VERSION " proto=1\n");
  expect("SIDES?\n", "SIDES L=1 R=0 T=1\n");
  expect("KEYS?\n", "KEY+ uid=" U " at=L\nKEY+ uid=ABABABABABABABABABABABAB at=T\nKEYS END\n");
  expect("MAP? uid=" U "\n", "MAP uid=" U " mods=00 key=00\n");
  expect("MAP uid=" U " mods=02 key=04\n", "OK MAP uid=" U "\n");
  assert(table[0].uid[0] == 0x0C && table[0].uid[11] == 0x01);   /* stored little endian */
  expect("MAP? uid=0102030405060708090a0b0c\n", "MAP uid=" U " mods=02 key=04\n");
  expect("MAP uid=" U " mods=01 key=00\n", "OK MAP uid=" U "\n");
  expect("MAP? uid=" U "\n", "MAP uid=" U " mods=01 key=00\n");
  expect("UNMAP uid=" U "\n", "OK UNMAP uid=" U "\n");
  expect("MAP? uid=" U "\n", "MAP uid=" U " mods=00 key=00\n");

  /* consumer keys: MAPC and MAP replace each other */
  expect("MAPC uid=" U " usage=00CD\n", "OK MAPC uid=" U "\n");
  assert(table[0].kind == KEYMAP_CONSUMER && table[0].code == 0x00CD);
  expect("MAP? uid=" U "\n", "MAPC uid=" U " usage=00CD\n");
  expect("MAP uid=" U " mods=00 key=04\n", "OK MAP uid=" U "\n");
  expect("MAP? uid=" U "\n", "MAP uid=" U " mods=00 key=04\n");
  expect("MAPC uid=" U " usage=00e9\n", "OK MAPC uid=" U "\n");
  expect("MAP? uid=" U "\n", "MAPC uid=" U " usage=00E9\n");
  assert(n == 1);
  expect("UNMAP uid=" U "\n", "OK UNMAP uid=" U "\n");
  expect("MAP? uid=" U "\n", "MAP uid=" U " mods=00 key=00\n");
  expect("MAPC uid=" U " usage=E2\n", "ERR usage: MAPC uid=<24hex> usage=<4hex>\n");
  expect("MAPC uid=" U "\n", "ERR usage: MAPC uid=<24hex> usage=<4hex>\n");
  expect("MAPC uid=" U " usage=00E2\n", "OK MAPC uid=" U "\n");
  expect("CLEAR\n", "OK CLEAR\n");
  assert(n == 0);
  expect("SAVE\n", "OK SAVE\n");

  /* chunked like 20 byte BLE writes */
  out[0] = 0;
  PROTO_Input((const uint8_t *)"MAP uid=01020304050", 19);
  assert(out[0] == 0);
  PROTO_Input((const uint8_t *)"60708090A0B0C mods=", 19);
  PROTO_Input((const uint8_t *)"00 key=1E\n", 10);
  assert(strcmp(out, "OK MAP uid=" U "\n") == 0);

  expect("MAP uid=0102 mods=00 key=04\n", "ERR usage: MAP uid=<24hex> mods=<2hex> key=<2hex>\n");
  expect("MAP uid=" U " mods=0 key=04\n", "ERR usage: MAP uid=<24hex> mods=<2hex> key=<2hex>\n");
  expect("MAP uid=" U "\n", "ERR usage: MAP uid=<24hex> mods=<2hex> key=<2hex>\n");
  expect("BOGUS\n", "ERR unknown command\n");
  expect("\n", "");

  n = KEYMAP_MAX;
  expect("MAP uid=FFFFFFFFFFFFFFFFFFFFFFFF mods=00 key=04\n", "ERR keymap full\n");
  expect("MAPC uid=FFFFFFFFFFFFFFFFFFFFFFFF usage=00CD\n", "ERR keymap full\n");

  char longline[300];
  memset(longline, 'A', sizeof(longline) - 2);
  longline[sizeof(longline) - 2] = '\n';
  longline[sizeof(longline) - 1] = 0;
  expect(longline, "ERR line too long\n");
  expect("HELLO\n", "KEYWORD v=" KEYWORD_FW_VERSION " proto=1\n");

  puts("proto_test: ok");
  return 0;
}
