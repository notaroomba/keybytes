#include "ch32fun.h"
#include "soft_i2c.h"

/* Give up on a transaction if a line sits still this long, e.g. when the
 * neighbour is pulled away mid-transfer. Timed on SysTick, not loop
 * counts, so it stays ~2 ms however slow the reads are. */
#define TIMEOUT (2 * DELAY_MS_TIME)

enum { EV_BYTE, EV_START, EV_STOP, EV_TIMEOUT };

#define SDA(b)      funDigitalRead((b)->sda)
#define SCL(b)      funDigitalRead((b)->scl)
#define SDA_LOW(b)  funDigitalWrite((b)->sda, FUN_LOW)
#define SDA_REL(b)  funDigitalWrite((b)->sda, FUN_HIGH)
#define SCL_LOW(b)  funDigitalWrite((b)->scl, FUN_LOW)
#define SCL_REL(b)  funDigitalWrite((b)->scl, FUN_HIGH)

void soft_i2c_init(soft_i2c_bus_t *bus)
{
  funDigitalWrite(bus->sda, FUN_HIGH);
  funDigitalWrite(bus->scl, FUN_HIGH);
  funPinMode(bus->sda, GPIO_CFGLR_OUT_10Mhz_OD);
  funPinMode(bus->scl, GPIO_CFGLR_OUT_10Mhz_OD);
  bus->prev_sda = 1;
  bus->prev_scl = 1;
}

static int wait_scl(soft_i2c_bus_t *b, int level)
{
  uint32_t t0 = SysTick->CNT;
  while (SCL(b) != level)
    if (SysTick->CNT - t0 >= TIMEOUT) return 0;
  return 1;
}

/* Clock in 8 bits. While SCL is high SDA must hold still; if it moves, the
 * controller sent a START (falling) or STOP (rising) instead of data. */
static int recv_byte(soft_i2c_bus_t *b, uint8_t *out)
{
  uint8_t v = 0;
  for (int bit = 0; bit < 8; bit++) {
    if (!wait_scl(b, 0) || !wait_scl(b, 1)) return EV_TIMEOUT;
    int d = SDA(b);
    uint32_t t0 = SysTick->CNT;
    while (SCL(b)) {
      int now = SDA(b);
      if (now != d) return now ? EV_STOP : EV_START;
      if (SysTick->CNT - t0 >= TIMEOUT) return EV_TIMEOUT;
    }
    v = (uint8_t)((v << 1) | d);
  }
  *out = v;
  return EV_BYTE;
}

/* Hold SDA low through the 9th clock. Called with SCL low. */
static int send_ack(soft_i2c_bus_t *b)
{
  SDA_LOW(b);
  int ok = wait_scl(b, 1) && wait_scl(b, 0);
  SDA_REL(b);
  return ok;
}

/* Clock out 8 bits, then read the controller's ACK. Returns 1 on ACK, 0 on
 * NACK, -1 if we lost arbitration, -2 on a timeout. We hold SCL low (clock
 * stretching) while setting up each bit, so slow polling can't miss the
 * setup time. */
static int send_byte(soft_i2c_bus_t *b, uint8_t v)
{
  for (int bit = 7; bit >= 0; bit--) {
    int one = (v >> bit) & 1;
    if (!wait_scl(b, 0)) { SDA_REL(b); return -2; }
    SCL_LOW(b);
    if (one) SDA_REL(b); else SDA_LOW(b);
    SCL_REL(b);
    if (!wait_scl(b, 1)) { SDA_REL(b); return -2; }
    if (one && !SDA(b)) return -1;  /* someone else is sending a 0 */
  }
  if (!wait_scl(b, 0)) { SDA_REL(b); return -2; }
  SDA_REL(b);
  if (!wait_scl(b, 1)) return -2;
  int ack = !SDA(b);
  wait_scl(b, 0);
  return ack;
}

/* Sit out the rest of a transaction that isn't for us. It's called right
 * after the address byte, before its 9th clock, so counting 9-bit bytes
 * from here was one bit off: at the end it could be left waiting through
 * TIMEOUT for an SCL fall after the STOP, deaf to the next transfer.
 * Instead just watch for SDA moving while SCL stays high. Returns 1 on a
 * repeated START (the caller reads its address), 0 on a STOP or when the
 * lines sit still for TIMEOUT. */
static int wait_bus_free(soft_i2c_bus_t *b)
{
  int scl = SCL(b), sda = SDA(b);
  uint32_t t0 = SysTick->CNT;
  for (;;) {
    int c = SCL(b), d = SDA(b);
    if (c && scl && d != sda) return !d;   /* SDA fell: START, rose: STOP */
    if (c != scl || d != sda) t0 = SysTick->CNT;
    else if (SysTick->CNT - t0 >= TIMEOUT) return 0;
    scl = c;
    sda = d;
  }
}

static uint8_t reg_ptr;

/* Returns 1 if we were addressed at some point. */
static int serve(soft_i2c_bus_t *b, const soft_i2c_target_t *t)
{
  int addressed = 0;
  for (;;) {  /* loops on repeated START */
    uint8_t a;
    int ev = recv_byte(b, &a);
    if (ev == EV_START) continue;
    if (ev != EV_BYTE) return addressed;
    uint8_t addr = a >> 1;
    if (!t->match(addr, b)) {
      if (wait_bus_free(b)) continue;
      return addressed;
    }
    addressed = 1;
    if (!send_ack(b)) return addressed;

    if (a & 1) {  /* controller reads */
      int r;
      uint8_t first = reg_ptr, n = 0;
      while ((r = send_byte(b, t->read(reg_ptr++, n, b))) == 1) n++;
      n++;
      if (r == 0 && t->read_done) t->read_done(first, n);
      if (r == -1) {
        if (t->lost) t->lost(addr);
        wait_bus_free(b);
        return addressed;
      }
      if (r < 0) return addressed;
      /* NACK ends the read; next is STOP or a repeated START */
      ev = recv_byte(b, &a);
      if (ev == EV_START) continue;
      return addressed;
    }

    int first = 1;
    for (;;) {
      uint8_t d;
      ev = recv_byte(b, &d);
      if (ev != EV_BYTE) break;
      if (first) { reg_ptr = d; first = 0; }
      else t->write(reg_ptr++, d, b);
      if (!send_ack(b)) return addressed;
    }
    if (ev == EV_START) continue;
    return addressed;
  }
}

int soft_i2c_poll(soft_i2c_bus_t *b, const soft_i2c_target_t *t)
{
  int sda = SDA(b), scl = SCL(b);
  int start = b->prev_scl && scl && b->prev_sda && !sda;
  b->prev_sda = sda;
  b->prev_scl = scl;
  if (!start) return 0;

  int addressed = serve(b, t);
  /* However it ended, let go of both lines so we can never hold the bus. */
  SDA_REL(b);
  SCL_REL(b);
  b->prev_sda = SDA(b);
  b->prev_scl = SCL(b);
  return addressed;
}
