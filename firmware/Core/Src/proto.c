#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "keymap.h"
#include "nus.h"
#include "proto.h"

#define LINE_MAX 128

static char line[LINE_MAX];
static uint16_t line_len;
static uint8_t line_overflow;

void PROTO_Send(const char *fmt, ...)
{
  char out[LINE_MAX];
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(out, sizeof(out) - 1, fmt, ap);
  va_end(ap);
  if (n < 0) return;
  if (n > (int)sizeof(out) - 2) n = sizeof(out) - 2;
  out[n++] = '\n';
  NUS_Write(out, (uint16_t)n);   /* add USB CDC here once there is one */
}

void PROTO_FormatUid(char *out, const uint8_t uid[UID_LEN])
{
  static const char hex[] = "0123456789ABCDEF";
  for (int i = 0; i < UID_LEN; i++)
  {
    uint8_t b = uid[UID_LEN - 1 - i];
    out[2 * i] = hex[b >> 4];
    out[2 * i + 1] = hex[b & 0xF];
  }
  out[2 * UID_LEN] = 0;
}

static int Hex_Digit(char c)
{
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

/* Finds "name=" in args and parses exactly digits hex chars after it into
 * out, most significant first. Returns 0 on success. */
static int Arg_Hex(const char *args, const char *name, int digits, uint8_t *out)
{
  size_t nl = strlen(name);
  for (const char *p = args; (p = strstr(p, name)) != NULL; p += nl)
  {
    if ((p != args && p[-1] != ' ') || p[nl] != '=') continue;
    const char *v = p + nl + 1;
    for (int i = 0; i < digits; i++)
    {
      if (Hex_Digit(v[i]) < 0) return -1;
    }
    if (v[digits] != 0 && v[digits] != ' ') return -1;
    for (int i = 0; i < digits / 2; i++)
    {
      out[i] = (uint8_t)(Hex_Digit(v[2 * i]) << 4 | Hex_Digit(v[2 * i + 1]));
    }
    return 0;
  }
  return -1;
}

/* uid= is printed most significant byte first, stored little endian. */
static int Arg_Uid(const char *args, uint8_t uid[UID_LEN])
{
  uint8_t be[UID_LEN];
  if (Arg_Hex(args, "uid", UID_LEN * 2, be) != 0) return -1;
  for (int i = 0; i < UID_LEN; i++) uid[i] = be[UID_LEN - 1 - i];
  return 0;
}

static void Handle_Line(char *cmd)
{
  char uid_s[UID_LEN * 2 + 1];
  uint8_t uid[UID_LEN];
  char *args = strchr(cmd, ' ');
  if (args) *args++ = 0;
  else args = cmd + strlen(cmd);

  if (strcmp(cmd, "HELLO") == 0)
  {
    PROTO_Send("KEYWORD v=%s proto=1", KEYWORD_FW_VERSION);
  }
  else if (strcmp(cmd, "SIDES?") == 0)
  {
    int n;
    const keybit_info_t *k = KEYMAP_Present(&n);
    int l = 0, r = 0, t = 0;
    for (int i = 0; i < n; i++)
    {
      l |= k[i].at == 'L';
      r |= k[i].at == 'R';
      t |= k[i].at == 'T';
    }
    PROTO_Send("SIDES L=%d R=%d T=%d", l, r, t);
  }
  else if (strcmp(cmd, "KEYS?") == 0)
  {
    int n;
    const keybit_info_t *k = KEYMAP_Present(&n);
    for (int i = 0; i < n; i++)
    {
      PROTO_FormatUid(uid_s, k[i].uid);
      PROTO_Send("KEY+ uid=%s at=%c", uid_s, k[i].at);
    }
    PROTO_Send("KEYS END");
  }
  else if (strcmp(cmd, "MAP") == 0)
  {
    uint8_t mods, key;
    if (Arg_Uid(args, uid) || Arg_Hex(args, "mods", 2, &mods) || Arg_Hex(args, "key", 2, &key))
    {
      PROTO_Send("ERR usage: MAP uid=<24hex> mods=<2hex> key=<2hex>");
      return;
    }
    if (KEYMAP_Set(uid, mods, key) != 0)
    {
      PROTO_Send("ERR keymap full");
      return;
    }
    PROTO_FormatUid(uid_s, uid);
    PROTO_Send("OK MAP uid=%s", uid_s);
  }
  else if (strcmp(cmd, "MAPC") == 0)
  {
    uint8_t usage[2];
    if (Arg_Uid(args, uid) || Arg_Hex(args, "usage", 4, usage))
    {
      PROTO_Send("ERR usage: MAPC uid=<24hex> usage=<4hex>");
      return;
    }
    if (KEYMAP_SetConsumer(uid, (uint16_t)(usage[0] << 8 | usage[1])) != 0)
    {
      PROTO_Send("ERR keymap full");
      return;
    }
    PROTO_FormatUid(uid_s, uid);
    PROTO_Send("OK MAPC uid=%s", uid_s);
  }
  else if (strcmp(cmd, "MAP?") == 0)
  {
    uint8_t mods;
    uint16_t code;
    if (Arg_Uid(args, uid))
    {
      PROTO_Send("ERR usage: MAP? uid=<24hex>");
      return;
    }
    PROTO_FormatUid(uid_s, uid);
    if (KEYMAP_Get(uid, &mods, &code) == KEYMAP_CONSUMER)
    {
      PROTO_Send("MAPC uid=%s usage=%04X", uid_s, code);
    }
    else
    {
      PROTO_Send("MAP uid=%s mods=%02X key=%02X", uid_s, mods, code);
    }
  }
  else if (strcmp(cmd, "UNMAP") == 0)
  {
    if (Arg_Uid(args, uid))
    {
      PROTO_Send("ERR usage: UNMAP uid=<24hex>");
      return;
    }
    KEYMAP_Unmap(uid);
    PROTO_FormatUid(uid_s, uid);
    PROTO_Send("OK UNMAP uid=%s", uid_s);
  }
  else if (strcmp(cmd, "CLEAR") == 0)
  {
    KEYMAP_Clear();
    PROTO_Send("OK CLEAR");
  }
  else if (strcmp(cmd, "SAVE") == 0)
  {
    if (KEYMAP_Save() != 0) PROTO_Send("ERR flash write failed");
    else PROTO_Send("OK SAVE");
  }
  else
  {
    PROTO_Send("ERR unknown command");
  }
}

void PROTO_Input(const uint8_t *data, uint16_t len)
{
  for (uint16_t i = 0; i < len; i++)
  {
    char c = (char)data[i];
    if (c == '\n')
    {
      if (line_len && line[line_len - 1] == '\r') line_len--;
      line[line_len] = 0;
      if (line_overflow) PROTO_Send("ERR line too long");
      else if (line_len) Handle_Line(line);
      line_len = 0;
      line_overflow = 0;
    }
    else if (line_len < LINE_MAX - 1)
    {
      line[line_len++] = c;
    }
    else
    {
      line_overflow = 1;
    }
  }
}
