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

enum { PROG_NONE, PROG_READ, PROG_WRITE };

#define PHY __attribute__((optimize("O2")))

/* ------------------------------------------------------------------------ */
/* Timing                                                                   */

static inline uint32_t cyc(void) { return DWT->CYCCNT; }

static inline void wait_from(uint32_t start, uint32_t cycles)
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
 *   read: a short low, release, sample ~330 ns later, wait for high
 * Cycles are at 64 MHz. DOCK? t=<n> scales them all by n/16 and od=<0|1>
 * picks open-drain (needs a 1k pull-up, like the BluePill) or push-pull,
 * the default, which works with or without one. */
#define T1_LOW   19
#define T0_LOW   54
#define T_HIGH   54
#define TR_LOW   4
#define TR_SAMPLE 21
uint32_t ch32_t_short = 16;
uint8_t ch32_open_drain;   /* 1 only with an external ~1k pull-up: the internal one is far too weak */

static inline uint32_t scaled(uint32_t c) { return c * ch32_t_short / 16; }

static inline PHY void line_high(ch32_swio_t *s)
{
  if (ch32_open_drain) s->port->MODER = s->moder_in;
  else s->port->BSRR = s->pin;
}

static inline PHY void send_bit(ch32_swio_t *s, int one)
{
  GPIO_TypeDef *p = s->port;
  const uint32_t lo = scaled(one ? T1_LOW : T0_LOW), hi = scaled(T_HIGH);
  uint32_t t = cyc();
  p->BRR = s->pin;
  p->MODER = s->moder_out;
  wait_from(t, lo);
  t = cyc();
  line_high(s);
  wait_from(t, hi);
}

/* Low = 0, high = 1, -1 = target never let go. */
static inline PHY int read_bit(ch32_swio_t *s, uint32_t timeout_cycles)
{
  GPIO_TypeDef *p = s->port;
  const uint32_t lo = scaled(TR_LOW), sample = scaled(TR_SAMPLE);
  uint32_t t = cyc();
  p->BRR = s->pin;
  p->MODER = s->moder_out;
  wait_from(t, lo);
  t = cyc();
  if (ch32_open_drain) p->MODER = s->moder_in;
  else {   /* push-pull: kick it high, then let the target pull against nothing */
    p->BSRR = s->pin;
    p->MODER = s->moder_in;
  }
  wait_from(t, sample);
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
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  refresh_moder(s);
  s->port->BSRR = s->pin;
  s->port->MODER = s->moder_out;

  send_bit(s, 1);
  for (uint32_t m = 1u << 6; m; m >>= 1) send_bit(s, reg & m);
  send_bit(s, 1);
  for (uint32_t m = 1u << 31; m; m >>= 1) send_bit(s, value & m);

  __set_PRIMASK(primask);
  delay_us(CH32_SWIO_FRAME_GAP_US);
}

PHY ch32_status_t ch32_read_reg(ch32_swio_t *s, uint8_t reg, uint32_t *value)
{
  uint32_t timeout = CH32_SWIO_READ_TIMEOUT_US * (SystemCoreClock / 1000000u);
  uint32_t v = 0;
  ch32_status_t ret = CH32_OK;

  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  refresh_moder(s);
  s->port->BSRR = s->pin;
  s->port->MODER = s->moder_out;

  send_bit(s, 1);
  for (uint32_t m = 1u << 6; m; m >>= 1) send_bit(s, reg & m);
  send_bit(s, 0);
  for (int i = 0; i < 32; i++) {
    int b = read_bit(s, timeout);
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
  uint32_t st = 0, cfg = 0;
  if (ch32_read_reg(s, CH32_DMSTATUS, &st) != CH32_OK) return CH32_ERR_NO_TARGET;
  if (st == 0 || st == 0xFFFFFFFFu) return CH32_ERR_NO_TARGET;
  if (ch32_read_reg(s, CH32_DMCFGR, &cfg) != CH32_OK) return CH32_ERR_NO_TARGET;
  if ((cfg & 0xFFFF0000u) != 0x5AA50000u) return CH32_ERR_NO_TARGET;
  return CH32_OK;
}

ch32_status_t ch32_halt(ch32_swio_t *s, int reset)
{
  ch32_write_reg(s, CH32_DMSHDWCFGR, DMCFG_OUTPUT_EN);
  ch32_write_reg(s, CH32_DMCFGR, DMCFG_OUTPUT_EN);
  ch32_write_reg(s, CH32_DMCONTROL, 0x80000001);    /* haltreq + dmactive */
  if (reset) ch32_write_reg(s, CH32_DMCONTROL, 0x80000003);  /* + ndmreset */
  ch32_write_reg(s, CH32_DMCONTROL, 0x80000001);    /* release reset, stay halted */
  ch32_write_reg(s, CH32_DMCONTROL, 0x80000001);
  forget_state(s);
  HAL_Delay(10);  /* let a flash op that was in progress finish */

  uint32_t st = 0;
  if (ch32_read_reg(s, CH32_DMSTATUS, &st) != CH32_OK) return CH32_ERR_NO_TARGET;
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
  do {
    if (ch32_read_reg(s, CH32_DMABSTRACTCS, &cs) != CH32_OK) return CH32_ERR_TIMEOUT;
    if (timeout-- == 0) return CH32_ERR_BUSY;
  } while (cs & (1u << 12));  /* busy */

  if ((cs >> 8) & 7) {  /* cmderr */
    ch32_write_reg(s, CH32_DMABSTRACTCS, 0x00000700);
    return CH32_ERR_ABSTRACT;
  }
  return CH32_OK;
}

/* Confirm it's a V003 and read its chip ID. The core must be halted. */
static ch32_status_t ensure_chip(ch32_swio_t *s)
{
  if (s->chip_known) return CH32_OK;

  uint32_t hart = 0;
  if (ch32_read_reg(s, CH32_DMHARTINFO, &hart) != CH32_OK) return CH32_ERR_NO_TARGET;
  if ((0xE0000000u | (hart & 0x7FF)) != V003_DATA0_ADDR) return CH32_ERR_NOT_V003;

  ch32_write_reg(s, CH32_DMCONTROL, 0x80000001);
  ch32_write_reg(s, CH32_DMCONTROL, 0x80000001);
  ch32_write_reg(s, CH32_DMABSTRACTCS, 0x08000700);
  ch32_write_reg(s, CH32_DMABSTRACTAUTO, 0);

  /* c.lw x8, 0(x8); c.ebreak, with x8 = chip ID address */
  ch32_write_reg(s, CH32_DMPROGBUF0, 0x90024000);
  ch32_write_reg(s, CH32_DMDATA0, V003_CHIP_ID);
  ch32_write_reg(s, CH32_DMCOMMAND, 0x00271008);  /* DATA0 -> x8, run progbuf */
  ch32_status_t r = wait_done(s);
  if (r != CH32_OK) return r;
  ch32_write_reg(s, CH32_DMCOMMAND, 0x00221008);  /* x8 -> DATA0 */
  if (ch32_read_reg(s, CH32_DMDATA0, &s->chip_id) != CH32_OK) return CH32_ERR_NO_TARGET;

  s->chip_known = 1;
  s->prog_state = PROG_NONE;
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
static void load_prog_regs(ch32_swio_t *s)
{
  ch32_write_reg(s, CH32_DMABSTRACTAUTO, 0);
  ch32_write_reg(s, CH32_DMDATA0, V003_DATA0_ADDR);
  ch32_write_reg(s, CH32_DMCOMMAND, 0x0023100A);
  ch32_write_reg(s, CH32_DMDATA0, V003_DATA0_ADDR + 4);
  ch32_write_reg(s, CH32_DMCOMMAND, 0x0023100B);
  ch32_write_reg(s, CH32_DMDATA0, CH_FLASH_STATR);
  ch32_write_reg(s, CH32_DMCOMMAND, 0x0023100C);
  ch32_write_reg(s, CH32_DMDATA0, CTLR_PAGE_PG | CTLR_BUF_LOAD);
  ch32_write_reg(s, CH32_DMCOMMAND, 0x0023100D);
}

/* ------------------------------------------------------------------------ */
/* Memory access. With autoexec on, every access to DATA0 reruns the
 * program buffer, so sequential words cost one frame each.                 */

ch32_status_t ch32_read_word(ch32_swio_t *s, uint32_t addr, uint32_t *data)
{
  ch32_status_t r = ensure_chip(s);
  if (r != CH32_OK) return r;

  /* Polling flash registers must not move the address along. */
  uint8_t autoinc = !(addr == CH_FLASH_CTLR || addr == CH_FLASH_STATR);

  if (s->prog_state != PROG_READ || addr != s->next_addr || autoinc != s->autoincrement) {
    if (s->prog_state != PROG_READ || autoinc != s->autoincrement) {
      if (s->prog_state != PROG_WRITE) load_prog_regs(s);
      ch32_write_reg(s, CH32_DMABSTRACTAUTO, 0);
      /* c.lw x8,0(x11)  address from DATA1;  c.lw x9,0(x8)  load it */
      ch32_write_reg(s, CH32_DMPROGBUF0, 0x40044180);
      /* c.addi x8,4 (or c.nop);  c.sw x9,0(x10)  value to DATA0 */
      ch32_write_reg(s, CH32_DMPROGBUF1, autoinc ? 0xC1040411 : 0xC1040001);
      /* c.sw x8,0(x11)  next address to DATA1;  c.ebreak */
      ch32_write_reg(s, CH32_DMPROGBUF2, 0x9002C180);
      ch32_write_reg(s, CH32_DMABSTRACTAUTO, 1);
      s->autoincrement = autoinc;
    }
    ch32_write_reg(s, CH32_DMDATA1, addr);
    ch32_write_reg(s, CH32_DMCOMMAND, 0x00240000);  /* run progbuf */
    s->prog_state = PROG_READ;
    s->next_addr = addr;
  }
  if (s->autoincrement) s->next_addr += 4;

  r = wait_done(s);
  if (r != CH32_OK) return r;
  return ch32_read_reg(s, CH32_DMDATA0, data) == CH32_OK ? CH32_OK : CH32_ERR_TIMEOUT;
}

ch32_status_t ch32_write_word(ch32_swio_t *s, uint32_t addr, uint32_t data)
{
  ch32_status_t r = ensure_chip(s);
  if (r != CH32_OK) return r;

  uint8_t is_flash = (addr & 0xFF000000u) == 0x08000000u ||
                     (addr & 0x1FFF0000u) == 0x1FFF0000u;

  if (s->prog_state != PROG_WRITE || is_flash != s->write_is_flash) {
    int started = 0;
    if (s->prog_state != PROG_WRITE) {
      ch32_write_reg(s, CH32_DMABSTRACTAUTO, 0);
      started = 1;
      if (s->prog_state != PROG_READ) load_prog_regs(s);
      /* c.lw x8,0(x10)  value;  c.lw x9,0(x11)  address */
      ch32_write_reg(s, CH32_DMPROGBUF0, 0x41844100);
      /* c.sw x8,0(x9)  store;  c.addi x9,4 */
      ch32_write_reg(s, CH32_DMPROGBUF1, 0x0491C080);
      /* c.sw x9,0(x11)  next address to DATA1;  c.nop */
      ch32_write_reg(s, CH32_DMPROGBUF2, 0x0001C184);
    }
    if (is_flash) {
      /* c.sw x13,4(x12)  CTLR = PAGE_PG|BUF_LOAD;  c.lw x8,0(x12)  STATR */
      ch32_write_reg(s, CH32_DMPROGBUF3, 0x4200C254);
      /* c.andi x8,1  BSY;  c.bnez x8,-4  wait */
      ch32_write_reg(s, CH32_DMPROGBUF4, 0xFC758805);
      ch32_write_reg(s, CH32_DMPROGBUF5, 0x90029002);  /* c.ebreak */
    } else {
      ch32_write_reg(s, CH32_DMPROGBUF3, 0x90029002);
    }

    ch32_write_reg(s, CH32_DMDATA1, addr);
    ch32_write_reg(s, CH32_DMDATA0, data);
    if (started) {
      ch32_write_reg(s, CH32_DMCOMMAND, 0x00240000);
      ch32_write_reg(s, CH32_DMABSTRACTAUTO, 1);
    }
    s->write_is_flash = is_flash;
    s->prog_state = PROG_WRITE;
    s->next_addr = addr;
  } else {
    if (addr != s->next_addr) ch32_write_reg(s, CH32_DMDATA1, addr);
    ch32_write_reg(s, CH32_DMDATA0, data);  /* autoexec runs the store */
  }
  s->next_addr += 4;

  return is_flash ? wait_done(s) : CH32_OK;
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
    uint32_t got = 0, want = 0xFFFFFFFFu;
    memcpy(&want, data + i, (len - i) < 4 ? (len - i) : 4);
    ch32_status_t r = ch32_read_word(s, addr + i, &got);
    if (r != CH32_OK) return r;
    if (got != want) return CH32_ERR_VERIFY;
  }
  return CH32_OK;
}

ch32_status_t ch32_program(ch32_swio_t *s, const uint8_t *image, uint32_t len)
{
  ch32_status_t r;
  if ((r = ch32_detect(s)) != CH32_OK) return r;
  if ((r = ch32_halt(s, 1)) != CH32_OK) return r;
  if ((r = ch32_write_flash(s, CH32_FLASH_BASE, image, len)) != CH32_OK) return r;
  if ((r = ch32_verify(s, CH32_FLASH_BASE, image, len)) != CH32_OK) return r;
  ch32_reboot(s);
  return CH32_OK;
}
