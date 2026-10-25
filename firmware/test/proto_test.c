/*
 * Host check for the config protocol parser in Core/Src/proto.c, with the
 * keymap and the two transports (NUS, USB CDC) replaced by small fakes.
 *   cc -std=c99 -Itest/stub -ICore/Inc test/proto_test.c Core/Src/proto.c -o /tmp/proto_test && /tmp/proto_test
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "keymap.h"
#include "proto.h"

/* Two fake transports, like NUS and USB CDC. */
static char out[2048], out2[2048];
static void write1(const char *data, uint16_t len) { strncat(out, data, len); }
static void write2(const char *data, uint16_t len) { strncat(out2, data, len); }
static proto_port_t port = {.write = write1};
static proto_port_t port2 = {.write = write2};

static struct { uint8_t uid[UID_LEN]; keyact_t act; } table[KEYMAP_MAX];
static int n;
static keybit_info_t present[2];

static int find(const uint8_t *uid)
{
  for (int i = 0; i < n; i++) if (!memcmp(table[i].uid, uid, UID_LEN)) return i;
  return -1;
}
static keyact_t *entry(const uint8_t *uid)
{
  int i = find(uid);
  if (i < 0) { if (n == KEYMAP_MAX) return NULL; i = n++; memset(&table[i], 0, sizeof(table[i])); memcpy(table[i].uid, uid, UID_LEN); }
  return &table[i].act;
}
static int put(const uint8_t *uid, uint8_t kind, uint8_t mods, uint16_t code)
{
  keyact_t *a = entry(uid);
  if (!a) return -1;
  a->kind = kind; a->mods = mods; a->code = code; a->macro_len = 0;
  return 0;
}
int KEYMAP_Set(const uint8_t *uid, uint8_t mods, uint8_t key) { return put(uid, KEYACT_KEY, mods, key); }
int KEYMAP_SetConsumer(const uint8_t *uid, uint16_t usage) { return put(uid, KEYACT_CONSUMER, 0, usage); }
void KEYMAP_Unmap(const uint8_t *uid);
int KEYMAP_SetHold(const uint8_t *uid, uint8_t mods, uint8_t key, uint16_t ms)
{
  if (!mods && !key)
  {
    int i = find(uid);
    if (i < 0) return 0;
    table[i].act.hold_mods = table[i].act.hold_key = 0; table[i].act.hold_ms = 0;
    if (table[i].act.kind == KEYACT_NONE) KEYMAP_Unmap(uid);
    return 0;
  }
  keyact_t *a = entry(uid);
  if (!a) return -1;
  a->hold_mods = mods; a->hold_key = key; a->hold_ms = ms;
  return 0;
}
int KEYMAP_SetMacro(const uint8_t *uid, const uint8_t seq[][2], uint8_t len)
{
  keyact_t *a = entry(uid);
  if (!a) return -1;
  a->kind = KEYACT_MACRO; a->mods = 0; a->code = 0; a->macro_len = len;
  memcpy(a->macro, seq, (size_t)len * 2);
  return 0;
}
const keyact_t *KEYMAP_Lookup(const uint8_t *uid) { int i = find(uid); return i < 0 ? NULL : &table[i].act; }
void KEYMAP_Unmap(const uint8_t *uid) { int i = find(uid); if (i >= 0) table[i] = table[--n]; }
void KEYMAP_Clear(void) { n = 0; }
int KEYMAP_Save(void) { return 0; }
const keybit_info_t *KEYMAP_Present(int *count) { *count = 2; return present; }

static void expect(const char *in, const char *want)
{
  out[0] = out2[0] = 0;
  PROTO_Input(&port, (const uint8_t *)in, (uint16_t)strlen(in));
  if (strcmp(out, want) != 0) { printf("in:   %s\nwant: %s\ngot:  %s\n", in, want, out); assert(0); }
  assert(out2[0] == 0);   /* replies only go to the port that asked */
}

#define U  "0102030405060708090A0B0C"

int main(void)
{
  PROTO_AddPort(&port);
  PROTO_AddPort(&port2);
  for (int i = 0; i < UID_LEN; i++) present[0].uid[i] = (uint8_t)(12 - i);   /* prints as U */
  present[0].at = 'L';
  present[0].fw = 2;
  memset(present[1].uid, 0xAB, UID_LEN);
  present[1].at = 'T';

  expect("HELLO\n", "KEYWORD v=" KEYWORD_FW_VERSION " proto=1\n");
  expect("HELLO\r\n", "KEYWORD v=" KEYWORD_FW_VERSION " proto=1\n");
  expect("HELLO\r", "KEYWORD v=" KEYWORD_FW_VERSION " proto=1\n");   /* terminal Enter */
  expect("\r\n\r\n", "");
  expect("SIDES?\n", "SIDES L=1 R=0 T=1\n");
  expect("KEYS?\n", "KEY+ uid=" U " at=L v=2\nKEY+ uid=ABABABABABABABABABABABAB at=T v=0\nKEYS END\n");
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
  assert(table[0].act.kind == KEYACT_CONSUMER && table[0].act.code == 0x00CD);
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

  /* HOLD: tap does the MAP action, hold does this one */
  expect("HOLD uid=" U " mods=02 key=00 ms=250\n", "OK HOLD uid=" U "\n");
  expect("MAP? uid=" U "\n", "MAP uid=" U " mods=00 key=00\nHOLD uid=" U " mods=02 key=00 ms=250\n");
  expect("MAP uid=" U " mods=00 key=29\n", "OK MAP uid=" U "\n");   /* MAP keeps the hold */
  expect("MAP? uid=" U "\n", "MAP uid=" U " mods=00 key=29\nHOLD uid=" U " mods=02 key=00 ms=250\n");
  expect("HOLD uid=" U " mods=00 key=E0\n", "OK HOLD uid=" U "\n");   /* ms defaults to 200 */
  expect("MAP? uid=" U "\n", "MAP uid=" U " mods=00 key=29\nHOLD uid=" U " mods=00 key=E0 ms=200\n");
  expect("HOLD uid=" U " mods=00 key=00\n", "OK HOLD uid=" U "\n");   /* clears it */
  expect("MAP? uid=" U "\n", "MAP uid=" U " mods=00 key=29\n");
  expect("HOLD uid=" U " mods=02 key=00 ms=70000\n", "ERR usage: HOLD uid=<24hex> mods=<2hex> key=<2hex> ms=<dec>\n");
  expect("HOLD uid=" U " mods=02 key=00 ms=\n", "ERR usage: HOLD uid=<24hex> mods=<2hex> key=<2hex> ms=<dec>\n");
  expect("HOLD uid=" U " key=04\n", "ERR usage: HOLD uid=<24hex> mods=<2hex> key=<2hex> ms=<dec>\n");
  expect("HOLD uid=" U " mods=02 key=00 ms=300\n", "OK HOLD uid=" U "\n");
  expect("UNMAP uid=" U "\n", "OK UNMAP uid=" U "\n");   /* UNMAP drops the hold too */
  expect("MAP? uid=" U "\n", "MAP uid=" U " mods=00 key=00\n");
  expect("HOLD uid=" U " mods=02 key=00\n", "OK HOLD uid=" U "\n");
  expect("HOLD uid=" U " mods=00 key=00\n", "OK HOLD uid=" U "\n");
  assert(n == 0);   /* a hold-only entry goes away when cleared */
  expect("HOLD uid=" U " mods=02 key=00\n", "OK HOLD uid=" U "\n");
  expect("CLEAR\n", "OK CLEAR\n");
  assert(n == 0);

  /* MACRO: replaces MAP/MAPC, MAP? prints it back, UNMAP clears it */
  expect("MACRO uid=" U " seq=020B,0008,000F,000F,0012\n", "OK MACRO uid=" U "\n");   /* "Hello" */
  assert(table[0].act.kind == KEYACT_MACRO && table[0].act.macro_len == 5);
  assert(table[0].act.macro[0][0] == 0x02 && table[0].act.macro[0][1] == 0x0B);
  expect("MAP? uid=" U "\n", "MACRO uid=" U " seq=020B,0008,000F,000F,0012\n");
  expect("MACRO uid=" U " seq=0004\n", "OK MACRO uid=" U "\n");
  expect("MAP? uid=" U "\n", "MACRO uid=" U " seq=0004\n");
  expect("MAP uid=" U " mods=00 key=05\n", "OK MAP uid=" U "\n");   /* MAP replaces it */
  expect("MAP? uid=" U "\n", "MAP uid=" U " mods=00 key=05\n");
  expect("MACRO uid=" U " seq=0004,0005\n", "OK MACRO uid=" U "\n");
  expect("MAPC uid=" U " usage=00CD\n", "OK MAPC uid=" U "\n");   /* so does MAPC */
  expect("MAP? uid=" U "\n", "MAPC uid=" U " usage=00CD\n");
  expect("MACRO uid=" U " seq=0004,0005\n", "OK MACRO uid=" U "\n");
  expect("UNMAP uid=" U "\n", "OK UNMAP uid=" U "\n");
  expect("MAP? uid=" U "\n", "MAP uid=" U " mods=00 key=00\n");
  /* all 16 taps, then a seventeenth is too many */
  expect("MACRO uid=" U " seq=0004,0005,0006,0007,0008,0009,000A,000B,000C,000D,000E,000F,0010,0011,0012,0013\n",
         "OK MACRO uid=" U "\n");
  expect("MAP? uid=" U "\n",
         "MACRO uid=" U " seq=0004,0005,0006,0007,0008,0009,000A,000B,000C,000D,000E,000F,0010,0011,0012,0013\n");
  expect("MACRO uid=" U " seq=0004,0005,0006,0007,0008,0009,000A,000B,000C,000D,000E,000F,0010,0011,0012,0013,0014\n",
         "ERR usage: MACRO uid=<24hex> seq=<MMKK,...> (1-16 taps)\n");
  expect("MACRO uid=" U " seq=\n", "ERR usage: MACRO uid=<24hex> seq=<MMKK,...> (1-16 taps)\n");
  expect("MACRO uid=" U " seq=004\n", "ERR usage: MACRO uid=<24hex> seq=<MMKK,...> (1-16 taps)\n");
  expect("MACRO uid=" U " seq=0004,\n", "ERR usage: MACRO uid=<24hex> seq=<MMKK,...> (1-16 taps)\n");
  expect("MACRO uid=" U " seq=0004;0005\n", "ERR usage: MACRO uid=<24hex> seq=<MMKK,...> (1-16 taps)\n");
  expect("CLEAR\n", "OK CLEAR\n");
  expect("SAVE\n", "OK SAVE\n");

  /* chunked like 20 byte BLE writes */
  out[0] = 0;
  PROTO_Input(&port, (const uint8_t *)"MAP uid=01020304050", 19);
  assert(out[0] == 0);
  PROTO_Input(&port, (const uint8_t *)"60708090A0B0C mods=", 19);
  PROTO_Input(&port, (const uint8_t *)"00 key=1E\n", 10);
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

  /* Two transports at once: each has its own line buffer, so half a line
   * on one doesn't mix with the other, and replies go back where they
   * came from. */
  out[0] = out2[0] = 0;
  PROTO_Input(&port, (const uint8_t *)"HEL", 3);
  PROTO_Input(&port2, (const uint8_t *)"SIDES?\n", 7);
  assert(out[0] == 0);
  assert(strcmp(out2, "SIDES L=1 R=0 T=1\n") == 0);
  PROTO_Input(&port, (const uint8_t *)"LO\n", 3);
  assert(strcmp(out, "KEYWORD v=" KEYWORD_FW_VERSION " proto=1\n") == 0);
  assert(strcmp(out2, "SIDES L=1 R=0 T=1\n") == 0);

  /* Unsolicited events go to every connected port, and only those. */
  port.connected = 0;
  port2.connected = 0;
  out[0] = out2[0] = 0;
  PROTO_Send("PRESS uid=%s down=%d", U, 1);
  assert(out[0] == 0 && out2[0] == 0);
  port2.connected = 1;   /* e.g. DTR set on CDC */
  PROTO_Send("PRESS uid=%s down=%d", U, 1);
  assert(out[0] == 0 && strcmp(out2, "PRESS uid=" U " down=1\n") == 0);
  out2[0] = 0;
  PROTO_Input(&port, (const uint8_t *)"HELLO\n", 6);   /* a line marks the port connected */
  assert(port.connected);
  out[0] = 0;
  PROTO_Send("KEY- uid=%s", U);
  assert(strcmp(out, "KEY- uid=" U "\n") == 0 && strcmp(out2, "KEY- uid=" U "\n") == 0);

  puts("proto_test: ok");
  return 0;
}
