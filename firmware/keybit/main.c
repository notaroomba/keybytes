/*
 * Keybit firmware (CH32V003F4U6).
 *
 * Each of the four side buses is its own bit-banged I2C pair:
 *
 *   bus 1  SDA PC4 / SCL PD2  south
 *   bus 2  SDA PC1 / SCL PC2  east
 *   bus 3  SDA PD4 / SCL PD5  west
 *   bus 4  SDA PA1 / SCL PA2  north
 *
 * (Sides as seen in keybit.kicad_pcb.)
 *
 * Addressing: the keyword hands addresses out. A Keybit with no address
 * answers on KEYBIT_DEFAULT_ADDR. The keyword reads its 96-bit UID there
 * (several new Keybits answering at once arbitrate down to one UID), then
 * writes that UID plus a free address to the ASSIGN registers. Only the
 * Keybit whose UID matches takes the address, and it keeps it in the last
 * flash page across power cycles. If two Keybits ever share an address,
 * the one that loses arbitration on a UID read drops its address and asks
 * again.
 *
 * Relaying: only Keybits touching the keyword are on its bus. The rest sit
 * behind private two-Keybit links, so key state hops inward:
 *   - Every Keybit has a depth: 1 when the keyword talks to it directly,
 *     otherwise one more than the shallowest neighbour that polls it.
 *     Depth only comes from being polled, so when a Keybit is pulled out
 *     everything behind it goes quiet within FRESH_MS instead of looping.
 *   - A Keybit with a depth polls each other side as I2C controller (at
 *     LINK_ADDR) unless the neighbour there is no deeper than it is. Each
 *     poll tells the child our depth, reads the child's depth, and takes
 *     up to 8 queued events.
 *   - Events are (UID low 32 bits, flags): key changes and a heartbeat
 *     about once a second. Each Keybit queues its own and forwards its
 *     children's, so they drain towards the keyword.
 *   A side where we ever see an address that isn't LINK_ADDR is the
 *   keyword's shared bus, and we never act as controller there.
 *
 * Registers (reads and writes auto-increment from the pointer):
 *   0x00       0x4B ('K') magic
 *   0x01       firmware version
 *   0x02-0x05  UID bits 0..31 (same as 0x10-0x13)
 *   0x06       key state, bit 0 = pressed
 *   0x07       side this request came in on: 'N', 'E', 'S' or 'W'
 *   0x08       our address, 0 if none yet
 *   0x10-0x1B  96-bit UID, little endian
 *   0x20-0x2B  ASSIGN: UID to match (write only)
 *   0x2C       ASSIGN: new address, 0 = forget it. Writing this applies it.
 *   0x50       (write) depth of the Keybit polling us over a link
 *   0x51       our depth, 0xFF if we can't reach the keyword yet
 *   0x52       events ready to read, at most 8
 *   0x53-0x7A  up to 8 events, 5 bytes each: UID bits 0..31, then flags
 *              (bit 0 pressed, bit 1 heartbeat, bits 4-7 origin depth)
 *   0x7B       (write) n: drop the first n events, they were received
 *
 * PD1 stays SWIO so the keyword can always reflash us. Don't remap it.
 */
#include "ch32fun.h"
#include "soft_i2c.h"
#include <string.h>

#define FW_VERSION          3
#define KEYBIT_DEFAULT_ADDR 0x30
#define LINK_ADDR           0x31  /* Keybit-to-Keybit links; the keyword never assigns it */
#define BTN_PIN             PD6   /* MX switch to GND */
#define DEBOUNCE            200   /* main loop passes */

#define DEPTH_NONE          0xFF
#define FRESH_MS            600   /* a neighbour's depth counts this long */
#define POLL_MS             5     /* child poll interval per side */
#define PEER_RECHECK_MS     2000
#define HEARTBEAT_MS        1000
#define QUEUE_LEN           32
#define BATCH               8

/* Last 64-byte page. The SWIO flasher only rewrites pages the image uses,
 * so this survives reflashing. */
#define CONFIG_PAGE  0x08003FC0u
#define CONFIG_MAGIC 0x4B42u

#define EV_PRESSED   0x01
#define EV_HEARTBEAT 0x02

typedef struct {
  uint16_t magic;
  uint8_t addr;
  uint8_t addr_inv;
} config_t;

typedef struct {
  uint8_t uid[4];
  uint8_t flags;
} event_t;

typedef struct {
  /* Learned when the neighbour polls us: it's a parent (or a peer). Only
   * this counts towards our depth, so a child can never root us and no
   * loop can keep itself alive after the real path is gone. */
  uint8_t parent_depth;  /* DEPTH_NONE if it isn't polling us */
  uint32_t parent_seen;
  /* Learned when we poll the neighbour. */
  uint8_t child_depth;   /* DEPTH_NONE if unknown */
  uint32_t child_seen;
  uint8_t keyword;       /* this side is the keyword's shared bus */
  uint32_t next_poll;    /* tick of our next controller poll here */
} side_t;

static soft_i2c_bus_t buses[4] = {
  { .sda = PC4, .scl = PD2, .side = 'S' },
  { .sda = PC1, .scl = PC2, .side = 'E' },
  { .sda = PD4, .scl = PD5, .side = 'W' },
  { .sda = PA1, .scl = PA2, .side = 'N' },
};
static side_t sides[4];

static uint8_t uid[12];
static volatile uint8_t key_down;
static uint8_t my_addr;           /* 0 = unassigned */
static uint8_t my_depth = DEPTH_NONE;
static uint8_t assign_buf[13];
static int16_t save_addr = -1;    /* address to write to flash, -1 = none */

static event_t queue[QUEUE_LEN];
static uint8_t q_head, q_count;

/* ------------------------------------------------------------------------ */
/* Time: SysTick runs freely; compare ticks with unsigned subtraction.      */

static inline uint32_t ticks(void) { return SysTick->CNT; }
#define MS(n) ((uint32_t)(n) * DELAY_MS_TIME)
static inline int elapsed(uint32_t since, uint32_t span) { return ticks() - since >= span; }

static int side_index(soft_i2c_bus_t *bus) { return (int)(bus - buses); }

/* ------------------------------------------------------------------------ */
/* Event queue                                                              */

static event_t *queue_at(int i) { return &queue[(q_head + i) % QUEUE_LEN]; }

static void queue_pop(int n)
{
  if (n > q_count) n = q_count;
  q_head = (uint8_t)((q_head + n) % QUEUE_LEN);
  q_count = (uint8_t)(q_count - n);
}

/* When full, the oldest event goes; heartbeats repair anything lost. */
static void queue_push(const event_t *e)
{
  if (q_count == QUEUE_LEN) queue_pop(1);
  *queue_at(q_count) = *e;
  q_count++;
}

static void queue_own(uint8_t flags)
{
  event_t e;
  memcpy(e.uid, uid, 4);
  uint8_t d = my_depth > 15 ? 15 : my_depth;
  e.flags = (uint8_t)(flags | (key_down ? EV_PRESSED : 0) | (d << 4));
  queue_push(&e);
}

/* ------------------------------------------------------------------------ */
/* Address storage                                                          */

static void load_config(void)
{
  const config_t *c = (const config_t *)CONFIG_PAGE;
  if (c->magic == CONFIG_MAGIC && (uint8_t)~c->addr == c->addr_inv &&
      c->addr >= 0x08 && c->addr <= 0x77 &&
      c->addr != KEYBIT_DEFAULT_ADDR && c->addr != LINK_ADDR)
    my_addr = c->addr;
}

/* Erase the config page and write it back with addr. About 6 ms, so it runs
 * from the main loop after the I2C transaction, not inside it. */
static void store_config(uint8_t addr)
{
  FLASH->KEYR = FLASH_KEY1;
  FLASH->KEYR = FLASH_KEY2;
  FLASH->MODEKEYR = FLASH_KEY1;
  FLASH->MODEKEYR = FLASH_KEY2;
  if (FLASH->CTLR & 0x8080) return;  /* still locked */

  FLASH->CTLR = CR_PAGE_ER;
  FLASH->ADDR = CONFIG_PAGE;
  FLASH->CTLR = CR_STRT_Set | CR_PAGE_ER;
  while (FLASH->STATR & FLASH_STATR_BSY) { }

  if (addr) {
    config_t c = { CONFIG_MAGIC, addr, (uint8_t)~addr };
    uint32_t words[16];
    memset(words, 0xFF, sizeof(words));
    memcpy(words, &c, sizeof(c));

    volatile uint32_t *dst = (volatile uint32_t *)CONFIG_PAGE;
    FLASH->CTLR = CR_PAGE_PG;
    FLASH->CTLR = CR_BUF_RST | CR_PAGE_PG;
    FLASH->ADDR = CONFIG_PAGE;
    while (FLASH->STATR & FLASH_STATR_BSY) { }
    for (int i = 0; i < 16; i++) {
      dst[i] = words[i];
      FLASH->CTLR = CR_PAGE_PG | FLASH_CTLR_BUF_LOAD;
      while (FLASH->STATR & FLASH_STATR_BSY) { }
    }
    FLASH->CTLR = CR_PAGE_PG | CR_STRT_Set;
    while (FLASH->STATR & FLASH_STATR_BSY) { }
  }
  FLASH->CTLR = CR_LOCK_Set;
}

/* ------------------------------------------------------------------------ */
/* I2C target callbacks                                                     */

static void polled_by(int s, uint8_t depth)
{
  sides[s].parent_depth = depth;
  sides[s].parent_seen = ticks();
}

static int match_addr(uint8_t addr, soft_i2c_bus_t *bus)
{
  int s = side_index(bus);
  if (addr == LINK_ADDR) return !sides[s].keyword;

  /* Anything else only happens on the keyword's bus. */
  sides[s].keyword = 1;
  int mine = my_addr ? addr == my_addr : addr == KEYBIT_DEFAULT_ADDR;
  if (mine) polled_by(s, 0);
  return mine;
}

static uint8_t read_reg(uint8_t reg, soft_i2c_bus_t *bus)
{
  if (reg >= 0x02 && reg <= 0x05) return uid[reg - 0x02];
  if (reg >= 0x10 && reg <= 0x1B) return uid[reg - 0x10];
  if (reg >= 0x53 && reg < 0x53 + BATCH * 5) {
    int i = (reg - 0x53) / 5, b = (reg - 0x53) % 5;
    if (i >= q_count) return 0xFF;
    const event_t *e = queue_at(i);
    return b < 4 ? e->uid[b] : e->flags;
  }
  switch (reg) {
  case 0x00: return 0x4B;
  case 0x01: return FW_VERSION;
  case 0x06: return key_down;
  case 0x07: return (uint8_t)bus->side;
  case 0x08: return my_addr;
  case 0x51: return my_depth;
  case 0x52: return q_count < BATCH ? q_count : BATCH;
  default:   return 0xFF;
  }
}

static void write_reg(uint8_t reg, uint8_t value, soft_i2c_bus_t *bus)
{
  if (reg == 0x50) { polled_by(side_index(bus), value); return; }
  if (reg == 0x7B) { queue_pop(value); return; }
  if (reg < 0x20 || reg > 0x2C) return;
  assign_buf[reg - 0x20] = value;
  if (reg != 0x2C) return;

  uint8_t addr = assign_buf[12];
  if (memcmp(assign_buf, uid, 12) != 0) return;  /* meant for another Keybit */
  if (addr && (addr < 0x08 || addr > 0x77 || addr == KEYBIT_DEFAULT_ADDR || addr == LINK_ADDR)) return;
  my_addr = addr;
  save_addr = addr;
}

/* Another Keybit answered on our address with a lower UID: we're the
 * duplicate, so go back to the default address and get a new one. */
static void lost_arbitration(uint8_t addr)
{
  if (my_addr && addr == my_addr) {
    my_addr = 0;
    save_addr = 0;
  }
}

static const soft_i2c_target_t target = {
  .match = match_addr,
  .read = read_reg,
  .write = write_reg,
  .lost = lost_arbitration,
};

/* ------------------------------------------------------------------------ */
/* Relay                                                                    */

static void update_depth(void)
{
  uint8_t best = DEPTH_NONE;
  for (int s = 0; s < 4; s++) {
    side_t *sd = &sides[s];
    if (sd->parent_depth == DEPTH_NONE) continue;
    if (elapsed(sd->parent_seen, MS(FRESH_MS))) { sd->parent_depth = DEPTH_NONE; continue; }
    if (sd->parent_depth < best) best = sd->parent_depth;
  }
  my_depth = best == DEPTH_NONE || best >= 0xFE ? DEPTH_NONE : (uint8_t)(best + 1);
}

/* Poll the Keybit on side s: tell it our depth, read its depth and up to
 * BATCH of its events, then tell it how many we kept. */
static void poll_child(int s)
{
  soft_i2c_bus_t *b = &buses[s];
  uint8_t depth = 0, count = 0;
  event_t got[BATCH];
  int n = 0, ok = 0;

  if (!soft_i2c_start(b)) return;
  if (soft_i2c_write(b, LINK_ADDR << 1) == 1 && soft_i2c_write(b, 0x50) == 1 &&
      soft_i2c_write(b, my_depth) == 1 && soft_i2c_restart(b) &&
      soft_i2c_write(b, (LINK_ADDR << 1) | 1) == 1 &&
      soft_i2c_read(b, &depth, 1) && soft_i2c_read(b, &count, 1)) {
    int room = QUEUE_LEN - q_count;
    if (count > BATCH) count = BATCH;
    n = count < room ? count : room;
    ok = 1;
    for (int i = 0; i < n && ok; i++)
      for (int j = 0; j < 5 && ok; j++) {
        uint8_t v;
        ok = soft_i2c_read(b, &v, !(i == n - 1 && j == 4));
        if (j < 4) got[i].uid[j] = v; else got[i].flags = v;
      }
    if (n == 0) { uint8_t dummy; ok = soft_i2c_read(b, &dummy, 0); }
  }
  soft_i2c_stop(b);
  if (!ok) return;

  sides[s].child_depth = depth;
  sides[s].child_seen = ticks();
  if (n == 0) return;
  for (int i = 0; i < n; i++) queue_push(&got[i]);

  if (soft_i2c_start(b)) {
    if (soft_i2c_write(b, LINK_ADDR << 1) == 1 && soft_i2c_write(b, 0x7B) == 1)
      soft_i2c_write(b, (uint8_t)n);
    soft_i2c_stop(b);
  }
}

static void relay(void)
{
  static uint32_t boot;
  static uint8_t started;
  if (!started) { boot = ticks(); started = 1; }
  /* Listen first so the keyword's bus is recognised before we ever drive
   * a side as controller. */
  if (!elapsed(boot, MS(FRESH_MS))) return;

  update_depth();
  if (my_depth == DEPTH_NONE) return;

  for (int s = 0; s < 4; s++) {
    side_t *sd = &sides[s];
    if (sd->keyword) continue;
    if (sd->parent_depth != DEPTH_NONE) continue;  /* it polls us */
    /* A peer at the same depth or shallower isn't our child; look again
     * now and then in case that changed. */
    if (sd->child_depth != DEPTH_NONE && sd->child_depth <= my_depth &&
        !elapsed(sd->child_seen, MS(PEER_RECHECK_MS))) continue;
    if ((int32_t)(ticks() - sd->next_poll) < 0) continue;
    /* A little UID-based jitter so two Keybits don't keep polling each
     * other in lockstep. */
    sd->next_poll = ticks() + MS(POLL_MS) + (uid[0] & 3) * DELAY_MS_TIME / 2;
    poll_child(s);
    buses[s].prev_sda = buses[s].prev_scl = 1;
  }
}

/* ------------------------------------------------------------------------ */

/* Counts towards the pin's level and only flips once it gets all the way. */
static void debounce_key(void)
{
  static int16_t count;
  if (!funDigitalRead(BTN_PIN)) { if (count < DEBOUNCE) count++; }
  else if (count > 0) count--;

  uint8_t was = key_down;
  if (count == DEBOUNCE) key_down = 1;
  else if (count == 0) key_down = 0;
  if (key_down != was) queue_own(0);
}

static void heartbeat(void)
{
  static uint32_t last;
  if (my_depth == DEPTH_NONE || !elapsed(last, MS(HEARTBEAT_MS + uid[1]))) return;
  last = ticks();
  queue_own(EV_HEARTBEAT);
}

int main(void)
{
  SystemInit();
  funGpioInitAll();

  uint32_t ids[3] = { ESIG->UNIID1, ESIG->UNIID2, ESIG->UNIID3 };
  memcpy(uid, ids, sizeof(uid));
  load_config();
  for (int s = 0; s < 4; s++) sides[s].parent_depth = sides[s].child_depth = DEPTH_NONE;

  funPinMode(BTN_PIN, GPIO_CFGLR_IN_PUPD);
  funDigitalWrite(BTN_PIN, FUN_HIGH);  /* pull-up */

  for (int i = 0; i < 4; i++) soft_i2c_init(&buses[i]);

  for (;;) {
    for (int i = 0; i < 4; i++) soft_i2c_poll(&buses[i], &target);
    debounce_key();
    relay();
    heartbeat();
    if (save_addr >= 0) {
      store_config((uint8_t)save_addr);
      save_addr = -1;
    }
  }
}
