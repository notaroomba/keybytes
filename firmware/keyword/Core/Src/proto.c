#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "keymap.h"
#include "batt.h"
#include "keybit_image.h"
#include "ota.h"
#include "fus.h"
#include "bootcfg.h"
#include "proto.h"
#include "main.h"

extern I2C_HandleTypeDef hi2c3;   /* the side bus */
#include "ch32v003_swio.h"

/* Optional features the desktop app can use, only what's implemented.
 * ota only while the CPU2 BLE stack runs, since the OTA bootloader needs it. */
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

/* uid= is 24 hex digits, or 8 for a relayed Keybit known only by its low
 * 32 bits. Printed most significant byte first, stored little endian. */
static int Arg_Uid(const char *args, keyid_t *id)
{
  uint8_t be[UID_LEN];
  memset(id, 0, sizeof(*id));
  if (Arg_Hex(args, "uid", UID_LEN * 2, be) == 0) id->len = UID_LEN;
  else if (Arg_Hex(args, "uid", UID_SHORT_LEN * 2, be) == 0) id->len = UID_SHORT_LEN;
  else return -1;
  for (int i = 0; i < id->len; i++) id->uid[i] = be[id->len - 1 - i];
  return 0;
}

/* Replies echo the uid in the form it was given. */
static void Format_Id(char *out, const keyid_t *id)
{
  if (id->len == UID_LEN)
  {
    PROTO_FormatUid(out, id->uid);
    return;
  }
  snprintf(out, 9, "%02X%02X%02X%02X", id->uid[3], id->uid[2], id->uid[1], id->uid[0]);
}

static void Handle_Line(proto_port_t *port, char *cmd)
{
  char uid_s[UID_LEN * 2 + 1];
  keyid_t id;
  char *args = strchr(cmd, ' ');
  if (args) *args++ = 0;
  else args = cmd + strlen(cmd);

  if (strcmp(cmd, "HELLO") == 0)
  {
    /* kb: version of the Keybit image this keyword flashes onto new Keybits */
    Reply(port, "KEYWORD v=%s proto=1 caps=%s%s kb=%u", KEYWORD_FW_VERSION, CAPS,
          OTA_Available() ? ",ota" : "", keybit_image_version);
  }
#ifdef KEYWORD_DEBUG_CMDS
  else if (strcmp(cmd, "CRASH") == 0 && port->local)
  {
    /* Test for the bootloader's crash fallback: hang until the IWDG resets
     * us. Three in a row should end in ROM USB DFU. Debug builds only. */
    Reply(port, "OK CRASH");
    for (volatile uint32_t i = 0; i < 2000000; i++);   /* let the reply out */
    while (1);
  }
#endif
  else if (strcmp(cmd, "FUS?") == 0)
  {
    char info[96];
    FUS_Info(info, sizeof(info));
    Reply(port, "FUS %s", info);
  }
  else if (strcmp(cmd, "STACK") == 0 && strcmp(args, "INSTALL") == 0)
  {
    /* Hands CPU2's flash to FUS: only from someone at the USB cable */
    if (!port->local)
    {
      Reply(port, "ERR STACK INSTALL is USB only");
      return;
    }
    const char *why = FUS_Install();
    if (why) Reply(port, "ERR %s", why);
    else Reply(port, "OK STACK INSTALL");
  }
  else if (strcmp(cmd, "STACK?") == 0)
  {
    char info[48];
    OTA_StackInfo(info, sizeof(info));
    Reply(port, "STACK %s", info);
  }
  else if (strcmp(cmd, "OTA") == 0)
  {
    /* The loader runs on the same CPU2 BLE stack: without it OTA would
     * just strand the keyword in the loader (which then drops to DFU). */
    if (!OTA_Available())
    {
      Reply(port, "ERR no BLE stack on CPU2, use DFU");
      return;
    }
    Reply(port, "OK OTA");
    OTA_Request();
  }
  else if (strcmp(cmd, "DFU") == 0)
  {
    Reply(port, "OK DFU");
    OTA_RequestDfu();
  }
  else if (strcmp(cmd, "BOOTCFG?") == 0)
  {
    char info[48];
    BOOTCFG_Info(info, sizeof(info));
    Reply(port, "BOOTCFG %s", info);
  }
  else if (strcmp(cmd, "BOOTLOCK") == 0)
  {
    /* Boot from flash always, BOOT0 ignored. Option bytes: USB only, and
     * only once the app and the BLE stack have proven themselves. */
    if (!port->local)
    {
      Reply(port, "ERR BOOTLOCK is USB only");
      return;
    }
    if (strcmp(args, "yes") != 0)
    {
      Reply(port, "ERR send BOOTLOCK yes to always boot from flash (BOOT0 ignored)");
      return;
    }
    if (BOOTCFG_Locked())
    {
      Reply(port, "OK BOOTLOCK already");
      return;
    }
    const char *why = BOOTCFG_Lock();
    if (why) Reply(port, "ERR %s", why);
    else Reply(port, "OK BOOTLOCK, reloading option bytes");
  }
  else if (strcmp(cmd, "BATT?") == 0)
  {
    const batt_t *b = BATT_Get();
    if (!b->valid) Reply(port, "ERR no battery reading yet");
    else Reply(port, "BATT pct=%u chg=%u usb=%u", b->pct, b->chg, b->usb);
  }
  else if (strcmp(cmd, "DOCK?") == 0)
  {
    /* optional: a=/b=/h=<"1" low, "0" low, high, wire cycles at 64 MHz>
     * s=<read sample delay, cycles at 64 MHz> od=<0|1> */
    const char *a;
    if ((a = strstr(args, "a=")) && atoi(a + 2) > 0) ch32_t1 = (uint32_t)atoi(a + 2);
    if ((a = strstr(args, "b=")) && atoi(a + 2) > 0) ch32_t0 = (uint32_t)atoi(a + 2);
    if ((a = strstr(args, "h=")) && atoi(a + 2) > 0) ch32_hi = (uint32_t)atoi(a + 2);
    if ((a = strstr(args, "k="))) ch32_t_kick = (uint32_t)atoi(a + 2);
    if ((a = strstr(args, "s=")) && atoi(a + 2) > 0) ch32_t_sample = (uint32_t)atoi(a + 2);
    if ((a = strstr(args, "od="))) ch32_open_drain = a[3] == '1';
    keybit_dock_pause_until = HAL_GetTick() + 5000;
    uint32_t cfg, status;
    int r = Keybit_Dock_Probe(&cfg, &status);
    uint32_t t = DWT->CYCCNT;
    HAL_Delay(10);
    t = DWT->CYCCNT - t;   /* ~640000 at 64 MHz: confirms the SWIO pulse timing base */
    Reply(port, "DOCK r=%d cfg=%08lx status=%08lx a=%lu b=%lu h=%lu k=%lu s=%lu od=%u hclk=%lu cyc10ms=%lu", r, (unsigned long)cfg,
          (unsigned long)status, (unsigned long)ch32_t1, (unsigned long)ch32_t0, (unsigned long)ch32_hi, (unsigned long)ch32_t_kick, (unsigned long)ch32_t_sample, ch32_open_drain,
          (unsigned long)HAL_RCC_GetHCLKFreq(), (unsigned long)t);
  }
  else if (strcmp(cmd, "DOCKSCAN") == 0)
  {
    /* Try write pulse widths ("1" low, "0" low) and read sample points on
     * whatever sits on the dock, high time fixed, open drain or not
     * (od=<0|1>, default the current mode). Each one that passes detection
     * gets 256 register reads (rd), then a halt and 256 DMDATA0 write/read-back
     * round trips (wr); score = rd + wr. Prints scores of 480 and up and keeps
     * the best, a tie going to the one with more neighbours at 500 and up.
     * Hold the Keybit still. */
    static const uint8_t as[] = { 17, 18, 19 };
    static const uint8_t bs[] = { 44, 52, 60 };
    static const uint8_t ss[] = { 16, 20, 24, 28, 32, 36, 40, 44 };
    enum { NA = sizeof(as), NB = sizeof(bs), NS = sizeof(ss) };
    int sc[NA][NB][NS];
    const char *a = strstr(args, "od=");
    if (a) ch32_open_drain = a[3] == '1';
    ch32_hi = 46;
    for (int i = 0; i < NA; i++)
      for (int j = 0; j < NB; j++)
        for (int k = 0; k < NS; k++)
        {
          keybit_dock_pause_until = HAL_GetTick() + 5000;
          ch32_t1 = as[i];
          ch32_t0 = bs[j];
          ch32_t_sample = ss[k];
          uint32_t cfg, status, bw, br;
          int bad, hr;
          sc[i][j][k] = -1;
          if (Keybit_Dock_Probe(&cfg, &status) != 0) continue;
          int rd = Keybit_Dock_Reads(256);
          int wr = Keybit_Dock_Score(256, &bw, &br, &bad, &hr);
          if (hr != 0) {
            sc[i][j][k] = rd;
            if (rd >= 240) Reply(port, "DOCKSCAN a=%u b=%u s=%u rd=%d/256 hr=%d", as[i], bs[j], ss[k], rd, hr);
            continue;
          }
          sc[i][j][k] = rd + wr;
          if (rd + wr >= 480) Reply(port, "DOCKSCAN a=%u b=%u s=%u rd=%d/256 wr=%d/256", as[i], bs[j], ss[k], rd, wr);
        }
    int bi = 0, bj = 0, bk = 0, best = -1, bn = -1;
    for (int i = 0; i < NA; i++)
      for (int j = 0; j < NB; j++)
        for (int k = 0; k < NS; k++)
        {
          int n = (i > 0 && sc[i - 1][j][k] >= 500) + (i < NA - 1 && sc[i + 1][j][k] >= 500) +
                  (j > 0 && sc[i][j - 1][k] >= 500) + (j < NB - 1 && sc[i][j + 1][k] >= 500) +
                  (k > 0 && sc[i][j][k - 1] >= 500) + (k < NS - 1 && sc[i][j][k + 1] >= 500);
          if (sc[i][j][k] > best || (sc[i][j][k] == best && n > bn)) { best = sc[i][j][k]; bn = n; bi = i; bj = j; bk = k; }
        }
    ch32_t1 = as[bi];
    ch32_t0 = bs[bj];
    ch32_t_sample = ss[bk];
    keybit_dock_pause_until = HAL_GetTick() + 5000;
    Reply(port, "DOCKSCAN best a=%u b=%u s=%u score=%d/512 nb=%d", as[bi], bs[bj], ss[bk], best, bn);
  }
  else if (strcmp(cmd, "DOCKID") == 0)
  {
    /* optional: h=<0|1> halt with reset (default 1) */
    const char *a = strstr(args, "h=");
    char buf[200];
    keybit_dock_pause_until = HAL_GetTick() + 5000;
    Keybit_Dock_Id(buf, sizeof(buf), a ? a[2] == '1' : 1);
    /* lines cap at PROTO_LINE_MAX, so long results go out in pieces */
    for (size_t o = 0, l = strlen(buf); o < l || o == 0; o += 100)
      Reply(port, "DOCKID%s %.100s", o ? "+" : "", buf + o);
  }
  else if (strcmp(cmd, "RST?") == 0)
  {
    /* Why the keyword last reset (RCC->CSR at boot). Several flags can be set. */
    uint32_t c = keyword_reset_csr;
    Reply(port, "RST csr=%08lx%s%s%s%s%s%s", (unsigned long)c, (c & RCC_CSR_IWDGRSTF) ? " iwdg" : "",
          (c & RCC_CSR_WWDGRSTF) ? " wwdg" : "", (c & RCC_CSR_BORRSTF) ? " bor" : "",
          (c & RCC_CSR_PINRSTF) ? " pin" : "", (c & RCC_CSR_SFTRSTF) ? " sw" : "",
          (c & RCC_CSR_LPWRRSTF) ? " lpwr" : "");
  }
  else if (strcmp(cmd, "I2CSCAN") == 0)
  {
    /* Every 7-bit address that ACKs on the side bus, and the idle line levels. */
    char list[100];
    int n = 0;
    list[0] = 0;
    for (uint8_t ad = 0x08; ad <= 0x77; ad++)
      if (HAL_I2C_IsDeviceReady(&hi2c3, (uint16_t)(ad << 1), 1, 10) == HAL_OK && n < (int)sizeof(list) - 4)
        n += snprintf(list + n, sizeof(list) - n, " %02x", ad);
    Reply(port, "I2CSCAN sda=%d scl=%d acks:%s", HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14) == GPIO_PIN_SET,
          HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13) == GPIO_PIN_SET, n ? list : " none");
  }
  else if (strcmp(cmd, "I2CSPEED") == 0)
  {
    /* I2CSPEED <khz>: retime the side bus. I2C3 runs off PCLK1 (64 MHz);
     * with PRESC=15 one tick is 0.25 us, so SCL low and high max out at
     * 256 ticks each and the slowest it goes is about 7.8 kHz. */
    unsigned khz = (unsigned)atoi(args);
    if (khz < 1 || khz > 400)
      Reply(port, "ERR usage: I2CSPEED <khz 1-400>");
    else {
      uint32_t half = 2000u / khz;   /* ticks per half period */
      if (half > 256) half = 256;
      if (half < 2) half = 2;
      uint32_t t = (15u << 28) | (4u << 20) | (2u << 16) | ((half - 1) << 8) | (half - 1);
      __HAL_I2C_DISABLE(&hi2c3);
      hi2c3.Instance->TIMINGR = t;
      __HAL_I2C_ENABLE(&hi2c3);
      Reply(port, "I2CSPEED timing=%08lx ~%lu Hz", (unsigned long)t, (unsigned long)(2000000u / half));
    }
  }
  else if (strcmp(cmd, "LINKS?") == 0)
  {
    /* LINKS? <addr hex>: a Keybit's link counters (v9+, regs 0x30-0x4F), one line per side. */
    unsigned ad = 0;
    uint8_t d[32];
    static const char side[4] = { 'S', 'E', 'W', 'N' };
    if (sscanf(args, "%x", &ad) != 1)
      Reply(port, "ERR usage: LINKS? <addr hex>");
    else if (HAL_I2C_Mem_Read(&hi2c3, (uint16_t)(ad << 1), 0x30, I2C_MEMADD_SIZE_8BIT, d, 16, 50) != HAL_OK ||
             HAL_I2C_Mem_Read(&hi2c3, (uint16_t)(ad << 1), 0x40, I2C_MEMADD_SIZE_8BIT, d + 16, 16, 50) != HAL_OK)
      Reply(port, "LINKS %02x err=%08lx", ad, (unsigned long)HAL_I2C_GetError(&hi2c3));
    else
      for (int i = 0; i < 4; i++) {
        const uint8_t *c = d + 8 * i;
        Reply(port, "LINKS %02x %c req=%u ok=%u noack=%u abort=%u bad=%u served=%u servefail=%u skipped=%u", ad,
              side[i], c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7]);
      }
  }
  else if (strcmp(cmd, "I2CRD") == 0)
  {
    /* I2CRD <addr hex> <reg hex> <n>: raw register read on the side bus. */
    unsigned ad = 0, reg = 0, cnt = 0;
    uint8_t buf[16];
    if (sscanf(args, "%x %x %u", &ad, &reg, &cnt) != 3 || cnt == 0 || cnt > sizeof(buf))
      Reply(port, "ERR usage: I2CRD <addr hex> <reg hex> <n 1-16>");
    else if (HAL_I2C_Mem_Read(&hi2c3, (uint16_t)(ad << 1), (uint16_t)reg, I2C_MEMADD_SIZE_8BIT, buf, (uint16_t)cnt, 10) != HAL_OK)
      Reply(port, "I2CRD %02x %02x err=%08lx", ad, reg, (unsigned long)HAL_I2C_GetError(&hi2c3));
    else {
      char hex[3 * 16 + 1];
      for (unsigned i = 0; i < cnt; i++) snprintf(hex + 3 * i, 4, " %02x", buf[i]);
      Reply(port, "I2CRD %02x %02x:%s", ad, reg, hex);
    }
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
    char line[PROTO_LINE_MAX];
    for (int i = 0; KEYMAP_KeyLine(i, line, sizeof(line)); i++)
    {
      Reply(port, "%s", line);
    }
    Reply(port, "KEYS END");
  }
  else if (strcmp(cmd, "MAP") == 0)
  {
    uint8_t mods, key;
    if (Arg_Uid(args, &id) || Arg_Hex(args, "mods", 2, &mods) || Arg_Hex(args, "key", 2, &key))
    {
      Reply(port, "ERR usage: MAP uid=<24hex> mods=<2hex> key=<2hex>");
      return;
    }
    if (KEYMAP_Set(&id, mods, key) != 0)
    {
      Reply(port, "ERR keymap full");
      return;
    }
    Format_Id(uid_s, &id);
    Reply(port, "OK MAP uid=%s", uid_s);
  }
  else if (strcmp(cmd, "MAPC") == 0)
  {
    uint8_t usage[2];
    if (Arg_Uid(args, &id) || Arg_Hex(args, "usage", 4, usage))
    {
      Reply(port, "ERR usage: MAPC uid=<24hex> usage=<4hex>");
      return;
    }
    if (KEYMAP_SetConsumer(&id, (uint16_t)(usage[0] << 8 | usage[1])) != 0)
    {
      Reply(port, "ERR keymap full");
      return;
    }
    Format_Id(uid_s, &id);
    Reply(port, "OK MAPC uid=%s", uid_s);
  }
  else if (strcmp(cmd, "MACRO") == 0)
  {
    uint8_t seq[KEYACT_MACRO_MAX][2];
    int n = -1;
    if (Arg_Uid(args, &id) || (n = Arg_Seq(args, seq)) < 0)
    {
      Reply(port, "ERR usage: MACRO uid=<24hex> seq=<MMKK,...> (1-%d taps)", KEYACT_MACRO_MAX);
      return;
    }
    if (KEYMAP_SetMacro(&id, (const uint8_t (*)[2])seq, (uint8_t)n) != 0)
    {
      Reply(port, "ERR keymap full");
      return;
    }
    Format_Id(uid_s, &id);
    Reply(port, "OK MACRO uid=%s", uid_s);
  }
  else if (strcmp(cmd, "HOLD") == 0)
  {
    uint8_t mods, key;
    uint16_t ms = KEYACT_HOLD_MS_DEF;
    if (Arg_Uid(args, &id) || Arg_Hex(args, "mods", 2, &mods) || Arg_Hex(args, "key", 2, &key)
        || (strstr(args, "ms=") && Arg_Dec(args, "ms", &ms)))
    {
      Reply(port, "ERR usage: HOLD uid=<24hex> mods=<2hex> key=<2hex> ms=<dec>");
      return;
    }
    if (KEYMAP_SetHold(&id, mods, key, ms) != 0)
    {
      Reply(port, "ERR keymap full");
      return;
    }
    Format_Id(uid_s, &id);
    Reply(port, "OK HOLD uid=%s", uid_s);
  }
  else if (strcmp(cmd, "MAP?") == 0)
  {
    if (Arg_Uid(args, &id))
    {
      Reply(port, "ERR usage: MAP? uid=<24hex>");
      return;
    }
    Format_Id(uid_s, &id);
    const keyact_t *a = KEYMAP_Lookup(&id);
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
    if (Arg_Uid(args, &id))
    {
      Reply(port, "ERR usage: UNMAP uid=<24hex>");
      return;
    }
    KEYMAP_Unmap(&id);
    Format_Id(uid_s, &id);
    Reply(port, "OK UNMAP uid=%s", uid_s);
  }
  else if (strcmp(cmd, "CLEAR") == 0)
  {
    KEYMAP_Clear();
    Reply(port, "OK CLEAR");
  }
  else if (strcmp(cmd, "SAVE") == 0)
  {
    /* Without the BLE stack the keymap page (SFSA - 1) moves when the stack
     * gets installed, and FUS may be writing flash: don't save then. */
    if (!OTA_Available() || FUS_Busy())
    {
      Reply(port, "ERR no BLE stack yet, keymap not saved");
      return;
    }
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
