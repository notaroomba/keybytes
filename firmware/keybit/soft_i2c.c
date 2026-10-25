#include "ch32fun.h"
#include "soft_i2c.h"

/* Give up on a transaction if a line sits still this long (a few ms at
 * 48 MHz), e.g. when the neighbour is pulled away mid-transfer. */
#define TIMEOUT_LOOPS 100000

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
  for (int i = 0; i < TIMEOUT_LOOPS; i++)
    if (SCL(b) == level) return 1;
  return 0;
}

/* Clock in 8 bits. While SCL is high SDA must hold still; if it moves, the
 * controller sent a START (falling) or STOP (rising) instead of data. */
static int recv_byte(soft_i2c_bus_t *b, uint8_t *out)
{
  uint8_t v = 0;
  for (int bit = 0; bit < 8; bit++) {
    if (!wait_scl(b, 0) || !wait_scl(b, 1)) return EV_TIMEOUT;
    int d = SDA(b);
    int i = 0;
    while (SCL(b)) {
      int now = SDA(b);
      if (now != d) return now ? EV_STOP : EV_START;
      if (++i > TIMEOUT_LOOPS) return EV_TIMEOUT;
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
 * NACK or timeout, -1 if we lost arbitration. We hold SCL low (clock
 * stretching) while setting up each bit, so slow polling can't miss the
 * setup time. */
static int send_byte(soft_i2c_bus_t *b, uint8_t v)
{
  for (int bit = 7; bit >= 0; bit--) {
    int one = (v >> bit) & 1;
    if (!wait_scl(b, 0)) { SDA_REL(b); return 0; }
    SCL_LOW(b);
    if (one) SDA_REL(b); else SDA_LOW(b);
    SCL_REL(b);
    if (!wait_scl(b, 1)) { SDA_REL(b); return 0; }
    if (one && !SDA(b)) return -1;  /* someone else is sending a 0 */
  }
  if (!wait_scl(b, 0)) { SDA_REL(b); return 0; }
  SDA_REL(b);
  if (!wait_scl(b, 1)) return 0;
  int ack = !SDA(b);
  wait_scl(b, 0);
  return ack;
}

/* Sit out the rest of a transaction that isn't for us. */
static void wait_bus_free(soft_i2c_bus_t *b)
{
  uint8_t dummy;
  for (int n = 0; n < 64; n++) {
    int ev = recv_byte(b, &dummy);
    if (ev == EV_STOP || ev == EV_TIMEOUT) return;
    if (ev == EV_BYTE) { if (!wait_scl(b, 1) || !wait_scl(b, 0)) return; }  /* 9th clock */
  }
}

static uint8_t reg_ptr;

static void serve(soft_i2c_bus_t *b, const soft_i2c_target_t *t)
{
  for (;;) {  /* loops on repeated START */
    uint8_t a;
    int ev = recv_byte(b, &a);
    if (ev == EV_START) continue;
    if (ev != EV_BYTE) return;
    uint8_t addr = a >> 1;
    if (!t->match(addr, b)) { wait_bus_free(b); return; }
    if (!send_ack(b)) return;

    if (a & 1) {  /* controller reads */
      int r;
      while ((r = send_byte(b, t->read(reg_ptr++, b))) == 1) { }
      if (r < 0) {
        if (t->lost) t->lost(addr);
        wait_bus_free(b);
        return;
      }
      /* NACK ends the read; next is STOP or a repeated START */
      ev = recv_byte(b, &a);
      if (ev == EV_START) continue;
      return;
    }

    int first = 1;
    for (;;) {
      uint8_t d;
      ev = recv_byte(b, &d);
      if (ev != EV_BYTE) break;
      if (first) { reg_ptr = d; first = 0; }
      else t->write(reg_ptr++, d, b);
      if (!send_ack(b)) return;
    }
    if (ev == EV_START) continue;
    return;
  }
}

int soft_i2c_poll(soft_i2c_bus_t *b, const soft_i2c_target_t *t)
{
  int sda = SDA(b), scl = SCL(b);
  int start = b->prev_scl && scl && b->prev_sda && !sda;
  b->prev_sda = sda;
  b->prev_scl = scl;
  if (!start) return 0;

  serve(b, t);
  b->prev_sda = SDA(b);
  b->prev_scl = SCL(b);
  return 1;
}

/* ------------------------------------------------------------------------ */
/* Controller                                                               */

/* Half a clock period. Keybit links run at about 100 kHz so the far side's
 * polling loop can keep up. */
#define HALF_US  5
/* Hold START this long so a target busy elsewhere still sees it. */
#define START_US 20

static void release_bus(soft_i2c_bus_t *b)
{
  SDA_REL(b);
  SCL_REL(b);
}

/* Let SCL go and wait for it, honouring clock stretching. */
static int scl_high(soft_i2c_bus_t *b)
{
  SCL_REL(b);
  return wait_scl(b, 1);
}

int soft_i2c_start(soft_i2c_bus_t *b)
{
  if (!SDA(b) || !SCL(b)) return 0;
  Delay_Us(HALF_US);
  if (!SDA(b) || !SCL(b)) return 0;  /* someone else started */
  SDA_LOW(b);
  Delay_Us(START_US);
  SCL_LOW(b);
  Delay_Us(HALF_US);
  return 1;
}

int soft_i2c_restart(soft_i2c_bus_t *b)
{
  SDA_REL(b);
  Delay_Us(HALF_US);
  if (!scl_high(b)) { release_bus(b); return 0; }
  Delay_Us(HALF_US);
  SDA_LOW(b);
  Delay_Us(HALF_US);
  SCL_LOW(b);
  Delay_Us(HALF_US);
  return 1;
}

void soft_i2c_stop(soft_i2c_bus_t *b)
{
  SDA_LOW(b);
  Delay_Us(HALF_US);
  scl_high(b);
  Delay_Us(HALF_US);
  SDA_REL(b);
  Delay_Us(HALF_US);
}

/* One bit out with SCL low on entry and exit. */
static int put_bit(soft_i2c_bus_t *b, int one)
{
  if (one) SDA_REL(b); else SDA_LOW(b);
  Delay_Us(HALF_US);
  if (!scl_high(b)) { release_bus(b); return 0; }
  if (one && !SDA(b)) { release_bus(b); return -1; }  /* lost arbitration */
  Delay_Us(HALF_US);
  SCL_LOW(b);
  return 1;
}

static int get_bit(soft_i2c_bus_t *b, int *bit)
{
  SDA_REL(b);
  Delay_Us(HALF_US);
  if (!scl_high(b)) { release_bus(b); return 0; }
  *bit = SDA(b);
  Delay_Us(HALF_US);
  SCL_LOW(b);
  return 1;
}

int soft_i2c_write(soft_i2c_bus_t *b, uint8_t v)
{
  for (int i = 7; i >= 0; i--) {
    int r = put_bit(b, (v >> i) & 1);
    if (r != 1) return r;
  }
  int nack;
  if (!get_bit(b, &nack)) return 0;
  return !nack;
}

int soft_i2c_read(soft_i2c_bus_t *b, uint8_t *v, int ack)
{
  uint8_t x = 0;
  for (int i = 0; i < 8; i++) {
    int bit;
    if (!get_bit(b, &bit)) return 0;
    x = (uint8_t)((x << 1) | bit);
  }
  *v = x;
  return put_bit(b, !ack) == 1;
}
