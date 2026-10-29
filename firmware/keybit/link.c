#include "ch32fun.h"
#include "link.h"

/* Half a clock period, in SysTick ticks: 0.5 us, so about 1 Mbit/s. Lines
 * are driven both ways during a frame, so the limit is how far the child's
 * polling loop trails CK (a few hundred ns), not the pull-ups. */
#define HALF         (DELAY_US_TIME / 2)
/* Bus turnaround: one end lets go of DT this long before the other drives
 * it, so the two outputs never fight. */
#define TURN         DELAY_US_TIME
/* How long the parent waits for a child to notice a request. A child can
 * be busy on another side for about a millisecond. */
#define ACK_US       1500
/* Any single step once a frame has started. */
#define STEP_US      200

#define DT(p)     (funDigitalRead((p)->dt) != 0)
#define CK(p)     (funDigitalRead((p)->ck) != 0)
#define DT_LOW(p) funDigitalWrite((p)->dt, FUN_LOW)
#define DT_REL(p) funDigitalWrite((p)->dt, FUN_HIGH)
#define CK_LOW(p) funDigitalWrite((p)->ck, FUN_LOW)
#define CK_REL(p) funDigitalWrite((p)->ck, FUN_HIGH)

/* Idle and handshake stay open-drain: either end may pull a line then,
 * and an unknown side might be the keyword's I2C bus. A line only goes
 * push-pull once the handshake has settled who owns it. */
#define DRIVE(pin)   funPinMode(pin, GPIO_CFGLR_OUT_50Mhz_PP)
#define RELEASE(pin) do { funDigitalWrite(pin, FUN_HIGH); funPinMode(pin, GPIO_CFGLR_OUT_10Mhz_OD); } while (0)

/* 1 when pin reaches level, 0 on timeout, -1 if idle() called it off. */
static int wait_pin(uint8_t pin, int level, uint32_t us, link_idle_fn idle)
{
  uint32_t t0 = SysTick->CNT, span = us * DELAY_US_TIME;
  while ((funDigitalRead(pin) != 0) != level) {
    if (SysTick->CNT - t0 >= span) return 0;
    if (idle && idle()) return -1;
  }
  return 1;
}

uint8_t link_crc8(uint8_t crc, uint8_t v)
{
  crc ^= v;
  for (int i = 0; i < 8; i++) crc = (uint8_t)(crc & 0x80 ? (crc << 1) ^ 0x07 : crc << 1);
  return crc;
}

/* ------------------------------------------------------------------------ */
/* Parent                                                                   */

/* turn: the child drove DT last; give it time to let go before we drive. */
static void p_put(const link_pins_t *p, uint8_t v, int turn)
{
  for (int i = 7; i >= 0; i--) {
    CK_LOW(p);
    if (turn) { DelaySysTick(TURN); DRIVE(p->dt); turn = 0; }
    if ((v >> i) & 1) DT_REL(p); else DT_LOW(p);
    DelaySysTick(HALF);
    CK_REL(p);
    DelaySysTick(HALF);
  }
}

/* turn: we drove DT last and just let go; give the child a little longer
 * on the first bit to take it over. */
static uint8_t p_get(const link_pins_t *p, int turn)
{
  uint8_t x = 0;
  for (int i = 0; i < 8; i++) {
    CK_LOW(p);
    DelaySysTick(turn ? HALF + TURN : HALF);
    turn = 0;
    CK_REL(p);
    DelaySysTick(HALF);
    x = (uint8_t)((x << 1) | DT(p));
  }
  return x;
}

int link_poll(const link_pins_t *p, int doorbell, uint8_t my_depth, const uint8_t me[LINK_ME_SIZE],
              link_reply_t *child, uint8_t room, uint8_t *kept, link_idle_fn idle)
{
  if (!CK(p)) return LINK_SKIPPED;
  /* DT low with CK high and no doorbell expected: a Keybit turned the
   * wrong way round has our DT on its GND and our CK on its 3V3. Pulling
   * CK low would short its rail through our pin, so leave the side alone. */
  if (!doorbell && !DT(p)) return LINK_SKIPPED;
  CK_LOW(p);

  /* A ringing child already holds DT low: it acks by letting go. (It used
   * to let go for 3 us and pull it low again, and a parent busy serving
   * I2C between looks missed that pulse and timed out.) Otherwise the ack
   * is DT low. */
  int ok = wait_pin(p->dt, doorbell ? 1 : 0, ACK_US, idle);
  if (ok <= 0) { CK_REL(p); return ok < 0 ? LINK_ABORTED : LINK_NO_ACK; }
  DelaySysTick(HALF);
  CK_REL(p);
  if (wait_pin(p->ck, 1, STEP_US, 0) <= 0 || wait_pin(p->dt, 1, STEP_US, 0) <= 0) { CK_REL(p); return LINK_BAD_FRAME; }

  /* From here we own CK and drive it both ways. The frame runs on our
   * clock without waiting for the child, so it can't hang if the child is
   * pulled away; a child that falls behind just fails the CRC. */
  DRIVE(p->ck);
  DRIVE(p->dt);
  DelaySysTick(HALF);
  p_put(p, LINK_MAGIC, 0);
  p_put(p, my_depth, 0);
  for (int i = 0; i < LINK_ME_SIZE; i++) p_put(p, me[i], 0);
  RELEASE(p->dt);

  child->depth = p_get(p, 1);
  child->n = p_get(p, 0);
  uint8_t crc = link_crc8(link_crc8(0, child->depth), child->n);
  int good = child->n <= LINK_EVENTS;
  for (int i = 0; good && i < child->n * LINK_EV_SIZE; i++) {
    child->ev[i] = p_get(p, 0);
    crc = link_crc8(crc, child->ev[i]);
  }
  good = good && p_get(p, 0) == crc;

  uint8_t k = !good ? 0 : child->n < room ? child->n : room;
  p_put(p, k, 1);
  p_put(p, (uint8_t)~k, 0);
  RELEASE(p->dt);
  RELEASE(p->ck);
  if (!good) return LINK_BAD_FRAME;
  *kept = k;
  return LINK_OK;
}

/* ------------------------------------------------------------------------ */
/* Child                                                                    */

/* The parent doesn't wait for us mid-frame, so following CK has to be
 * quick: spin on the input register directly, a few cycles per check.
 * SPIN bounds the wait (roughly 200 us at 48 MHz). */
#define SPIN 4000
/* The input register read, swapped out by the host simulator (firmware/sim). */
#ifndef READ_PORT
#define READ_PORT(in) (*(in))
#endif

typedef struct {
  const volatile uint32_t *in;
  uint32_t ck, dt;
} fast_t;

static inline __attribute__((always_inline)) int spin(const fast_t *f, int level)
{
  uint32_t want = level ? f->ck : 0;
  for (int n = SPIN; n; n--)
    if ((READ_PORT(f->in) & f->ck) == want) return 1;
  return 0;
}

static inline __attribute__((always_inline)) int c_get(const fast_t *f, const fast_t *d, uint8_t *v)
{
  uint8_t x = 0;
  for (int i = 0; i < 8; i++) {
    if (!spin(f, 0) || !spin(f, 1)) return 0;
    x = (uint8_t)((x << 1) | ((READ_PORT(d->in) & d->dt) != 0));
  }
  *v = x;
  return 1;
}

static inline __attribute__((always_inline)) int c_put(const fast_t *f, const link_pins_t *p, uint8_t v)
{
  for (int i = 7; i >= 0; i--) {
    if (!spin(f, 0)) return 0;
    if ((v >> i) & 1) DT_REL(p); else DT_LOW(p);
    if (!spin(f, 1)) return 0;
  }
  return 1;
}

int link_serve(const link_pins_t *p, int ringing, const link_reply_t *reply,
               uint8_t *parent_depth, uint8_t parent[LINK_ME_SIZE], uint8_t *kept)
{
  /* CK and DT may sit on different ports. */
  fast_t f = { &GpioOf(p->ck)->INDR, 1u << (p->ck & 0xf), 0 };
  fast_t d = { &GpioOf(p->dt)->INDR, 0, 1u << (p->dt & 0xf) };

  /* Work out everything we send now; there's no time between bytes. */
  uint8_t out[2 + LINK_EVENTS * LINK_EV_SIZE + 1];
  int len = 2 + reply->n * LINK_EV_SIZE;
  out[0] = reply->depth;
  out[1] = reply->n;
  for (int i = 2; i < len; i++) out[i] = reply->ev[i - 2];
  uint8_t crc = 0;
  for (int i = 0; i < len; i++) crc = link_crc8(crc, out[i]);
  out[len++] = crc;

  /* Ack: let go of the doorbell, or pull DT low; then wait for CK high. */
  if (ringing) DT_REL(p);
  else DT_LOW(p);
  int ok = spin(&f, 1);
  DT_REL(p);
  if (!ok) return 0;

  uint8_t magic, depth, them[LINK_ME_SIZE];
  if (!c_get(&f, &d, &magic) || !c_get(&f, &d, &depth) || magic != LINK_MAGIC) return 0;
  for (int i = 0; i < LINK_ME_SIZE; i++)
    if (!c_get(&f, &d, &them[i])) return 0;

  /* The parent let go of DT after its last bit and gives us an extra TURN
   * on this first low phase; wait half of it, then drive. */
  if (!spin(&f, 0)) return 0;
  DelaySysTick(TURN / 2);
  DRIVE(p->dt);
  for (int i = 0; i < len; i++)
    if (!c_put(&f, p, out[i])) goto fail;

  /* Let go of DT as soon as the parent takes CK low for its next bit. */
  if (!spin(&f, 0)) goto fail;
  RELEASE(p->dt);

  uint8_t k, inv;
  if (!c_get(&f, &d, &k) || !c_get(&f, &d, &inv)) return 0;
  *parent_depth = depth;
  for (int i = 0; i < LINK_ME_SIZE; i++) parent[i] = them[i];
  *kept = (uint8_t)~inv == k && k <= reply->n ? k : 0;
  return 1;

fail:
  RELEASE(p->dt);
  return 0;
}
