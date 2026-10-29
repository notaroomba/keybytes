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
 * behind private two-Keybit links, which don't use I2C but the protocol in
 * link.h, so key state hops inward:
 *   - Every Keybit has a depth: 1 when the keyword talks to it directly,
 *     otherwise one more than the shallowest neighbour that polls it.
 *     Depth only comes from being polled, so when a Keybit is pulled out
 *     everything behind it goes quiet within FRESH_MS instead of looping.
 *   - A Keybit with a depth polls each other side over the link unless the
 *     neighbour there is no deeper than it is: about every CHILD_POLL_MS,
 *     and at once when the child rings the doorbell because it has events.
 *     Each poll tells the child our depth, reads the child's depth, and
 *     takes up to 8 queued events.
 *   - Events are (UID low 32 bits, flags): key changes and a heartbeat
 *     about once a second. Each Keybit queues its own and forwards its
 *     children's, so they drain towards the keyword.
 *   A side where we see an I2C address is the keyword's shared bus. We
 *   never start a link frame there, and only answer one on a side that
 *   isn't known to be either after the line has been held for LINK_REQ_US,
 *   which I2C never does.
 *
 * Registers (reads and writes auto-increment from the pointer):
 *   0x00       0x4B ('K') magic
 *   0x01       firmware version
 *   0x02-0x05  UID bits 0..31 (same as 0x10-0x13)
 *   0x06       key state, bit 0 = pressed
 *   0x07       side this request came in on: 'N', 'E', 'S' or 'W'
 *   0x08       our address, 0 if none yet
 *   0x09       sides with a Keybit on backwards: bit 0 N, 1 E, 2 S, 3 W
 *   0x10-0x1B  96-bit UID, little endian
 *   0x20-0x2B  ASSIGN: UID to match (write only)
 *   0x2C       ASSIGN: new address, 0 = forget it. Writing this applies it.
 *   0x30-0x4F  link diagnostics, 8 counters per side (see diag below)
 *   0x51       our depth, 0xFF if we can't reach the keyword yet
 *   0x52       events ready to read, at most 8
 *   0x53-0x7A  up to 8 events, 5 bytes each: UID bits 0..31, then flags
 *              (bit 0 pressed, bit 1 heartbeat, bit 2 a neighbour is on
 *              backwards, bits 4-7 origin depth)
 *   0x7B       (write) n: drop the first n events, they were received
 *
 * PD1 stays SWIO so the keyword can always reflash us. Don't remap it.
 */
#include "ch32fun.h"
#include "soft_i2c.h"
#include "link.h"
#include <string.h>

#define FW_VERSION          10
#define KEYBIT_DEFAULT_ADDR 0x30
#define LINK_ADDR           0x31  /* reserved: the keyword never assigns it */
#define BTN_PIN             PD6   /* MX switch to GND */
#define DEBOUNCE            200   /* main loop passes */

#define DEPTH_NONE          0xFF
#define FRESH_MS            600   /* a neighbour's depth counts this long */
#define CHILD_POLL_MS       100   /* keeps a child's depth fresh; events come by doorbell */
#define PROBE_MS            100   /* look for a new neighbour on an empty side */
#define PEER_RECHECK_MS     2000
#define LINK_FRESH_MS       1000  /* a side counts as a link this long after a frame */
#define LINK_REQ_US         50    /* CK held low this long on an unknown side = link request */
#define HEARTBEAT_MS        1000
#define BACKWARDS_MS        200   /* DT low, CK high this long = neighbour on backwards */
#define QUEUE_LEN           32
#define BATCH               8

/* Last 64-byte page. The SWIO flasher only rewrites pages the image uses,
 * so this survives reflashing. */
#define CONFIG_PAGE  0x08003FC0u
#define CONFIG_MAGIC 0x4B42u

#define EV_PRESSED   0x01
#define EV_HEARTBEAT 0x02
#define EV_BACKWARDS 0x04

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
  uint8_t keyword;       /* the keyword's shared bus */
  uint8_t link;          /* a link frame ran here at link_seen */
  uint32_t link_seen;
  uint8_t ck_low;        /* CK low on an unknown side since ck_low_at */
  uint32_t ck_low_at;
  uint8_t ringing;       /* we're holding DT low as a doorbell */
  uint32_t polled_at;    /* tick of our last link poll here */
  uint32_t poll_wait;    /* how long after polled_at the next one is due */
  uint8_t held;          /* DT low with CK high since held_at */
  uint32_t held_at;
} side_t;

static soft_i2c_bus_t buses[4] = {
  { .sda = PC4, .scl = PD2, .side = 'S' },
  { .sda = PC1, .scl = PC2, .side = 'E' },
  { .sda = PD4, .scl = PD5, .side = 'W' },
  { .sda = PA1, .scl = PA2, .side = 'N' },
};
static side_t sides[4];

/* Link diagnostics, read over I2C at 0x30 + 8 * side (S, E, W, N): link
 * requests we sent, ones that came back ok, no ack, called off for keyword
 * traffic, bad frames, requests we answered as a child, ones we failed to
 * answer, and probes skipped because the lines weren't idle. Each counts up
 * to 255 and stays there. */
enum { D_REQ, D_OK, D_NO_ACK, D_ABORT, D_BAD, D_SERVED, D_SERVE_FAIL, D_SKIPPED, D_N };
static uint8_t diag[4][D_N];
static void diag_count(int s, int what) { if (diag[s][what] < 255) diag[s][what]++; }
static uint8_t backwards;  /* reg 0x09: sides with a Keybit on backwards */

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

/* Once a side has carried the keyword's I2C it stays the keyword's side
 * until we lose power (moving a Keybit always does). If it expired, a gap
 * in the keyword's polling would make us probe that side: CK low there is
 * SCL low on the keyword's bus, the keyword can't poll through it, so the
 * side never freshens again and the bus stays jammed. */
static int is_keyword(int s)
{
  return sides[s].keyword;
}

static int is_link(int s)
{
  side_t *sd = &sides[s];
  if (sd->link && elapsed(sd->link_seen, MS(LINK_FRESH_MS))) sd->link = 0;
  return sd->link;
}

static link_pins_t pins(int s) { return (link_pins_t){ .dt = buses[s].sda, .ck = buses[s].scl }; }

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
  e.flags = (uint8_t)(flags | (key_down ? EV_PRESSED : 0) | (backwards ? EV_BACKWARDS : 0) | (d << 4));
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
  int mine = my_addr ? addr == my_addr : addr == KEYBIT_DEFAULT_ADDR;
  /* Only the keyword addresses us, so being addressed is what marks its
   * bus. Any byte at all used to, and contact bounce or a link frame from
   * a neighbour being pushed on can clock one in: that side then counted
   * as the keyword's for good and never formed a link. */
  if (mine) {
    sides[s].keyword = 1;
    sides[s].link = 0;
    polled_by(s, 0);
  }
  return mine;
}

static uint8_t read_reg(uint8_t reg, soft_i2c_bus_t *bus)
{
  if (reg >= 0x02 && reg <= 0x05) return uid[reg - 0x02];
  if (reg >= 0x10 && reg <= 0x1B) return uid[reg - 0x10];
  if (reg >= 0x30 && reg < 0x30 + sizeof(diag)) return diag[(reg - 0x30) / D_N][(reg - 0x30) % D_N];
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
  case 0x09: return backwards;
  case 0x51: return my_depth;
  case 0x52: return q_count < BATCH ? q_count : BATCH;
  default:   return 0xFF;
  }
}

static void write_reg(uint8_t reg, uint8_t value, soft_i2c_bus_t *bus)
{
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

static void fill_reply(link_reply_t *r)
{
  r->depth = my_depth;
  r->n = q_count < BATCH ? q_count : BATCH;
  for (int i = 0; i < r->n; i++) {
    const event_t *e = queue_at(i);
    memcpy(&r->ev[i * LINK_EV_SIZE], e->uid, 4);
    r->ev[i * LINK_EV_SIZE + 4] = e->flags;
  }
}

/* Keep serving I2C on the other sides while a link request waits for its
 * ack. Returns 1 (give up on the request) only if the keyword addressed us,
 * so it never waits on us for long; traffic for other Keybits on its bus
 * doesn't cut the request short. */
static int link_side = -1;
static int serve_others(void)
{
  for (int s = 0; s < 4; s++) {
    if (s == link_side || is_link(s)) continue;
    if (soft_i2c_poll(&buses[s], &target)) return 1;
  }
  return 0;
}

static void reset_i2c_edge(int s)
{
  buses[s].prev_sda = funDigitalRead(buses[s].sda);
  buses[s].prev_scl = funDigitalRead(buses[s].scl);
}

/* Parent side: run a link frame with the Keybit on side s. */
static void poll_child(int s, int doorbell)
{
  link_pins_t p = pins(s);
  link_reply_t got;
  uint8_t kept = 0;

  link_side = s;
  int ok = link_poll(&p, doorbell, my_depth, &got, (uint8_t)(QUEUE_LEN - q_count), &kept, serve_others);
  link_side = -1;
  reset_i2c_edge(s);
  if (ok != LINK_SKIPPED) diag_count(s, D_REQ);
  diag_count(s, ok == LINK_OK ? D_OK : ok == LINK_NO_ACK ? D_NO_ACK : ok == LINK_ABORTED ? D_ABORT
                : ok == LINK_BAD_FRAME ? D_BAD : D_SKIPPED);
  if (ok != LINK_OK) return;

  side_t *sd = &sides[s];
  sd->link = 1;
  sd->link_seen = ticks();
  sd->child_depth = got.depth;
  sd->child_seen = ticks();
  for (int i = 0; i < kept; i++) {
    event_t e;
    memcpy(e.uid, &got.ev[i * LINK_EV_SIZE], 4);
    e.flags = got.ev[i * LINK_EV_SIZE + 4];
    queue_push(&e);
  }
}

/* Child side: CK is low on side s. Answer if it's a link request. */
static void answer_link(int s, int listening)
{
  side_t *sd = &sides[s];
  if (funDigitalRead(buses[s].scl)) { sd->ck_low = 0; return; }
  if (is_keyword(s)) return;

  if (!is_link(s)) {
    /* An unknown side might be the keyword's bus mid-transfer, where SCL
     * is never held low this long with SDA high. */
    if (listening || !funDigitalRead(buses[s].sda)) { sd->ck_low = 0; return; }
    if (!sd->ck_low) { sd->ck_low = 1; sd->ck_low_at = ticks(); return; }
    if (!elapsed(sd->ck_low_at, LINK_REQ_US * DELAY_US_TIME)) return;
  }
  sd->ck_low = 0;

  link_pins_t p = pins(s);
  link_reply_t r;
  uint8_t depth, kept;
  fill_reply(&r);
  int ok = link_serve(&p, sd->ringing, &r, &depth, &kept);
  sd->ringing = 0;
  reset_i2c_edge(s);
  diag_count(s, ok ? D_SERVED : D_SERVE_FAIL);
  if (!ok) return;

  sd->link = 1;
  sd->link_seen = ticks();
  polled_by(s, depth);
  queue_pop(kept);
}

/* With events queued, hold DT low towards our parent so it polls us now.
 * Only on a link side, never the keyword's bus: the keyword polls often
 * anyway, and SDA low there would look like an I2C START. */
static void doorbell(void)
{
  int want = -1;
  if (q_count) {
    uint8_t best = DEPTH_NONE;
    for (int s = 0; s < 4; s++)
      if (is_link(s) && !is_keyword(s) && sides[s].parent_depth < best) {
        best = sides[s].parent_depth;
        want = s;
      }
  }
  for (int s = 0; s < 4; s++) {
    side_t *sd = &sides[s];
    if (s == want && !sd->ringing && funDigitalRead(buses[s].scl)) {
      funDigitalWrite(buses[s].sda, FUN_LOW);
      sd->ringing = 1;
    } else if (s != want && sd->ringing) {
      funDigitalWrite(buses[s].sda, FUN_HIGH);
      sd->ringing = 0;
    }
  }
}

static uint8_t jitter(void) { return uid[0] & 7; }

static void relay(int listening)
{
  if (listening) return;
  update_depth();
  if (my_depth == DEPTH_NONE) return;

  for (int s = 0; s < 4; s++) {
    side_t *sd = &sides[s];
    if (is_keyword(s)) continue;
    if (sd->parent_depth != DEPTH_NONE) continue;  /* it polls us */
    if (sd->ringing) continue;

    int child = sd->child_depth != DEPTH_NONE && sd->child_depth > my_depth &&
                !elapsed(sd->child_seen, MS(FRESH_MS));
    /* A ringing child holds DT low with CK high. So does a Keybit turned
     * the wrong way round (its GND on our DT, its 3V3 on our CK), but that
     * side never completes a frame, so it never counts as a link. Never
     * probe it: CK low there shorts the neighbour's rail through our pin,
     * or pushes current into its SCL clamp diode if it's only fed from us. */
    int held = !funDigitalRead(buses[s].sda) && funDigitalRead(buses[s].scl);
    if (held && !is_link(s)) continue;
    int rang = held;
    if (!rang) {
      /* A peer at the same depth or shallower isn't our child; look again
       * now and then in case that changed. */
      if (sd->child_depth != DEPTH_NONE && sd->child_depth <= my_depth &&
          !elapsed(sd->child_seen, MS(PEER_RECHECK_MS))) continue;
      /* Unsigned "time since", not a signed compare with a due tick: SysTick
       * wraps every ~12 min, and a due tick more than ~6 min old (never set,
       * or a side skipped that long) compared as in the future, so the side
       * wasn't probed again for up to 6 minutes. */
      if (!elapsed(sd->polled_at, sd->poll_wait)) continue;
    }
    /* Some UID-based jitter so two Keybits don't keep asking each other
     * in lockstep. */
    sd->polled_at = ticks();
    sd->poll_wait = MS((child ? CHILD_POLL_MS : PROBE_MS) + jitter());
    poll_child(s, rang);
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

/* A Keybit turned the wrong way round puts its GND on our DT and its 3V3
 * on our CK, so that side sits with DT low and CK high and never becomes a
 * link (relay() leaves it alone). Report it in reg 0x09 and on every event
 * we queue, and queue one at once when it changes, so it reaches the
 * keyword and the app. */
static void watch_backwards(int listening)
{
  static const uint8_t bit[4] = { 1u << 2, 1u << 1, 1u << 3, 1u << 0 };  /* S E W N */
  uint8_t mask = 0;
  for (int s = 0; s < 4; s++) {
    side_t *sd = &sides[s];
    int held = !listening && !is_keyword(s) && !is_link(s) &&
               !funDigitalRead(buses[s].sda) && funDigitalRead(buses[s].scl);
    if (!held) { sd->held = 0; continue; }
    if (!sd->held) { sd->held = 1; sd->held_at = ticks(); }
    if (elapsed(sd->held_at, MS(BACKWARDS_MS)) || (backwards & bit[s])) mask |= bit[s];
  }
  if (mask != backwards) {
    backwards = mask;
    if (my_depth != DEPTH_NONE) queue_own(0);
  }
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

  uint32_t boot = ticks();
  int listening = 1;
  for (;;) {
    /* Listen first so the keyword's bus is recognised before we ever
     * drive a side. */
    if (listening && elapsed(boot, MS(FRESH_MS))) listening = 0;
    for (int i = 0; i < 4; i++) {
      if (!is_link(i)) soft_i2c_poll(&buses[i], &target);
      answer_link(i, listening);
    }
    debounce_key();
    relay(listening);
    doorbell();
    watch_backwards(listening);
    heartbeat();
    if (save_addr >= 0) {
      store_config((uint8_t)save_addr);
      save_addr = -1;
    }
  }
}
