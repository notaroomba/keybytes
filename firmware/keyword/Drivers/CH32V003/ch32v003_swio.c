/*
 * CH32V003 SWIO programmer, see ch32v003_swio.h.
 *
 * Three layers:
 *   1. PHY: one bit at a time on the wire (send_bit / read_bit).
 *   2. Frames: one debug-module register read or write (ch32_*_reg).
 *   3. Memory and flash: small RISC-V programs loaded into the debug
 *      module's program buffer that do loads and stores for us.
 */
#include "ch32v003_swio.h"
#include <string.h>

/* CH32V003 flash controller */
#define CH_FLASH_KEYR     0x40022004u
#define CH_FLASH_OBKEYR   0x40022008u
#define CH_FLASH_STATR    0x4002200Cu
#define CH_FLASH_CTLR     0x40022010u
#define CH_FLASH_ADDR     0x40022014u
#define CH_FLASH_MODEKEYR 0x40022024u
#define CH_FLASH_KEY1     0x45670123u
#define CH_FLASH_KEY2     0xCDEF89ABu

#define STATR_BSY      0x00000001u
#define STATR_WRPRTERR 0x00000010u
#define CTLR_MER       0x00000004u
#define CTLR_STRT      0x00000040u
#define CTLR_LOCKS     0x00008080u  /* LOCK | FLOCK */
#define CTLR_PAGE_PG   0x00010000u
#define CTLR_PAGE_ER   0x00020000u
#define CTLR_BUF_LOAD  0x00040000u
#define CTLR_BUF_RST   0x00080000u

/* On the V003 DATA0/DATA1 are memory mapped here (from DMHARTINFO). */
#define V003_DATA0_ADDR 0xE00000F4u
#define V003_CHIP_ID    0x1FFFF704u

/* Unlock key for DMSHDWCFGR/DMCFGR, plus bit 10 = let the target drive the
 * line (needed for any read). */
#define DMCFG_OUTPUT_EN (0x5AA50000u | (1u << 10))

/* Which program sits in the progbuf; anything but NONE also means x10-x13
 * are loaded (see load_prog_regs). */
enum { PROG_NONE, PROG_READ, PROG_WRITE, PROG_WRITE_FLASH };

#define PHY __attribute__((optimize("O2")))
/* The project builds with -Og, which doesn't inline across a different
 * optimize attribute: cyc() and wait_from() stayed real calls and added
 * 10-20 cycles to every pulse, stretching a 300 ns "1" to about 530 ns
 * and pushing the read sample past the end of the target's "0". Force
 * them inline so the cycle counts below are what goes on the wire. */
#define FAST static inline __attribute__((always_inline))

/* ------------------------------------------------------------------------ */
/* Timing                                                                   */

FAST uint32_t cyc(void) { return DWT->CYCCNT; }

FAST void wait_from(uint32_t start, uint32_t cycles)
{
  while ((cyc() - start) < cycles) { }
}

static void delay_us(uint32_t us)
{
  uint32_t start = cyc();
  wait_from(start, us * (SystemCoreClock / 1000000u));
}

/* The PHY writes all of MODER at once (one store, for timing), so retake
 * the rest of the port's modes before each frame instead of trusting the
 * ones from init: other GPIOB pins (the I2C bus) can change later. */
static void refresh_moder(ch32_swio_t *s)
{
  uint32_t n = __builtin_ctz(s->pin);
  s->moder_in = s->port->MODER & ~(3u << (2 * n));
  s->moder_out = s->moder_in | (1u << (2 * n));
}

/* ------------------------------------------------------------------------ */
/* PHY                                                                      */

/* Timings copied from aslavd/BluePillCH32V003Ardulink (VSL_WCH_SDI.c), an
 * Ardulink-compatible STM32F103 programmer that works with minichlink.
 * After its reset the target's SDI runs in "normal mode 2x" (slow), so every
 * bit needs a long high after it:
 *   1: ~300 ns low, ~850 ns high      0: ~850 ns low, ~850 ns high
 *   read: a short low, release, sample ~300 ns later, wait for high
 * The T_ values are those wire times in cycles at 64 MHz. The code between
 * the edges and the wait loops costs a fixed number of cycles on top (the
 * LAG_ values, counted from the -Og build's disassembly), so phy_timing()
 * takes that off; without it a "1" came out at ~530 ns, close to a "0".
 * On the dock the three write times needed tuning apart, and the "1" low is
 * touchy: about 16-19 cycles works, 20 is the edge, 22 and up fail. Reads
 * are fine across the board. The defaults below came out of DOCKSCAN;
 * DOCK? a=/b=/h= (ch32_t1, ch32_t0, ch32_hi), s= (sample point) and od=
 * still change them live. Push-pull by default: drive the bits, then for a
 * read just let go onto the internal pull-up (no kick, that fights the
 * target) and sample late, ~500 ns, so no external pull-up is needed.
 * Even tuned, about 1 write frame in 10 gets dropped, so the memory layer
 * reads back every register write, checks DATA1 == addr + 4 after each
 * command to catch a dropped one, and ch32_program retries per block and
 * resumes by chip UID. */
#define TR_LOW    10   /* ~150 ns; the target's SDI clock is about 8 MHz, so keep it over 125 ns */
#define LAG_LOW   15
#define LAG_HIGH  12
#define LAG_RLOW  10
uint32_t ch32_t1 = 19;
uint32_t ch32_t0 = 52;
uint32_t ch32_hi = 46;
uint32_t ch32_t_kick = 0;     /* push-pull read: cycles driven high after the low, before letting go; any kick breaks reads */
uint32_t ch32_t_sample = 30;  /* cycles from release to sampling, plus ~3; ~500 ns, late enough for the internal pull-up */
uint32_t ch32_last_acs;     /* last DMABSTRACTCS seen by wait_done, for debugging */
uint8_t ch32_open_drain = 0;   /* 1 needs the external ~1k pull-up; push-pull works with or without it */

/* Pulse lengths in cycles for one frame, worked out before interrupts go
 * off so no multiply or divide sits between edges. */
typedef struct { uint32_t t1, t0, hi, rlo, rs, kick; } phy_t;

static uint32_t wire(uint32_t cycles, uint32_t lag)
{
  return cycles > lag ? cycles - lag : 1;
}

static phy_t phy_timing(void)
{
  return (phy_t){ wire(ch32_t1, LAG_LOW), wire(ch32_t0, LAG_LOW), wire(ch32_hi, LAG_HIGH),
                  wire(TR_LOW, LAG_RLOW), ch32_t_sample, ch32_t_kick };
}

FAST void line_high(ch32_swio_t *s)
{
  if (ch32_open_drain) s->port->MODER = s->moder_in;
  else s->port->BSRR = s->pin;
}

FAST void send_bit(ch32_swio_t *s, const phy_t *ph, int one)
{
  GPIO_TypeDef *p = s->port;
  const uint32_t lo = one ? ph->t1 : ph->t0;
  p->BRR = s->pin;
  uint32_t t = cyc();
  p->MODER = s->moder_out;
  wait_from(t, lo);
  line_high(s);
  t = cyc();
  wait_from(t, ph->hi);
}

/* Low = 0, high = 1, -1 = target never let go. */
FAST int read_bit(ch32_swio_t *s, const phy_t *ph, uint32_t timeout_cycles)
{
  GPIO_TypeDef *p = s->port;
  p->BRR = s->pin;
  uint32_t t = cyc();
  p->MODER = s->moder_out;
  wait_from(t, ph->rlo);
  if (ch32_open_drain) p->MODER = s->moder_in;
  else {   /* push-pull: kick it high briefly, then leave it to the internal pull-up and the target */
    if (ph->kick) {   /* 0: no kick, let go straight away */
      p->BSRR = s->pin;
      t = cyc();
      wait_from(t, ph->kick);
    }
    p->MODER = s->moder_in;
  }
  t = cyc();
  wait_from(t, ph->rs);
  int bit = (p->IDR & s->pin) != 0;

  t = cyc();
  while (!(p->IDR & s->pin)) {
    if ((cyc() - t) > timeout_cycles) {
      p->BSRR = s->pin;
      p->MODER = s->moder_out;
      return -1;
    }
  }
  return bit;
}

/* ------------------------------------------------------------------------ */
/* Frames: start bit, 7-bit address MSB first, R/W bit (1 = write), then 32
 * data bits MSB first. No parity in SWIO mode.                             */

void ch32_init(ch32_swio_t *s, GPIO_TypeDef *port, uint16_t pin)
{
  memset(s, 0, sizeof(*s));
  s->port = port;
  s->pin = pin;

  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

  port->BSRR = pin;
  GPIO_InitTypeDef g = {0};
  g.Pin = pin;
  g.Mode = GPIO_MODE_OUTPUT_PP;
  g.Pull = GPIO_PULLUP;
  g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(port, &g);

  refresh_moder(s);
}

PHY void ch32_write_reg(ch32_swio_t *s, uint8_t reg, uint32_t value)
{
  const phy_t ph = phy_timing();
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  refresh_moder(s);
  s->port->BSRR = s->pin;
  s->port->MODER = s->moder_out;

  send_bit(s, &ph, 1);
  for (uint32_t m = 1u << 6; m; m >>= 1) send_bit(s, &ph, reg & m);
  send_bit(s, &ph, 1);
  for (uint32_t m = 1u << 31; m; m >>= 1) send_bit(s, &ph, value & m);

  __set_PRIMASK(primask);
  delay_us(CH32_SWIO_FRAME_GAP_US);
}

PHY ch32_status_t ch32_read_reg(ch32_swio_t *s, uint8_t reg, uint32_t *value)
{
  const phy_t ph = phy_timing();
  uint32_t timeout = CH32_SWIO_READ_TIMEOUT_US * (SystemCoreClock / 1000000u);
  uint32_t v = 0;
  ch32_status_t ret = CH32_OK;

  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  refresh_moder(s);
  s->port->BSRR = s->pin;
  s->port->MODER = s->moder_out;

  send_bit(s, &ph, 1);
  for (uint32_t m = 1u << 6; m; m >>= 1) send_bit(s, &ph, reg & m);
  send_bit(s, &ph, 0);
  for (int i = 0; i < 32; i++) {
    int b = read_bit(s, &ph, timeout);
    if (b < 0) { ret = CH32_ERR_TIMEOUT; break; }
    v = (v << 1) | (uint32_t)b;
  }

  __set_PRIMASK(primask);
  delay_us(CH32_SWIO_FRAME_GAP_US);
  if (ret == CH32_OK) *value = v;
  return ret;
}

/* ------------------------------------------------------------------------ */
/* Debug module control                                                     */

static void forget_state(ch32_swio_t *s)
{
  s->prog_state = PROG_NONE;
  s->write_is_flash = 0;
  s->autoincrement = 0;
  s->flash_unlocked = 0;
  s->chip_known = 0;
}

ch32_status_t ch32_detect(ch32_swio_t *s)
{
  /* Same handshake as minichlink's DefaultSetupInterface (cnlohr/ch32fun),
   * minus the halt: enable the target's output twice, set dmactive three
   * times (early writes can get lost while the debug module starts), then
   * DMSTATUS must read something other than all 0s or all 1s.
   * First the BluePill programmer's reset: high 5 ms, low 20 ms, which
   * puts the target's SDI back in its slow default mode. */
  refresh_moder(s);
  s->port->BSRR = s->pin;
  s->port->MODER = s->moder_out;
  delay_us(5000);
  s->port->BRR = s->pin;
  delay_us(20000);
  s->port->BSRR = s->pin;
  delay_us(CH32_SWIO_FRAME_GAP_US);

  for (int i = 0; i < 2; i++) {
    ch32_write_reg(s, CH32_DMSHDWCFGR, DMCFG_OUTPUT_EN);
    ch32_write_reg(s, CH32_DMCFGR, DMCFG_OUTPUT_EN);
  }
  for (int i = 0; i < 3; i++) ch32_write_reg(s, CH32_DMCONTROL, 0x00000001);  /* dmactive */

  /* minichlink only rejects an all-0/all-1 DMSTATUS, but a line that floats
   * low (open-drain with no pull-up) reads as random 0s and gets past that,
   * so also require the 0x5AA5 key back from DMCFGR. */
  /* The first reads after the handshake sometimes come back wrong while
   * the reads right after are fine, so give it a few tries. */
  for (int i = 0; i < 3; i++) {
    uint32_t st = 0, cfg = 0;
    if (ch32_read_reg(s, CH32_DMSTATUS, &st) == CH32_OK && st != 0 && st != 0xFFFFFFFFu &&
        ch32_read_reg(s, CH32_DMCFGR, &cfg) == CH32_OK && (cfg & 0xFFFF0000u) == 0x5AA50000u)
      return CH32_OK;
    delay_us(100);
  }
  return CH32_ERR_NO_TARGET;
}

ch32_status_t ch32_halt(ch32_swio_t *s, int reset)
{
  ch32_write_reg(s, CH32_DMCONTROL, 0x80000001);    /* haltreq + dmactive */
  if (reset) ch32_write_reg(s, CH32_DMCONTROL, 0x80000003);  /* + ndmreset */
  ch32_write_reg(s, CH32_DMCONTROL, 0x80000001);    /* release reset, stay halted */
  ch32_write_reg(s, CH32_DMCONTROL, 0x80000001);
  forget_state(s);
  HAL_Delay(10);  /* let a flash op that was in progress finish */

  /* A haltreq or a DMSTATUS read can get lost, so ask again a couple of times. */
  uint32_t st = 0xFFFFFFFFu;
  for (int t = 0; t < 3; t++) {
    if (t) {
      ch32_write_reg(s, CH32_DMCONTROL, 0x80000001);
      ch32_write_reg(s, CH32_DMCONTROL, 0x80000001);
      HAL_Delay(1);
    }
    for (int i = 0; i < 3 && (st == 0xFFFFFFFFu || !(st & (1u << 9))); i++)
      if (ch32_read_reg(s, CH32_DMSTATUS, &st) != CH32_OK) st = 0xFFFFFFFFu;
    if (st != 0xFFFFFFFFu && (st & (1u << 9))) break;
  }
  if (st == 0xFFFFFFFFu) return CH32_ERR_NO_TARGET;
  if (!(st & (1u << 9))) return CH32_ERR_BUSY;  /* allhalted */
  ch32_write_reg(s, CH32_DMABSTRACTCS, 0x08000700);  /* clear old errors */
  return CH32_OK;
}

void ch32_reboot(ch32_swio_t *s)
{
  ch32_write_reg(s, CH32_DMCONTROL, 0x80000001);
  ch32_write_reg(s, CH32_DMCONTROL, 0x80000001);
  ch32_write_reg(s, CH32_DMCONTROL, 0x80000003);  /* reset */
  ch32_write_reg(s, CH32_DMCONTROL, 0x40000001);  /* resumereq */
  forget_state(s);
  HAL_Delay(10);
}

/* Wait for an abstract command (or program buffer run) to finish. */
static ch32_status_t wait_done(ch32_swio_t *s)
{
  uint32_t cs = 0;
  int timeout = 1000;
  for (;;) {
    if (ch32_read_reg(s, CH32_DMABSTRACTCS, &cs) != CH32_OK) { ch32_last_acs = cs; return CH32_ERR_TIMEOUT; }
    if (timeout-- == 0) { ch32_last_acs = cs; return CH32_ERR_BUSY; }
    if (((cs >> 24) & 0x1F) != 8) continue;  /* progbufsize is always 8: a garbled read, ask again */
    if (!(cs & (1u << 12))) break;           /* busy */
  }

  ch32_last_acs = cs;
  if ((cs >> 8) & 7) {  /* cmderr */
    ch32_write_reg(s, CH32_DMABSTRACTCS, 0x00000700);
    return CH32_ERR_ABSTRACT;
  }
  return CH32_OK;
}

/* About 1 write frame in 10 still gets dropped on the dock, so every
 * register we can read back is written until it reads back right. */
static ch32_status_t write_v(ch32_swio_t *s, uint8_t reg, uint32_t value)
{
  for (int i = 0; i < 10; i++) {
    uint32_t got = 0;
    ch32_write_reg(s, reg, value);
    if (ch32_read_reg(s, reg, &got) == CH32_OK && got == value) return CH32_OK;
  }
  return CH32_ERR_TIMEOUT;
}

/* Run an abstract command. COMMAND can't be read back, so on a command
 * error clear it and send it again. A dropped COMMAND just looks like a
 * finished one; callers catch that by reading results back. */
static ch32_status_t exec(ch32_swio_t *s, uint32_t cmd)
{
  ch32_status_t r = CH32_ERR_ABSTRACT;
  for (int i = 0; i < 5 && r == CH32_ERR_ABSTRACT; i++) {
    ch32_write_reg(s, CH32_DMCOMMAND, cmd);
    r = wait_done(s);
    if (r == CH32_ERR_ABSTRACT) ch32_write_reg(s, CH32_DMABSTRACTCS, 0x08000700);
  }
  return r;
}

/* Confirm it's a V003 and read its chip ID. The core must be halted. */
static ch32_status_t ensure_chip(ch32_swio_t *s)
{
  if (s->chip_known) return CH32_OK;

  uint32_t hart = 0;
  for (int i = 0; i < 3 && (0xE0000000u | (hart & 0x7FF)) != V003_DATA0_ADDR; i++)
    if (ch32_read_reg(s, CH32_DMHARTINFO, &hart) != CH32_OK) hart = 0;
  if ((0xE0000000u | (hart & 0x7FF)) != V003_DATA0_ADDR) return CH32_ERR_NOT_V003;

  ch32_write_reg(s, CH32_DMCONTROL, 0x80000001);
  ch32_write_reg(s, CH32_DMCONTROL, 0x80000001);
  ch32_write_reg(s, CH32_DMABSTRACTCS, 0x08000700);
  s->prog_state = PROG_NONE;

  ch32_status_t r;
  if ((r = write_v(s, CH32_DMABSTRACTAUTO, 0)) != CH32_OK) return r;
  /* c.lw x8, 0(x8); c.ebreak, with x8 = chip ID address */
  if ((r = write_v(s, CH32_DMPROGBUF0, 0x90024000)) != CH32_OK) return r;
  if ((r = write_v(s, CH32_DMDATA0, V003_CHIP_ID)) != CH32_OK) return r;
  if ((r = exec(s, 0x00271008)) != CH32_OK) return r;  /* DATA0 -> x8, run progbuf */
  if ((r = exec(s, 0x00221008)) != CH32_OK) return r;  /* x8 -> DATA0 */
  if (ch32_read_reg(s, CH32_DMDATA0, &s->chip_id) != CH32_OK) return CH32_ERR_NO_TARGET;

  s->chip_known = 1;
  return CH32_OK;
}

ch32_status_t ch32_read_chip_id(ch32_swio_t *s, uint32_t *id)
{
  ch32_status_t r = ensure_chip(s);
  if (r == CH32_OK) *id = s->chip_id;
  return r;
}

/* Load the registers the read/write programs use:
 *   x10 = &DATA0, x11 = &DATA1, x12 = &CH_FLASH_STATR, x13 = PAGE_PG|BUF_LOAD */
static ch32_status_t load_prog_regs(ch32_swio_t *s)
{
  const uint32_t v[4] = { V003_DATA0_ADDR, V003_DATA0_ADDR + 4, CH_FLASH_STATR, CTLR_PAGE_PG | CTLR_BUF_LOAD };
  ch32_status_t r;
  for (uint32_t i = 0; i < 4; i++) {
    if ((r = write_v(s, CH32_DMDATA0, v[i])) != CH32_OK) return r;
    if ((r = exec(s, 0x0023100A + i)) != CH32_OK) return r;  /* DATA0 -> x10 + i */
  }
  return CH32_OK;
}

/* Put the read, write or flash-write program in the progbuf, unless it's
 * already there. */
static ch32_status_t load_prog(ch32_swio_t *s, uint8_t want)
{
  static const uint32_t rd[] = {
    0x40044180,  /* c.lw x8,0(x11)  address from DATA1;  c.lw x9,0(x8)  load it */
    0xC1040411,  /* c.addi x8,4;  c.sw x9,0(x10)  value to DATA0 */
    0x9002C180,  /* c.sw x8,0(x11)  next address to DATA1;  c.ebreak */
  };
  static const uint32_t wr[] = {
    0x41844100,  /* c.lw x8,0(x10)  value;  c.lw x9,0(x11)  address */
    0x0491C080,  /* c.sw x8,0(x9)  store;  c.addi x9,4 */
    0x0001C184,  /* c.sw x9,0(x11)  next address to DATA1;  c.nop */
    0x90029002,  /* c.ebreak */
  };
  static const uint32_t wf[] = {
    0x41844100, 0x0491C080, 0x0001C184,
    0x4200C254,  /* c.sw x13,4(x12)  CTLR = PAGE_PG|BUF_LOAD;  c.lw x8,0(x12)  STATR */
    0xFC758805,  /* c.andi x8,1  BSY;  c.bnez x8,-4  wait */
    0x90029002,  /* c.ebreak */
  };
  if (s->prog_state == want) return CH32_OK;

  ch32_status_t r;
  if (s->prog_state == PROG_NONE && (r = load_prog_regs(s)) != CH32_OK) return r;
  const uint32_t *p = want == PROG_READ ? rd : want == PROG_WRITE ? wr : wf;
  uint32_t n = want == PROG_READ ? 3 : want == PROG_WRITE ? 4 : 6;
  for (uint32_t i = 0; i < n; i++)
    if ((r = write_v(s, CH32_DMPROGBUF0 + i, p[i])) != CH32_OK) { s->prog_state = PROG_NONE; return r; }
  s->prog_state = want;
  return CH32_OK;
}

/* ------------------------------------------------------------------------ */
/* Memory access. One word per command, no autoexec: every DATA write is
 * checked before the program runs.                                         */

/* Run the loaded read or write program on addr (and data, for a write).
 * Both programs leave addr + 4 in DATA1, so if that isn't there afterwards
 * the COMMAND frame was dropped and it all goes again. */
static ch32_status_t run_at(ch32_swio_t *s, uint32_t addr, int write, uint32_t data)
{
  ch32_status_t r;
  for (int i = 0; i < 10; i++) {
    if ((r = write_v(s, CH32_DMDATA1, addr)) != CH32_OK) return r;
    if (write && (r = write_v(s, CH32_DMDATA0, data)) != CH32_OK) return r;
    if ((r = exec(s, 0x00240000)) != CH32_OK) return r;  /* run progbuf; for flash, also waits out the BUF_LOAD */
    uint32_t next = 0;
    if (ch32_read_reg(s, CH32_DMDATA1, &next) == CH32_OK && next == addr + 4) return CH32_OK;
  }
  return CH32_ERR_TIMEOUT;
}

ch32_status_t ch32_read_word(ch32_swio_t *s, uint32_t addr, uint32_t *data)
{
  ch32_status_t r;
  if ((r = ensure_chip(s)) != CH32_OK) return r;
  if ((r = load_prog(s, PROG_READ)) != CH32_OK) return r;
  if ((r = run_at(s, addr, 0, 0)) != CH32_OK) return r;
  return ch32_read_reg(s, CH32_DMDATA0, data) == CH32_OK ? CH32_OK : CH32_ERR_TIMEOUT;
}

ch32_status_t ch32_write_word(ch32_swio_t *s, uint32_t addr, uint32_t data)
{
  uint8_t is_flash = (addr & 0xFF000000u) == 0x08000000u ||
                     (addr & 0x1FFF0000u) == 0x1FFF0000u;
  ch32_status_t r;
  if ((r = ensure_chip(s)) != CH32_OK) return r;
  if ((r = load_prog(s, is_flash ? PROG_WRITE_FLASH : PROG_WRITE)) != CH32_OK) return r;
  return run_at(s, addr, 1, data);
}

/* ------------------------------------------------------------------------ */
/* Flash                                                                    */

static ch32_status_t wait_flash(ch32_swio_t *s)
{
  uint32_t st = 0;
  int timeout = 0;
  ch32_status_t r;
  do {
    r = ch32_read_word(s, CH_FLASH_STATR, &st);
    if (r != CH32_OK) return r;
  } while ((st & STATR_BSY) && timeout++ < 2000);

  ch32_write_word(s, CH_FLASH_STATR, 0);
  if (st & (STATR_WRPRTERR | STATR_BSY)) return CH32_ERR_FLASH;
  return CH32_OK;
}

ch32_status_t ch32_unlock_flash(ch32_swio_t *s)
{
  uint32_t ctlr = 0;
  ch32_status_t r = ch32_read_word(s, CH_FLASH_CTLR, &ctlr);
  if (r != CH32_OK) return r;

  if (ctlr & CTLR_LOCKS) {
    ch32_write_word(s, CH_FLASH_KEYR, CH_FLASH_KEY1);
    ch32_write_word(s, CH_FLASH_KEYR, CH_FLASH_KEY2);
    ch32_write_word(s, CH_FLASH_OBKEYR, CH_FLASH_KEY1);
    ch32_write_word(s, CH_FLASH_OBKEYR, CH_FLASH_KEY2);
    ch32_write_word(s, CH_FLASH_MODEKEYR, CH_FLASH_KEY1);  /* unlocks 64-byte fast ops */
    ch32_write_word(s, CH_FLASH_MODEKEYR, CH_FLASH_KEY2);
    r = ch32_read_word(s, CH_FLASH_CTLR, &ctlr);
    if (r != CH32_OK) return r;
    if (ctlr & CTLR_LOCKS) return CH32_ERR_LOCKED;
  }
  s->flash_unlocked = 1;
  return CH32_OK;
}

ch32_status_t ch32_erase_page(ch32_swio_t *s, uint32_t addr)
{
  ch32_status_t r;
  if (!s->flash_unlocked && (r = ch32_unlock_flash(s)) != CH32_OK) return r;
  if ((r = wait_flash(s)) != CH32_OK) return r;
  ch32_write_word(s, CH_FLASH_CTLR, CTLR_PAGE_ER);
  ch32_write_word(s, CH_FLASH_ADDR, addr);
  ch32_write_word(s, CH_FLASH_CTLR, CTLR_PAGE_ER | CTLR_STRT);
  if ((r = wait_flash(s)) != CH32_OK) return r;
  ch32_write_word(s, CH_FLASH_CTLR, 0);
  return CH32_OK;
}

ch32_status_t ch32_erase_all(ch32_swio_t *s)
{
  ch32_status_t r;
  if (!s->flash_unlocked && (r = ch32_unlock_flash(s)) != CH32_OK) return r;
  ch32_write_word(s, CH_FLASH_CTLR, 0);
  ch32_write_word(s, CH_FLASH_CTLR, CTLR_MER);
  ch32_write_word(s, CH_FLASH_CTLR, CTLR_MER | CTLR_STRT);
  if ((r = wait_flash(s)) != CH32_OK) return r;
  ch32_write_word(s, CH_FLASH_CTLR, 0);
  return CH32_OK;
}

/* Erase one 64-byte page, fill the page buffer word by word, then commit. */
static ch32_status_t write_block(ch32_swio_t *s, uint32_t addr, const uint8_t *data, uint32_t len)
{
  ch32_status_t r = ch32_erase_page(s, addr);
  if (r != CH32_OK) return r;

  ch32_write_word(s, CH_FLASH_CTLR, CTLR_PAGE_PG);
  ch32_write_word(s, CH_FLASH_CTLR, CTLR_PAGE_PG | CTLR_BUF_RST);
  if ((r = wait_flash(s)) != CH32_OK) return r;

  for (uint32_t i = 0; i < CH32_FLASH_BLOCK; i += 4) {
    uint32_t w = 0xFFFFFFFFu;
    if (i < len) memcpy(&w, data + i, (len - i) < 4 ? (len - i) : 4);
    if ((r = ch32_write_word(s, addr + i, w)) != CH32_OK) return r;
  }

  ch32_write_word(s, CH_FLASH_ADDR, addr);
  ch32_write_word(s, CH_FLASH_CTLR, CTLR_PAGE_PG | CTLR_STRT);
  return wait_flash(s);
}

ch32_status_t ch32_write_flash(ch32_swio_t *s, uint32_t addr, const uint8_t *data, uint32_t len)
{
  if ((addr & (CH32_FLASH_BLOCK - 1)) || addr < CH32_FLASH_BASE ||
      addr + len > CH32_FLASH_BASE + CH32_FLASH_SIZE)
    return CH32_ERR_ARG;

  ch32_status_t r = ensure_chip(s);
  if (r != CH32_OK) return r;

  for (uint32_t off = 0; off < len; off += CH32_FLASH_BLOCK) {
    uint32_t n = len - off < CH32_FLASH_BLOCK ? len - off : CH32_FLASH_BLOCK;
    if ((r = write_block(s, addr + off, data + off, n)) != CH32_OK) return r;
  }
  ch32_write_word(s, CH_FLASH_CTLR, 0);
  return CH32_OK;
}

ch32_status_t ch32_verify(ch32_swio_t *s, uint32_t addr, const uint8_t *data, uint32_t len)
{
  for (uint32_t i = 0; i < len; i += 4) {
    IWDG->KR = 0xAAAA;  /* a full verify plus a flash can outlast the bootloader's ~8 s watchdog */
    uint32_t got = 0, want = 0xFFFFFFFFu;
    memcpy(&want, data + i, (len - i) < 4 ? (len - i) : 4);
    /* A garbled read looks like a mismatch, so read a word that differs again before believing it. */
    for (int t = 0; t < 3; t++) {
      ch32_status_t r = ch32_read_word(s, addr + i, &got);
      if (r != CH32_OK) return r;
      if (got == want) break;
    }
    if (got != want) {
      ch32_bad_addr = addr + i;
      ch32_bad_want = want;
      ch32_bad_got = got;
      return CH32_ERR_VERIFY;
    }
  }
  return CH32_OK;
}

uint32_t ch32_bad_addr, ch32_bad_want, ch32_bad_got;
uint32_t ch32_retries;
int32_t ch32_fail_blk = -1;
uint32_t ch32_resume_from;

/* Where the last unfinished ch32_program got to, and on which chip (UID),
 * so the next attempt on the same chip carries on instead of starting over. */
static struct { uint32_t uid[3]; uint32_t next_blk; uint8_t valid; } resume;

/* After a failed step: clear the command error, drop what we cached about
 * the target and halt it again, so the step can be retried. */
static void recover(ch32_swio_t *s)
{
  ch32_write_reg(s, CH32_DMABSTRACTCS, 0x08000700);
  forget_state(s);
  ch32_halt(s, 0);
  ch32_retries++;
}

/* Flashes the image block by block. A frame now and then still comes back
 * garbled, and a flash takes thousands of them, so each step gets a few
 * tries: the unlock up to 5, each 64-byte block (written, then read back) up to 8. */
ch32_status_t ch32_program(ch32_swio_t *s, const uint8_t *image, uint32_t len)
{
  ch32_status_t r;
  ch32_retries = 0;
  ch32_fail_blk = -1;
  ch32_resume_from = 0;
  /* The dock poll has usually just halted it; a fresh detect would reset
   * the line again, so only do it if it isn't halted (read twice). */
  uint32_t st1 = 0, st2 = 0;
  int halted = ch32_read_reg(s, CH32_DMSTATUS, &st1) == CH32_OK && ch32_read_reg(s, CH32_DMSTATUS, &st2) == CH32_OK &&
               st1 != 0xFFFFFFFFu && st2 != 0xFFFFFFFFu && (st1 & st2 & (1u << 9));
  if (!halted) {
    if ((r = ch32_detect(s)) != CH32_OK) return r;
    if ((r = ch32_halt(s, 0)) != CH32_OK && (r = ch32_detect(s)) == CH32_OK) r = ch32_halt(s, 0);
    if (r != CH32_OK) return r;
  }
  for (int t = 0; t < 5; t++) {
    if ((r = ensure_chip(s)) == CH32_OK && (r = ch32_unlock_flash(s)) == CH32_OK) break;
    recover(s);
  }
  if (r != CH32_OK) return r;

  /* Only trust the UID when two reads agree, or a garbled one restarts at block 0. */
  uint32_t uid[3], again[3];
  int have_uid = 0;
  for (int t = 0; t < 3 && !have_uid; t++) {
    have_uid = 1;
    for (int i = 0; i < 3; i++)
      if (ch32_read_word(s, 0x1FFFF7E8u + 4u * i, &uid[i]) != CH32_OK ||
          ch32_read_word(s, 0x1FFFF7E8u + 4u * i, &again[i]) != CH32_OK || uid[i] != again[i]) have_uid = 0;
  }
  if (!have_uid || !resume.valid || memcmp(uid, resume.uid, sizeof(uid)) != 0) resume.next_blk = 0;
  resume.valid = have_uid;
  if (have_uid) memcpy(resume.uid, uid, sizeof(uid));
  ch32_resume_from = resume.next_blk;

  for (uint32_t off = resume.next_blk * CH32_FLASH_BLOCK; off < len; off += CH32_FLASH_BLOCK) {
    uint32_t n = len - off < CH32_FLASH_BLOCK ? len - off : CH32_FLASH_BLOCK;
    IWDG->KR = 0xAAAA;  /* the whole image takes longer than the bootloader's ~8 s watchdog */
    for (int t = 0; t < 8; t++) {
      r = ensure_chip(s);
      if (r == CH32_OK && !s->flash_unlocked) r = ch32_unlock_flash(s);
      if (r == CH32_OK) r = write_block(s, CH32_FLASH_BASE + off, image + off, n);
      if (r == CH32_OK) r = ch32_verify(s, CH32_FLASH_BASE + off, image + off, n);
      if (r == CH32_OK) break;
      recover(s);
    }
    if (r != CH32_OK) {
      ch32_fail_blk = (int32_t)(off / CH32_FLASH_BLOCK);
      return r;
    }
    resume.next_blk = off / CH32_FLASH_BLOCK + 1;
  }
  resume.valid = 0;
  ch32_write_word(s, CH_FLASH_CTLR, 0);
  ch32_reboot(s);
  return CH32_OK;
}
