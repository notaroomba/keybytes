#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "keymap.h"
#include "batt.h"
#include "proto.h"

/* Optional features the desktop app can use, only what's implemented.
 * kb= (the embedded Keybit image version) waits for keybit_image.h to
 * export one. */
#define CAPS "batt,hold,macro"

static proto_port_t *ports[PROTO_MAX_PORTS];
static int port_count;

void PROTO_AddPort(proto_port_t *port)
{
  if (port_count < PROTO_MAX_PORTS) ports[port_count++] = port;
}

/* Formats one line into out with the \n, returns its length or -1. */
static int Format(char out[PROTO_LINE_MAX], const char *fmt, va_list ap)
{
  int n = vsnprintf(out, PROTO_LINE_MAX - 1, fmt, ap);
  if (n < 0) return -1;
  if (n > PROTO_LINE_MAX - 2) n = PROTO_LINE_MAX - 2;
  out[n++] = '\n';
  return n;
}

void PROTO_Send(const char *fmt, ...)
{
  char out[PROTO_LINE_MAX];
  va_list ap;
  va_start(ap, fmt);
  int n = Format(out, fmt, ap);
  va_end(ap);
  if (n < 0) return;
  for (int i = 0; i < port_count; i++)
  {
    if (ports[i]->connected) ports[i]->write(out, (uint16_t)n);
  }
}

static void Reply(proto_port_t *port, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
static void Reply(proto_port_t *port, const char *fmt, ...)
{
  char out[PROTO_LINE_MAX];
  va_list ap;
  va_start(ap, fmt);
  int n = Format(out, fmt, ap);
  va_end(ap);
  if (n >= 0) port->write(out, (uint16_t)n);
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

/* Finds "name=" and parses 1 to 5 decimal digits after it. 0 on success. */
static int Arg_Dec(const char *args, const char *name, uint16_t *out)
{
  size_t nl = strlen(name);
  for (const char *p = args; (p = strstr(p, name)) != NULL; p += nl)
  {
    if ((p != args && p[-1] != ' ') || p[nl] != '=') continue;
    const char *v = p + nl + 1;
    uint32_t n = 0;
    int digits = 0;
    for (; *v >= '0' && *v <= '9'; v++, digits++) n = n * 10 + (uint32_t)(*v - '0');
    if (digits == 0 || digits > 5 || n > 0xFFFF || (*v != 0 && *v != ' ')) return -1;
    *out = (uint16_t)n;
    return 0;
  }
  return -1;
}

/* seq=MMKK,MMKK,... (mods byte, key usage), 1 to KEYACT_MACRO_MAX taps.
 * Returns the count, or -1. */
static int Arg_Seq(const char *args, uint8_t seq[KEYACT_MACRO_MAX][2])
{
  const char *v = strstr(args, "seq=");
  if (!v || (v != args && v[-1] != ' ')) return -1;
  v += 4;
  for (int n = 0; n < KEYACT_MACRO_MAX; n++)
  {
    for (int i = 0; i < 4; i++)
    {
      if (Hex_Digit(v[i]) < 0) return -1;
    }
    seq[n][0] = (uint8_t)(Hex_Digit(v[0]) << 4 | Hex_Digit(v[1]));
    seq[n][1] = (uint8_t)(Hex_Digit(v[2]) << 4 | Hex_Digit(v[3]));
    v += 4;
    if (*v == 0 || *v == ' ') return n + 1;
    if (*v++ != ',') return -1;
  }
  return -1;   /* more than KEYACT_MACRO_MAX */
}

/* uid= is printed most significant byte first, stored little endian. */
static int Arg_Uid(const char *args, uint8_t uid[UID_LEN])
{
  uint8_t be[UID_LEN];
  if (Arg_Hex(args, "uid", UID_LEN * 2, be) != 0) return -1;
  for (int i = 0; i < UID_LEN; i++) uid[i] = be[UID_LEN - 1 - i];
  return 0;
}

static void Handle_Line(proto_port_t *port, char *cmd)
{
  char uid_s[UID_LEN * 2 + 1];
  uint8_t uid[UID_LEN];
  char *args = strchr(cmd, ' ');
  if (args) *args++ = 0;
  else args = cmd + strlen(cmd);

  if (strcmp(cmd, "HELLO") == 0)
  {
    Reply(port, "KEYWORD v=%s proto=1 caps=%s", KEYWORD_FW_VERSION, CAPS);
  }
  else if (strcmp(cmd, "BATT?") == 0)
  {
    const batt_t *b = BATT_Get();
    if (!b->valid) Reply(port, "ERR no battery reading yet");
    else Reply(port, "BATT pct=%u chg=%u usb=%u", b->pct, b->chg, b->usb);
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
    Reply(port, "SIDES L=%d R=%d T=%d", l, r, t);
  }
  else if (strcmp(cmd, "KEYS?") == 0)
  {
    int n;
    const keybit_info_t *k = KEYMAP_Present(&n);
    for (int i = 0; i < n; i++)
    {
      PROTO_FormatUid(uid_s, k[i].uid);
      Reply(port, "KEY+ uid=%s at=%c v=%u", uid_s, k[i].at, k[i].fw);
    }
    Reply(port, "KEYS END");
  }
  else if (strcmp(cmd, "MAP") == 0)
  {
    uint8_t mods, key;
    if (Arg_Uid(args, uid) || Arg_Hex(args, "mods", 2, &mods) || Arg_Hex(args, "key", 2, &key))
    {
      Reply(port, "ERR usage: MAP uid=<24hex> mods=<2hex> key=<2hex>");
      return;
    }
    if (KEYMAP_Set(uid, mods, key) != 0)
    {
      Reply(port, "ERR keymap full");
      return;
    }
    PROTO_FormatUid(uid_s, uid);
    Reply(port, "OK MAP uid=%s", uid_s);
  }
  else if (strcmp(cmd, "MAPC") == 0)
  {
    uint8_t usage[2];
    if (Arg_Uid(args, uid) || Arg_Hex(args, "usage", 4, usage))
    {
      Reply(port, "ERR usage: MAPC uid=<24hex> usage=<4hex>");
      return;
    }
    if (KEYMAP_SetConsumer(uid, (uint16_t)(usage[0] << 8 | usage[1])) != 0)
    {
      Reply(port, "ERR keymap full");
      return;
    }
    PROTO_FormatUid(uid_s, uid);
    Reply(port, "OK MAPC uid=%s", uid_s);
  }
  else if (strcmp(cmd, "MACRO") == 0)
  {
    uint8_t seq[KEYACT_MACRO_MAX][2];
    int n = -1;
    if (Arg_Uid(args, uid) || (n = Arg_Seq(args, seq)) < 0)
    {
      Reply(port, "ERR usage: MACRO uid=<24hex> seq=<MMKK,...> (1-%d taps)", KEYACT_MACRO_MAX);
      return;
    }
    if (KEYMAP_SetMacro(uid, (const uint8_t (*)[2])seq, (uint8_t)n) != 0)
    {
      Reply(port, "ERR keymap full");
      return;
    }
    PROTO_FormatUid(uid_s, uid);
    Reply(port, "OK MACRO uid=%s", uid_s);
  }
  else if (strcmp(cmd, "HOLD") == 0)
  {
    uint8_t mods, key;
    uint16_t ms = KEYACT_HOLD_MS_DEF;
    if (Arg_Uid(args, uid) || Arg_Hex(args, "mods", 2, &mods) || Arg_Hex(args, "key", 2, &key)
        || (strstr(args, "ms=") && Arg_Dec(args, "ms", &ms)))
    {
      Reply(port, "ERR usage: HOLD uid=<24hex> mods=<2hex> key=<2hex> ms=<dec>");
      return;
    }
    if (KEYMAP_SetHold(uid, mods, key, ms) != 0)
    {
      Reply(port, "ERR keymap full");
      return;
    }
    PROTO_FormatUid(uid_s, uid);
    Reply(port, "OK HOLD uid=%s", uid_s);
  }
  else if (strcmp(cmd, "MAP?") == 0)
  {
    if (Arg_Uid(args, uid))
    {
      Reply(port, "ERR usage: MAP? uid=<24hex>");
      return;
    }
    PROTO_FormatUid(uid_s, uid);
    const keyact_t *a = KEYMAP_Lookup(uid);
    if (a && a->kind == KEYACT_CONSUMER)
    {
      Reply(port, "MAPC uid=%s usage=%04X", uid_s, a->code);
    }
    else if (a && a->kind == KEYACT_MACRO)
    {
      /* 16 taps is 79 chars of seq, the line still fits PROTO_LINE_MAX */
      char seq_s[KEYACT_MACRO_MAX * 5];
      int len = 0;
      for (int i = 0; i < a->macro_len; i++)
      {
        len += snprintf(&seq_s[len], sizeof(seq_s) - (size_t)len, "%s%02X%02X",
                        i ? "," : "", a->macro[i][0], a->macro[i][1]);
      }
      seq_s[len] = 0;
      Reply(port, "MACRO uid=%s seq=%s", uid_s, seq_s);
    }
    else
    {
      Reply(port, "MAP uid=%s mods=%02X key=%02X", uid_s, a ? a->mods : 0, a ? a->code : 0);
    }
    if (a && (a->hold_mods || a->hold_key))
    {
      Reply(port, "HOLD uid=%s mods=%02X key=%02X ms=%u", uid_s, a->hold_mods, a->hold_key, a->hold_ms);
    }
  }
  else if (strcmp(cmd, "UNMAP") == 0)
  {
    if (Arg_Uid(args, uid))
    {
      Reply(port, "ERR usage: UNMAP uid=<24hex>");
      return;
    }
    KEYMAP_Unmap(uid);
    PROTO_FormatUid(uid_s, uid);
    Reply(port, "OK UNMAP uid=%s", uid_s);
  }
  else if (strcmp(cmd, "CLEAR") == 0)
  {
    KEYMAP_Clear();
    Reply(port, "OK CLEAR");
  }
  else if (strcmp(cmd, "SAVE") == 0)
  {
    if (KEYMAP_Save() != 0) Reply(port, "ERR flash write failed");
    else Reply(port, "OK SAVE");
  }
  else
  {
    Reply(port, "ERR unknown command");
  }
}

void PROTO_Input(proto_port_t *port, const uint8_t *data, uint16_t len)
{
  for (uint16_t i = 0; i < len; i++)
  {
    char c = (char)data[i];
    /* \n ends a line; so does a bare \r, which is what terminals send
     * on Enter. \r\n just leaves an empty line, which is ignored. */
    if (c == '\n' || c == '\r')
    {
      port->line[port->len] = 0;
      if (port->len || port->overflow) port->connected = 1;
      if (port->overflow) Reply(port, "ERR line too long");
      else if (port->len) Handle_Line(port, port->line);
      port->len = 0;
      port->overflow = 0;
    }
    else if (port->len < PROTO_LINE_MAX - 1)
    {
      port->line[port->len++] = c;
    }
    else
    {
      port->overflow = 1;
    }
  }
}
