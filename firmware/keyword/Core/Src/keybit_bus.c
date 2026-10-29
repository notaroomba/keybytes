#include "keybit_bus.h"
#include <string.h>

#define I2C_TIMEOUT_MS 50   /* at 20 kHz a full 8-event relay read alone takes ~19 ms */
#define ADDR_FIRST 0x08
#define ADDR_LAST  0x77
#define CHARGER_ADDR 0x6A  /* BQ25895 shares I2C3 */
#define LINK_ADDR    0x31  /* Keybit-to-Keybit links */

#define REG_MAGIC  0x00
#define REG_KEY    0x06
#define REG_UID    0x10
#define REG_ASSIGN 0x20
#define REG_EV_COUNT 0x52
#define REG_EV_DATA  0x53
#define REG_EV_POP   0x7B
#define REG_EV_DATA11 0x80  /* v11: 11-byte events with where the Keybit is */
#define EV_SIZE11    11
#define EV_BATCH     8
#define EV_PRESSED   0x01
#define EV_HEARTBEAT 0x02
#define EV_BACKWARDS 0x04
#define EV_WHERE     0x08   /* v12: this event's place fields are news */
#define REG_STREAM   0x7F   /* v12: status stream; 0x80 | n drops events before n first */
#define STREAM_EV    5
#define MAGIC      0x4B

/* I2C3's pins (see stm32wbxx_hal_msp.c); IDR still reads them in AF mode. */
#define BUS_PORT GPIOB
#define BUS_SCL  GPIO_PIN_13
#define BUS_SDA  GPIO_PIN_14
#define JAM_RETRY_MS 1000

int keybit_bus_held(void)
{
  return HAL_GPIO_ReadPin(BUS_PORT, BUS_SDA) == GPIO_PIN_RESET &&
         HAL_GPIO_ReadPin(BUS_PORT, BUS_SCL) == GPIO_PIN_SET;
}

int keybit_bus_unstick(I2C_HandleTypeDef *i2c)
{
  static uint32_t low_since;
  uint32_t now = HAL_GetTick();
  if (HAL_GPIO_ReadPin(BUS_PORT, BUS_SCL) == GPIO_PIN_SET || HAL_I2C_GetState(i2c) != HAL_I2C_STATE_READY) {
    low_since = now;
    return 0;
  }
  if (now - low_since < 5) return 0;

  HAL_I2C_DeInit(i2c);
  GPIO_InitTypeDef g = {0};
  g.Pin = BUS_SCL | BUS_SDA;
  g.Mode = GPIO_MODE_OUTPUT_OD;
  g.Pull = GPIO_NOPULL;
  g.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_WritePin(BUS_PORT, BUS_SCL | BUS_SDA, GPIO_PIN_SET);
  HAL_GPIO_Init(BUS_PORT, &g);
  for (int i = 0; i < 9; i++) {   /* 9 clocks finish any byte and its ACK */
    HAL_GPIO_WritePin(BUS_PORT, BUS_SCL, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(BUS_PORT, BUS_SCL, GPIO_PIN_SET);
    HAL_Delay(1);
  }
  HAL_GPIO_WritePin(BUS_PORT, BUS_SDA, GPIO_PIN_RESET);   /* STOP: SDA rises while SCL is high */
  HAL_Delay(1);
  HAL_GPIO_WritePin(BUS_PORT, BUS_SDA, GPIO_PIN_SET);
  HAL_I2C_Init(i2c);   /* MspInit puts the pins back on I2C3 */
  low_since = HAL_GetTick();
  return 1;
}

int keybit_bus_jammed(void)
{
  static uint32_t last_try;
  if (!keybit_bus_held()) return 0;
  /* Let one attempt through now and then: its few clocks free a Keybit
   * that really did get stuck mid-byte, at a tiny fraction of the usual
   * poll rate. */
  if (HAL_GetTick() - last_try >= JAM_RETRY_MS) {
    last_try = HAL_GetTick();
    return 0;
  }
  return 1;
}

/* A Keybit serves I2C in software and only looks for a START once per
 * main loop pass. After a transfer ends it finishes that pass first, so a
 * transfer straight after another one (the key read, then the event count;
 * the count, then the events) often found it not listening yet and was
 * NACKed, costing a whole poll cycle. Leave the bus idle this long first. */
#ifndef BUS_GAP_US
#define BUS_GAP_US 200
#endif
static uint32_t bus_free_at;   /* DWT cycle count when the last transfer ended */

static void bus_gap(void)
{
  uint32_t need = BUS_GAP_US * (HAL_RCC_GetHCLKFreq() / 1000000u);
  while (DWT->CYCCNT - bus_free_at < need) { }
}

static HAL_StatusTypeDef ended(HAL_StatusTypeDef st)
{
  bus_free_at = DWT->CYCCNT;
  return st;
}

/* v12 Keybits look at the bus between every step of their main loop, so
 * they keep up with a faster clock than older ones. The side bus runs fast
 * only while every touching Keybit is v12, and always slow for scans and
 * address handouts, where a Keybit of any version may answer. */
#define SLOW_KHZ 20
#ifndef FAST_KHZ
#define FAST_KHZ 100
#endif
/* SDA changes this many 0.25 us ticks after SCL falls. A Keybit reads SCL
 * and then SDA; if SDA moved in between it sees a START or STOP that isn't
 * there, so give it time (the low phase still has SCLDEL for setup). */
#ifndef SDADEL
#define SDADEL 8
#endif

/* If v12 reads keep failing at a speed, step down (100, 50, 20 kHz) and
 * try the next one up again after a while. */
#define FALLBACK_WINDOW 32       /* stream reads */
#define FALLBACK_FAILS  8        /* more failed than this in a window: slower */
#define FALLBACK_RETRY_MS 30000
static const unsigned fast_steps[] = { FAST_KHZ, FAST_KHZ / 2, SLOW_KHZ };
static struct { uint8_t step, reads, fails; uint32_t down_at; } fb;

static unsigned fast_khz(void)
{
  if (fb.step && HAL_GetTick() - fb.down_at >= FALLBACK_RETRY_MS) { fb.step--; fb.down_at = HAL_GetTick(); }
  return fast_steps[fb.step];
}

static void fast_result(int ok)
{
  fb.fails += !ok;
  if (++fb.reads < FALLBACK_WINDOW) return;
  if (fb.fails > FALLBACK_FAILS && fb.step < 2) { fb.step++; fb.down_at = HAL_GetTick(); }
  fb.reads = fb.fails = 0;
}

static void bus_khz(I2C_HandleTypeDef *i2c, unsigned khz)
{
  static unsigned now_khz;   /* 0: set it the first time, with our SDADEL */
  if (khz == now_khz) return;
  now_khz = khz;
  /* I2C3 runs off PCLK1 (64 MHz); PRESC=15 makes a tick 0.25 us */
  uint32_t half = 2000u / khz;
  uint32_t t = (15u << 28) | (4u << 20) | ((uint32_t)SDADEL << 16) | ((half - 1) << 8) | (half - 1);
#ifdef KEYBIT_SIM
  (void)i2c;
  sim_i2c_timing(t);
#else
  __HAL_I2C_DISABLE(i2c);
  i2c->Instance->TIMINGR = t;
  __HAL_I2C_ENABLE(i2c);
#endif
}

static int is_ready(I2C_HandleTypeDef *i2c, uint8_t addr)
{
  /* Just the address byte (~0.5 ms at 20 kHz): a short timeout, or a
   * misbehaving bus makes a full scan outlast the bootloader's ~8 s watchdog. */
  bus_gap();
  return ended(HAL_I2C_IsDeviceReady(i2c, (uint16_t)(addr << 1), 1, 3)) == HAL_OK;
}

static int read_regs(I2C_HandleTypeDef *i2c, uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
  bus_gap();
  return ended(HAL_I2C_Mem_Read(i2c, (uint16_t)(addr << 1), reg, I2C_MEMADD_SIZE_8BIT,
                                buf, len, I2C_TIMEOUT_MS)) == HAL_OK;
}

static int write_regs(I2C_HandleTypeDef *i2c, uint8_t addr, uint8_t reg, const uint8_t *buf, uint16_t len)
{
  bus_gap();
  return ended(HAL_I2C_Mem_Write(i2c, (uint16_t)(addr << 1), reg, I2C_MEMADD_SIZE_8BIT,
                                 (uint8_t *)buf, len, I2C_TIMEOUT_MS)) == HAL_OK;
}

/* A Keybit busy with a link frame can miss a probe, so an address only
 * counts as free if it doesn't answer, isn't on the list, and wasn't
 * handed out in the last GIVEN_MS (the scan may not have found it yet).
 * Handing one out twice made the pair fight it out and one start over. */
#define GIVEN_MS 5000
static uint8_t listed[128];      /* addresses on the list, by address */
static uint32_t given_at[128];   /* HAL tick + 1 it was handed out, 0 = never */

static uint8_t free_address(I2C_HandleTypeDef *i2c)
{
  uint32_t now = HAL_GetTick();
  for (uint8_t a = ADDR_FIRST; a <= ADDR_LAST; a++) {
    if (a == KEYBIT_DEFAULT_ADDR || a == LINK_ADDR || a == CHARGER_ADDR || listed[a]) continue;
    if (given_at[a] && now - (given_at[a] - 1) < GIVEN_MS) continue;
    if (!is_ready(i2c, a)) return a;
  }
  return 0;
}

int keybit_bus_assign(I2C_HandleTypeDef *i2c)
{
  int assigned = 0;
  /* Each round, all unassigned Keybits answer the UID read together and
   * arbitrate down to the lowest UID; that one gets an address and leaves
   * the default address, so the next round finds the next one. */
  for (int round = 0; round < KEYBIT_MAX; round++) {
    uint8_t msg[13];
    if (!read_regs(i2c, KEYBIT_DEFAULT_ADDR, REG_UID, msg, 12)) break;
    uint8_t addr = free_address(i2c);
    if (!addr) break;
    msg[12] = addr;
    if (!write_regs(i2c, KEYBIT_DEFAULT_ADDR, REG_ASSIGN, msg, sizeof(msg)))
      break;
    given_at[addr] = HAL_GetTick() + 1;
    HAL_Delay(15);  /* it saves the address to flash, about 6 ms */
    assigned++;
  }
  return assigned;
}

/* The Keybit's side touching us gives its place on the keyword. It can only
 * mate upright, so its east side means it sits on our left, and so on. */
static char keyword_side(char keybit_side)
{
  switch (keybit_side) {
  case 'E': return 'L';
  case 'W': return 'R';
  case 'N': return 'R';   /* turned 90 degrees: only the right side meets an N side the right way round */
  case 'S': return 'T';
  default:  return '?';
  }
}

int keybit_bus_scan(I2C_HandleTypeDef *i2c, keybit_info_t *out, int max)
{
  int n = 0;
  for (uint8_t a = ADDR_FIRST; a <= ADDR_LAST && n < max; a++) {
    IWDG->KR = 0xAAAA;
    if (a == KEYBIT_DEFAULT_ADDR || a == LINK_ADDR || a == CHARGER_ADDR) continue;
    if (!is_ready(i2c, a)) continue;

    uint8_t head[10];
    if (!read_regs(i2c, a, REG_MAGIC, head, sizeof(head)) || head[0] != MAGIC) continue;

    keybit_info_t *k = &out[n];
    memset(k, 0, sizeof(*k));
    /* Reading the UID also catches two Keybits on one address: the loser of
     * the arbitration drops its address and turns up on the default one. */
    if (!read_regs(i2c, a, REG_UID, k->uid, sizeof(k->uid))) continue;
    k->addr = a;
    k->fw = head[1];
    k->key_down = head[6] & 1;
    k->keybit_side = (char)head[7];
    k->at = keyword_side(k->keybit_side);
    k->backwards = k->fw >= 6 ? head[9] & 0x0F : 0;  /* reg 0x09 is new in v6 */
    n++;
  }
  return n;
}

uint32_t keybit_bus_poll_keys(I2C_HandleTypeDef *i2c, keybit_info_t *k, int n)
{
  uint32_t changed = 0;
  for (int i = 0; i < n; i++) {
    uint8_t v;
    if (k[i].fw >= 12) continue;   /* read with its events instead */
    if (!read_regs(i2c, k[i].addr, REG_KEY, &v, 1)) continue;  /* scans drop it if this keeps up */
    k[i].last_ok = HAL_GetTick();
    if (v & 1) {
      k[i].saw_down = 1;
      k[i].pend = 0;
    }
    if ((v & 1) != k[i].key_down) {
      k[i].key_down = v & 1;
      changed |= 1u << i;
    }
  }
  return changed;
}

static uint32_t uid32(const uint8_t *b)
{
  return (uint32_t)b[0] | (uint32_t)b[1] << 8 | (uint32_t)b[2] << 16 | (uint32_t)b[3] << 24;
}

static keybit_remote_t *remote_slot(keybit_remote_t *r, int *rn, uint32_t uid)
{
  keybit_remote_t *free_slot = NULL;
  for (int i = 0; i < *rn; i++) {
    if (r[i].uid == uid) return &r[i];
    if (!r[i].present && !r[i].dirty && !free_slot) free_slot = &r[i];
  }
  if (*rn < KEYBIT_REMOTE_MAX) return &r[(*rn)++];
  return free_slot;  /* NULL when the table is full */
}

/* One event from touching Keybit k[i]'s queue: size 5 (uid, flags) or 11
 * (with the place). Returns 1 if an entry changed; *remote is set when it
 * was a chained Keybit's. */
static int take_event(keybit_info_t *k, int n, int i, const uint8_t *x8, int size,
                      keybit_remote_t *r, int *rn, int *remote)
{
  uint32_t uid = uid32(x8);
  uint8_t flags = x8[4];

  /* Touching Keybits queue their own key events too. Their key is read
   * directly, but a tap shorter than the key poll only shows up here:
   * a press the poll never saw, then its release, is a missed tap. */
  int direct = 0;
  for (int j = 0; j < n; j++) {
    if (uid32(k[j].uid) != uid) continue;
    direct = 1;
    if (flags & EV_HEARTBEAT) continue;   /* only key changes count */
    if (flags & EV_PRESSED) {
      if (!k[j].saw_down) k[j].pend = 1;
    } else {
      if (k[j].pend && !k[j].saw_down && !k[j].tap) k[j].tap = 1;
      k[j].pend = 0;
    }
  }
  if (direct) return 0;

  *remote = 1;
  keybit_remote_t *x = remote_slot(r, rn, uid);
  if (!x) return 0;
  int changed = 0;
  uint8_t down = flags & EV_PRESSED;
  uint8_t back = (flags & EV_BACKWARDS) != 0;
  /* Down and up again before the keys task saw the down: keep it as a tap. */
  if (down && !x->key_down) x->fresh_down = 1;
  if (!down && x->key_down && x->fresh_down && !x->tap) x->tap = 1;
  if (x->uid != uid || !x->present || x->key_down != down || x->backwards != back) {
    x->dirty = 1;
    changed = 1;
  }
  x->uid = uid;
  x->present = 1;
  x->key_down = down;
  x->backwards = back;
  x->depth = flags >> 4;
  x->via = k[i].addr;
  if (size == EV_SIZE11 && x8[10] != '?') {
    uint32_t parent = uid32(&x8[5]);
    if (parent != x->parent || x8[9] != x->pside || x8[10] != x->face) {
      x->parent = parent;
      x->pside = (char)x8[9];
      x->face = (char)x8[10];
      x->moved = 1;
      x->dirty = 1;
      changed = 1;
    }
  }
  x->last_seen = HAL_GetTick();
  return changed;
}

/* Drain one touching Keybit's queued events (before v12). Returns 1 if any
 * entry changed; *remote is set when they included a chained Keybit's. */
static int relay_one(I2C_HandleTypeDef *i2c, keybit_info_t *k, int n, int i,
                     keybit_remote_t *r, int *rn, int *remote)
{
  int changed = 0;
  uint8_t count;
  if (!read_regs(i2c, k[i].addr, REG_EV_COUNT, &count, 1)) goto done;
  k[i].last_ok = HAL_GetTick();
  if (count == 0) goto done;
  if (count > EV_BATCH) count = EV_BATCH;
  /* v11 Keybits also say where each Keybit is; older ones only uid + flags. */
  int size = k[i].fw >= 11 ? EV_SIZE11 : 5;
  uint8_t ev[EV_BATCH * EV_SIZE11];
  if (!read_regs(i2c, k[i].addr, size == 5 ? REG_EV_DATA : REG_EV_DATA11, ev, (uint16_t)(count * size))) goto done;
  write_regs(i2c, k[i].addr, REG_EV_POP, &count, 1);
  for (int e = 0; e < count; e++) changed |= take_event(k, n, i, &ev[e * size], size, r, rn, remote);
done:
  /* Its next batch's events are measured against the polls until then. */
  k[i].saw_down = k[i].key_down;
  return changed;
}

static uint8_t crc8(uint8_t crc, uint8_t v)
{
  crc ^= v;
  for (int i = 0; i < 8; i++) crc = (uint8_t)(crc & 0x80 ? (crc << 1) ^ 0x07 : crc << 1);
  return crc;
}

/* v12: key state, event count and the first events in one read. Each event
 * is 5 bytes, or 11 with its place (EV_WHERE), then a CRC-8. Reading from
 * 0x80 | n tells the Keybit we have everything before event n, so it drops
 * those; asking twice drops nothing more, and an event that came through
 * damaged is just sent again. Returns 1 if anything changed; *ok is set if
 * the first read worked. */
static int stream_one(I2C_HandleTypeDef *i2c, keybit_info_t *k, int n, int i,
                      keybit_remote_t *r, int *rn, int *remote, int *ok)
{
  int changed = 0;
  uint16_t len = 3 + STREAM_EV + 1;   /* most often: nothing, or one key event */
  for (int round = 0, tries = 0; round < 3; round++) {
    uint8_t b[3 + EV_BATCH * (EV_SIZE11 + 1)];
    uint8_t reg = k[i].num_ok ? (uint8_t)(0x80 | k[i].next_num) : REG_STREAM;
    int good = read_regs(i2c, k[i].addr, reg, b, len) && (b[1] >> 4) == (~b[0] & 0x0F);
    if (!good) {
      /* busy with a link frame for a moment, or noise: one more try */
      if (round == 0 && !tries++) { round--; continue; }
      break;
    }
    if (round == 0) {
      *ok = 1;
      k[i].last_ok = HAL_GetTick();
      uint8_t down = b[0] & 1;
      if (down) { k[i].saw_down = 1; k[i].pend = 0; }
      if (down != k[i].key_down) { k[i].key_down = down; changed = 1; }
    }
    int count = b[1] & 0x0F, e = 0, pos = 3, damaged = 0;
    if (count > EV_BATCH) count = EV_BATCH;
    while (e < count && pos + STREAM_EV < len) {
      int size = b[pos + 4] & EV_WHERE ? EV_SIZE11 : STREAM_EV;
      if (pos + size + 1 > len) break;   /* not all of it in this read */
      uint8_t crc = 0;
      for (int j = 0; j < size; j++) crc = crc8(crc, b[pos + j]);
      if (crc != b[pos + size]) { damaged = 1; break; }
      changed |= take_event(k, n, i, &b[pos], size, r, rn, remote);
      pos += size + 1;
      e++;
    }
    k[i].next_num = (uint8_t)((b[2] + e) & 0x7F);
    k[i].num_ok = 1;
    if (e >= count || damaged) break;
    /* the rest: room for the first to carry its place, 6 bytes for the others */
    len = (uint16_t)(3 + EV_SIZE11 + 1 + (count - e - 1) * (STREAM_EV + 1));
  }
  k[i].saw_down = k[i].key_down;
  return changed;
}

static int relay_age(keybit_remote_t *r, int rn)
{
  int changed = 0;
  uint32_t now = HAL_GetTick();
  for (int i = 0; i < rn; i++) {
    if (r[i].present && now - r[i].last_seen > KEYBIT_REMOTE_TIMEOUT_MS) {
      r[i].present = 0;
      r[i].key_down = 0;
      r[i].backwards = 0;
      r[i].dirty = 1;
      changed = 1;
    }
  }
  return changed;
}

int keybit_bus_poll_relay(I2C_HandleTypeDef *i2c, keybit_info_t *k, int n,
                          keybit_remote_t *r, int *rn)
{
  int changed = 0, remote = 0;
  for (int i = 0; i < n; i++) changed |= relay_one(i2c, k, n, i, r, rn, &remote);
  return changed | relay_age(r, *rn);
}

/* ------------------------------------------------------------------------ */
/* The whole side bus, a little at a time from the main loop                */

/* At 20 kHz one register read is ~2 ms, and every Keybit on the bus has to
 * sit through every transfer in its software I2C target. Keeping the bus
 * close to 100% busy leaves a Keybit no quiet gap to run a link frame to
 * its neighbour, so chained Keybits never join. */
#ifndef KEYS_MS           /* the simulator (firmware/sim) tries other values */
#define KEYS_MS        15
#endif
#ifndef RELAY_CHAIN_MS
#define RELAY_CHAIN_MS 15   /* a Keybit that relays chained ones: their key events */
#endif
#define RELAY_MS       60   /* a Keybit with nothing behind it: only its own short taps */
#define CHAIN_FORGET_MS 3000
#define SCAN_MS        500
#ifndef SCAN_GAP_MS
#define SCAN_GAP_MS    2    /* between scan probes: room for link frames */
#endif
/* v12 Keybits are read one per slot, round robin. A slot is about twice
 * a short stream read (~90 bit times), so the bus stays at most half busy
 * and links get their gaps: 2 ms at 100 kHz, 4 at 50, 9 at 20. */
#define STREAM_SLOT_MS(khz) ((180u + (khz) - 1) / (khz))
#define MISSES_MAX     2    /* scans a Keybit may miss in a row before it counts as gone, */
#define GONE_MS        1000 /* and no read of any kind worked for this long */

static struct {
  uint32_t keys_at, scan_at, probe_at, stream_at;
  uint8_t stream_next;
  uint32_t relay_at[128], chain_at[128];  /* by address */
  uint8_t chained[128];                   /* relayed a chained Keybit lately */
  uint8_t cursor;                         /* next address to look at, 0 = between scans */
  keybit_info_t found[KEYBIT_MAX];
  int nfound, assigned;
  uint8_t missed[KEYBIT_MAX];
} bs;

/* Look at one address for the running scan. A Keybit we already know only
 * needs its head read: the UID bits 0-31 in it tell it's still the same one,
 * and two Keybits sharing the address still arbitrate on them. */
static int scan_one(I2C_HandleTypeDef *i2c, uint8_t a, keybit_info_t *k, int *n)
{
  if (a == KEYBIT_DEFAULT_ADDR || a == LINK_ADDR || a == CHARGER_ADDR || bs.nfound >= KEYBIT_MAX) return 0;
  const keybit_info_t *known = NULL;
  for (int i = 0; i < *n; i++) if (k[i].addr == a) known = &k[i];
  if (!known && !is_ready(i2c, a)) return 0;

  /* It answered, so a failed read is most likely it being busy with a
   * link frame for a moment: try again rather than wait a whole scan. */
  uint8_t head[10];
  int got = 0;
  for (int t = 0; t < 3 && !got; t++) got = read_regs(i2c, a, REG_MAGIC, head, sizeof(head)) && head[0] == MAGIC;
  if (!got) return 0;
  keybit_info_t *f = &bs.found[bs.nfound];
  if (known && uid32(known->uid) == uid32(&head[2])) {
    *f = *known;   /* keeps its tap tracking */
  } else {
    memset(f, 0, sizeof(*f));
    got = 0;
    for (int t = 0; t < 3 && !got; t++) got = read_regs(i2c, a, REG_UID, f->uid, sizeof(f->uid));
    if (!got) return 0;
  }
  f->addr = a;
  f->last_ok = HAL_GetTick();
  f->fw = head[1];
  f->keybit_side = (char)head[7];
  f->at = keyword_side(f->keybit_side);
  f->backwards = f->fw >= 6 ? head[9] & 0x0F : 0;  /* reg 0x09 is new in v6 */
  if (!known) f->key_down = head[6] & 1;
  bs.nfound++;

  /* One new to the list, or back on another address, is polled from now
   * on rather than from the end of the scan, most of a second away. */
  int i = 0;
  while (i < *n && memcmp(k[i].uid, f->uid, sizeof(f->uid)) != 0) i++;
  if (i < *n) {
    if (k[i].addr != a) { k[i].addr = a; k[i].num_ok = 0; listed[a] = 1; }
    return 0;
  }
  if (*n >= KEYBIT_MAX) return 0;
  k[(*n)++] = *f;
  listed[a] = 1;
  return 1;
}

/* The scan is done: the found list becomes the list, keeping a Keybit that
 * missed it unless it missed MISSES_MAX scans in a row and nothing else
 * reached it for GONE_MS either (one busy serving a link misses a read now
 * and then), so it doesn't flicker in and out. */
static int scan_commit(keybit_info_t *k, int *n)
{
  uint32_t now = HAL_GetTick();
  uint8_t missed[KEYBIT_MAX] = {0};
  int m = bs.nfound, joined = bs.assigned > 0;
  for (int i = 0; i < *n && m < KEYBIT_MAX; i++) {
    int here = 0;
    for (int j = 0; j < bs.nfound && !here; j++)
      here = memcmp(bs.found[j].uid, k[i].uid, sizeof(k[i].uid)) == 0;
    if (!here && (bs.missed[i] < MISSES_MAX || now - k[i].last_ok < GONE_MS)) {
      bs.found[m] = k[i];
      missed[m++] = (uint8_t)(bs.missed[i] < 255 ? bs.missed[i] + 1 : 255);
    }
  }
  /* A Keybit already on the list keeps its live entry (key state, taps,
   * event number): the copy taken when the scan passed its address can be
   * most of a scan old, and putting it back brought back a key that had
   * been let go since. Only what a scan reads is refreshed. */
  for (int j = 0; j < bs.nfound; j++) {
    int known = 0;
    for (int i = 0; i < *n && !known; i++)
      if (memcmp(bs.found[j].uid, k[i].uid, sizeof(k[i].uid)) == 0) {
        known = 1;
        keybit_info_t *f = &bs.found[j];
        uint8_t fw = f->fw, backwards = f->backwards, addr = f->addr;
        char side = f->keybit_side, at = f->at;
        *f = k[i];
        if (f->addr != addr) f->num_ok = 0;   /* it lost its address and got another */
        f->addr = addr;
        f->fw = fw;
        f->backwards = backwards;
        f->keybit_side = side;
        f->at = at;
      }
    joined |= !known;
  }
  memcpy(k, bs.found, m * sizeof(k[0]));
  memcpy(bs.missed, missed, sizeof(missed));
  *n = m;
  memset(listed, 0, sizeof(listed));
  for (int i = 0; i < m; i++) listed[k[i].addr & 0x7F] = 1;
  return joined;
}

int keybit_bus_service(I2C_HandleTypeDef *i2c, keybit_info_t *k, int *n, keybit_remote_t *r, int *rn)
{
  int flags = 0, used = 0, all12 = *n > 0, any12 = 0;
  uint32_t now = HAL_GetTick();
  for (int i = 0; i < *n; i++) { all12 &= k[i].fw >= 12; any12 |= k[i].fw >= 12; }
  unsigned khz = all12 ? fast_khz() : SLOW_KHZ;
  bus_khz(i2c, khz);

  int had = 0, has = 0;
  for (int i = 0; i < *rn; i++) had += r[i].present;
  if (any12 && now - bs.stream_at >= STREAM_SLOT_MS(khz)) {
    bs.stream_at = now;
    for (int tries = 0; tries < *n; tries++) {
      int i = bs.stream_next++ % *n;
      if (k[i].fw < 12) continue;
      int remote = 0, ok = 0;
      if (stream_one(i2c, k, *n, i, r, rn, &remote, &ok)) flags |= KEYBIT_CHANGED;
      if (all12) fast_result(ok);
      used = 1;
      break;
    }
  }

  if (now - bs.keys_at >= KEYS_MS) {
    bs.keys_at = now;
    if (keybit_bus_poll_keys(i2c, k, *n)) flags |= KEYBIT_CHANGED;
    used = 1;
  }

  for (int i = 0; i < *n; i++) {
    uint8_t a = k[i].addr & 0x7F;
    if (k[i].fw >= 12) continue;
    if (bs.chained[a] && now - bs.chain_at[a] > CHAIN_FORGET_MS) bs.chained[a] = 0;
    if (now - bs.relay_at[a] < (bs.chained[a] ? RELAY_CHAIN_MS : RELAY_MS)) continue;
    bs.relay_at[a] = now;
    int remote = 0;
    if (relay_one(i2c, k, *n, i, r, rn, &remote)) flags |= KEYBIT_CHANGED;
    if (remote) { bs.chained[a] = 1; bs.chain_at[a] = now; }
    used = 1;
  }
  if (relay_age(r, *rn)) flags |= KEYBIT_CHANGED;
  for (int i = 0; i < *rn; i++) has += r[i].present;
  if (has > had) flags |= KEYBIT_JOINED;

  /* One address per pass, only on a pass without other transfers and at
   * most every SCAN_GAP_MS, so a scan never holds up the key polls for
   * more than one read and leaves the Keybits gaps for their links. */
  if (used) return flags;
  bus_khz(i2c, SLOW_KHZ);
  if (!bs.cursor) {
    if (now - bs.scan_at < SCAN_MS) return flags;
    bs.scan_at = now;
    bs.assigned = keybit_bus_assign(i2c);
    bs.nfound = 0;
    bs.cursor = ADDR_FIRST;
    return flags;
  }
  if (now - bs.probe_at < SCAN_GAP_MS) return flags;
  bs.probe_at = now;
  if (scan_one(i2c, bs.cursor, k, n)) flags |= KEYBIT_JOINED | KEYBIT_CHANGED;
  if (++bs.cursor > ADDR_LAST) {
    bs.cursor = 0;
    if (scan_commit(k, n)) flags |= KEYBIT_JOINED;
    flags |= KEYBIT_CHANGED;
  }
  return flags;
}

int keybit_bus_read(I2C_HandleTypeDef *i2c, uint8_t addr7, uint8_t reg, uint8_t *buf, uint8_t len)
{
  if (keybit_bus_jammed()) return 0;
  return read_regs(i2c, addr7, reg, buf, len);
}

int keybit_bus_write(I2C_HandleTypeDef *i2c, uint8_t addr7, uint8_t reg, const uint8_t *buf, uint8_t len)
{
  if (keybit_bus_jammed()) return 0;
  return write_regs(i2c, addr7, reg, buf, len);
}
