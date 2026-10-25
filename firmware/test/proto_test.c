/*
 * Host check for the config protocol parser in Core/Src/proto.c, with the
 * real keymap table (keytable.c), and the Keybit bus, battery and the two
 * transports (NUS, USB CDC) replaced by small fakes.
 *   cc -std=c99 -Itest/stub -ICore/Inc test/proto_test.c Core/Src/proto.c Core/Src/keytable.c -o /tmp/proto_test && /tmp/proto_test
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "keymap.h"
#include "batt.h"
#include "proto.h"

/* Two fake transports, like NUS and USB CDC. */
static char out[2048], out2[2048];
static void write1(const char *data, uint16_t len) { strncat(out, data, len); }
static void write2(const char *data, uint16_t len) { strncat(out2, data, len); }
static proto_port_t port = {.write = write1, .local = 1};   /* like USB CDC */
static proto_port_t port2 = {.write = write2};

/* The real keytable.c is linked; the bus side of keymap.c is faked. */
static keybit_info_t present[2];
static int remote_listed;
const uint8_t keybit_image_version = 3;
static int stack_ok = 1, ota_asked, dfu_asked;
int OTA_Available(void) { return stack_ok; }
void OTA_Request(void) { ota_asked++; }
void OTA_RequestDfu(void) { dfu_asked++; }
void OTA_StackInfo(char *out, size_t n) { snprintf(out, n, stack_ok ? "ble v=1.24.0 fus=2.2.0" : "none fus=2.2.0"); }
static int fus_installs;
static const char *fus_refuse;
void FUS_Info(char *out, size_t n) { snprintf(out, n, "v=2.2.0 state=idle stack=none sfsa=0xF4 image=129460"); }
const char *FUS_Install(void) { if (fus_refuse) return fus_refuse; fus_installs++; return NULL; }
int FUS_Busy(void) { return 0; }
static batt_t batt_fake;
const batt_t *BATT_Get(void) { return &batt_fake; }
int KEYMAP_Save(void) { return 0; }
const keybit_info_t *KEYMAP_Present(int *count) { *count = 2; return present; }
int KEYMAP_KeyLine(int i, char *out, size_t len)
{
  char uid[25];
  if (i < 2)
  {
    PROTO_FormatUid(uid, present[i].uid);
    snprintf(out, len, "KEY+ uid=%s at=%c v=%u", uid, present[i].at, present[i].fw);
    return 1;
  }
  if (i == 2 && remote_listed)
  {
    snprintf(out, len, "KEY+ uid=1234ABCD at=X via=%s depth=2", "0102030405060708090A0B0C");
    return 1;
  }
  return 0;
}

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

  expect("HELLO\n", "KEYWORD v=" KEYWORD_FW_VERSION " proto=1 caps=batt,hold,macro,ota kb=3\n");
  expect("HELLO\r\n", "KEYWORD v=" KEYWORD_FW_VERSION " proto=1 caps=batt,hold,macro,ota kb=3\n");
  expect("HELLO\r", "KEYWORD v=" KEYWORD_FW_VERSION " proto=1 caps=batt,hold,macro,ota kb=3\n");   /* terminal Enter */
  expect("\r\n\r\n", "");
  expect("SIDES?\n", "SIDES L=1 R=0 T=1\n");
  expect("BATT?\n", "ERR no battery reading yet\n");
  batt_fake = (batt_t){.valid = 1, .pct = 73, .chg = 1, .usb = 1, .mv = 3970};
  expect("BATT?\n", "BATT pct=73 chg=1 usb=1\n");
  expect("KEYS?\n", "KEY+ uid=" U " at=L v=2\nKEY+ uid=ABABABABABABABABABABABAB at=T v=0\nKEYS END\n");
  expect("MAP? uid=" U "\n", "MAP uid=" U " mods=00 key=00\n");
  expect("MAP uid=" U " mods=02 key=04\n", "OK MAP uid=" U "\n");
  assert(keytable.entries[0].uid[0] == 0x0C && keytable.entries[0].uid[11] == 0x01);   /* stored little endian */
  expect("MAP? uid=0102030405060708090a0b0c\n", "MAP uid=" U " mods=02 key=04\n");
  expect("MAP uid=" U " mods=01 key=00\n", "OK MAP uid=" U "\n");
  expect("MAP? uid=" U "\n", "MAP uid=" U " mods=01 key=00\n");
  expect("UNMAP uid=" U "\n", "OK UNMAP uid=" U "\n");
  expect("MAP? uid=" U "\n", "MAP uid=" U " mods=00 key=00\n");

  /* consumer keys: MAPC and MAP replace each other */
  expect("MAPC uid=" U " usage=00CD\n", "OK MAPC uid=" U "\n");
  assert(keytable.entries[0].act.kind == KEYACT_CONSUMER && keytable.entries[0].act.code == 0x00CD);
  expect("MAP? uid=" U "\n", "MAPC uid=" U " usage=00CD\n");
  expect("MAP uid=" U " mods=00 key=04\n", "OK MAP uid=" U "\n");
  expect("MAP? uid=" U "\n", "MAP uid=" U " mods=00 key=04\n");
  expect("MAPC uid=" U " usage=00e9\n", "OK MAPC uid=" U "\n");
  expect("MAP? uid=" U "\n", "MAPC uid=" U " usage=00E9\n");
  assert(keytable.count == 1);
  expect("UNMAP uid=" U "\n", "OK UNMAP uid=" U "\n");
  expect("MAP? uid=" U "\n", "MAP uid=" U " mods=00 key=00\n");
  expect("MAPC uid=" U " usage=E2\n", "ERR usage: MAPC uid=<24hex> usage=<4hex>\n");
  expect("MAPC uid=" U "\n", "ERR usage: MAPC uid=<24hex> usage=<4hex>\n");
  expect("MAPC uid=" U " usage=00E2\n", "OK MAPC uid=" U "\n");
  expect("CLEAR\n", "OK CLEAR\n");
  assert(keytable.count == 0);

  /* Relayed Keybits: 8 hex digits, the low 32 bits of the UID */
  remote_listed = 1;
  expect("KEYS?\n", "KEY+ uid=" U " at=L v=2\nKEY+ uid=ABABABABABABABABABABABAB at=T v=0\n"
                     "KEY+ uid=1234ABCD at=X via=" U " depth=2\nKEYS END\n");
  remote_listed = 0;
  expect("MAP uid=090a0b0c mods=00 key=04\n", "OK MAP uid=090A0B0C\n");
  assert(keytable.count == 1 && keytable.entries[0].uid_len == UID_SHORT_LEN && keytable.entries[0].uid[0] == 0x0C);
  expect("MAP? uid=090A0B0C\n", "MAP uid=090A0B0C mods=00 key=04\n");
  /* U's low 32 bits are 090A0B0C: the full UID finds the short entry */
  expect("MAP? uid=" U "\n", "MAP uid=" U " mods=00 key=04\n");
  assert(KEYMAP_FullUid(0x090A0B0C) == NULL);
  /* and mapping it in full upgrades that entry instead of adding one */
  expect("MAP uid=" U " mods=00 key=05\n", "OK MAP uid=" U "\n");
  assert(keytable.count == 1 && keytable.entries[0].uid_len == UID_LEN);
  assert(KEYMAP_FullUid(0x090A0B0C) == keytable.entries[0].uid);
  expect("MAP? uid=090A0B0C\n", "MAP uid=090A0B0C mods=00 key=05\n");   /* short still finds it */
  /* another full UID with the same low 32 bits is a different Keybit */
  expect("MAP? uid=FFFFFFFFFFFFFFFF090A0B0C\n", "MAP uid=FFFFFFFFFFFFFFFF090A0B0C mods=00 key=00\n");
  expect("MAPC uid=090A0B0C usage=00CD\n", "OK MAPC uid=090A0B0C\n");   /* edits the same entry */
  expect("MAP? uid=" U "\n", "MAPC uid=" U " usage=00CD\n");
  expect("UNMAP uid=090A0B0C\n", "OK UNMAP uid=090A0B0C\n");
  assert(keytable.count == 0);
  expect("HOLD uid=1234ABCD mods=02 key=00\n", "OK HOLD uid=1234ABCD\n");
  expect("MACRO uid=1234ABCD seq=0004\n", "OK MACRO uid=1234ABCD\n");
  expect("MAP? uid=1234ABCD\n", "MACRO uid=1234ABCD seq=0004\nHOLD uid=1234ABCD mods=02 key=00 ms=200\n");
  expect("MAP uid=1234ABCDEF mods=00 key=04\n", "ERR usage: MAP uid=<24hex> mods=<2hex> key=<2hex>\n");
  expect("CLEAR\n", "OK CLEAR\n");

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
  assert(keytable.count == 0);   /* a hold-only entry goes away when cleared */
  expect("HOLD uid=" U " mods=02 key=00\n", "OK HOLD uid=" U "\n");
  expect("CLEAR\n", "OK CLEAR\n");
  assert(keytable.count == 0);

  /* MACRO: replaces MAP/MAPC, MAP? prints it back, UNMAP clears it */
  expect("MACRO uid=" U " seq=020B,0008,000F,000F,0012\n", "OK MACRO uid=" U "\n");   /* "Hello" */
  assert(keytable.entries[0].act.kind == KEYACT_MACRO && keytable.entries[0].act.macro_len == 5);
  assert(keytable.entries[0].act.macro[0][0] == 0x02 && keytable.entries[0].act.macro[0][1] == 0x0B);
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

  keytable.count = KEYMAP_MAX;
  expect("MAP uid=FFFFFFFFFFFFFFFFFFFFFFFF mods=00 key=04\n", "ERR keymap full\n");
  expect("MAPC uid=FFFFFFFFFFFFFFFFFFFFFFFF usage=00CD\n", "ERR keymap full\n");

  char longline[300];
  memset(longline, 'A', sizeof(longline) - 2);
  longline[sizeof(longline) - 2] = '\n';
  longline[sizeof(longline) - 1] = 0;
  expect(longline, "ERR line too long\n");
  expect("HELLO\n", "KEYWORD v=" KEYWORD_FW_VERSION " proto=1 caps=batt,hold,macro,ota kb=3\n");

  /* OTA / DFU / STACK? */
  expect("STACK?\n", "STACK ble v=1.24.0 fus=2.2.0\n");
  expect("OTA\n", "OK OTA\n");
  assert(ota_asked == 1);
  expect("DFU\n", "OK DFU\n");
  assert(dfu_asked == 1);
  /* FUS? and STACK INSTALL (USB only) */
  expect("FUS?\n", "FUS v=2.2.0 state=idle stack=none sfsa=0xF4 image=129460\n");
  expect("STACK INSTALL\n", "OK STACK INSTALL\n");
  assert(fus_installs == 1);
  fus_refuse = "no image at 0x080D0000";
  expect("STACK INSTALL\n", "ERR no image at 0x080D0000\n");
  fus_refuse = NULL;
  expect("STACK BOGUS\n", "ERR unknown command\n");
  out2[0] = 0;
  PROTO_Input(&port2, (const uint8_t *)"STACK INSTALL\n", 14);   /* port2 is like NUS */
  assert(strcmp(out2, "ERR STACK INSTALL is USB only\n") == 0 && fus_installs == 1);
  out2[0] = 0;
  PROTO_Input(&port2, (const uint8_t *)"FUS?\n", 5);              /* reading is fine anywhere */
  assert(strncmp(out2, "FUS v=2.2.0", 11) == 0);

  stack_ok = 0;   /* FUS only, no BLE stack: no OTA, and HELLO says so */
  expect("SAVE\n", "ERR no BLE stack yet, keymap not saved\n");
  expect("HELLO\n", "KEYWORD v=" KEYWORD_FW_VERSION " proto=1 caps=batt,hold,macro kb=3\n");
  expect("STACK?\n", "STACK none fus=2.2.0\n");
  expect("OTA\n", "ERR no BLE stack on CPU2, use DFU\n");
  assert(ota_asked == 1);
  expect("DFU\n", "OK DFU\n");   /* DFU still works */
  assert(dfu_asked == 2);
  stack_ok = 1;

  /* Two transports at once: each has its own line buffer, so half a line
   * on one doesn't mix with the other, and replies go back where they
   * came from. */
  out[0] = out2[0] = 0;
  PROTO_Input(&port, (const uint8_t *)"HEL", 3);
  PROTO_Input(&port2, (const uint8_t *)"SIDES?\n", 7);
  assert(out[0] == 0);
  assert(strcmp(out2, "SIDES L=1 R=0 T=1\n") == 0);
  PROTO_Input(&port, (const uint8_t *)"LO\n", 3);
  assert(strcmp(out, "KEYWORD v=" KEYWORD_FW_VERSION " proto=1 caps=batt,hold,macro,ota kb=3\n") == 0);
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
