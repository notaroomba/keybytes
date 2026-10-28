/*
 * CH32V003 single-wire debug (SWIO) programmer for the STM32WB.
 *
 * Bit-bangs WCH's one-wire debug protocol on one GPIO so the keyword can
 * detect a Keybit sitting on its programming studs and flash it.
 *
 * The protocol and flash sequences are ported from cnlohr's ch32v003
 * programmer (github.com/cnlohr/esp32s2-cookbook, ch32v003_swio.h, MIT) and
 * minichlink (github.com/cnlohr/ch32v003fun, MIT).
 */
#ifndef CH32V003_SWIO_H
#define CH32V003_SWIO_H

#include <stdint.h>
#include "stm32wbxx_hal.h"

/* Bit timing lives in ch32v003_swio.c. These are live (DOCK? a= b= h= s= od=):
 * the "1" low, "0" low and high times in wire cycles at 64 MHz, and
 * ch32_open_drain releases the line for highs instead of driving them
 * (needs a 1k pull-up). */
extern uint32_t ch32_t1, ch32_t0, ch32_hi;
extern uint32_t ch32_t_sample;  /* read: cycles from release to sampling (DOCK? s=) */
extern uint8_t ch32_open_drain;
extern uint32_t ch32_last_acs;
extern uint32_t ch32_retries;   /* steps ch32_program had to retry */
extern int32_t ch32_fail_blk;   /* block ch32_program gave up on, -1 if none */
extern uint32_t ch32_resume_from;  /* block ch32_program started at (resumed from an earlier try) */  /* last DMABSTRACTCS read while waiting on a command */
/* Idle time after each frame. 2 us is sometimes too short. */
#ifndef CH32_SWIO_FRAME_GAP_US
#define CH32_SWIO_FRAME_GAP_US 8
#endif
/* How long to wait for the target to let the line go high again. */
#ifndef CH32_SWIO_READ_TIMEOUT_US
#define CH32_SWIO_READ_TIMEOUT_US 10
#endif

#define CH32_FLASH_BASE  0x08000000u
#define CH32_FLASH_SIZE  (16u * 1024u)
#define CH32_FLASH_BLOCK 64u

/* RISC-V debug module registers (DMI addresses). */
#define CH32_DMDATA0        0x04
#define CH32_DMDATA1        0x05
#define CH32_DMCONTROL      0x10
#define CH32_DMSTATUS       0x11
#define CH32_DMHARTINFO     0x12
#define CH32_DMABSTRACTCS   0x16
#define CH32_DMCOMMAND      0x17
#define CH32_DMABSTRACTAUTO 0x18
#define CH32_DMPROGBUF0     0x20
#define CH32_DMPROGBUF1     0x21
#define CH32_DMPROGBUF2     0x22
#define CH32_DMPROGBUF3     0x23
#define CH32_DMPROGBUF4     0x24
#define CH32_DMPROGBUF5     0x25
#define CH32_DMCPBR         0x7C
#define CH32_DMCFGR         0x7D
#define CH32_DMSHDWCFGR     0x7E

typedef enum {
  CH32_OK            = 0,
  CH32_ERR_NO_TARGET = -1,  /* nothing answered on the line */
  CH32_ERR_TIMEOUT   = -2,  /* target held the line low too long */
  CH32_ERR_NOT_V003  = -3,  /* answered, but not a CH32V003 */
  CH32_ERR_BUSY      = -4,  /* debug module command never finished */
  CH32_ERR_ABSTRACT  = -5,  /* debug module reported a command error */
  CH32_ERR_LOCKED    = -6,  /* flash would not unlock */
  CH32_ERR_FLASH     = -7,  /* flash stayed busy or write-protected */
  CH32_ERR_VERIFY    = -8,  /* read-back did not match */
  CH32_ERR_ARG       = -9,  /* bad address or length */
} ch32_status_t;

typedef struct {
  GPIO_TypeDef *port;
  uint16_t pin;          /* HAL pin mask, e.g. GPIO_PIN_5 */

  /* Internal: cached MODER values and debug-module program state. */
  uint32_t moder_in;
  uint32_t moder_out;
  uint8_t prog_state;
  uint8_t write_is_flash;
  uint8_t autoincrement;
  uint8_t flash_unlocked;
  uint8_t chip_known;
  uint32_t next_addr;
  uint32_t chip_id;
} ch32_swio_t;

/* Set the pin up (push-pull, very high speed, pull-up, idling high) and
 * start the DWT cycle counter. Call once after MX_GPIO_Init(). */
void ch32_init(ch32_swio_t *s, GPIO_TypeDef *port, uint16_t pin);

/* Raw debug-module register access, one SWIO frame each. */
void ch32_write_reg(ch32_swio_t *s, uint8_t reg, uint32_t value);
ch32_status_t ch32_read_reg(ch32_swio_t *s, uint8_t reg, uint32_t *value);

/* CH32_OK when a CH32 answers on the line. Does not halt it. */
ch32_status_t ch32_detect(ch32_swio_t *s);

/* Halt the core, optionally resetting it first so user code can't run. */
ch32_status_t ch32_halt(ch32_swio_t *s, int reset);
/* Reset and let the new firmware run. */
void ch32_reboot(ch32_swio_t *s);

/* Read the chip ID word (0x1FFFF704 on the V003). Needs a halted core. */
ch32_status_t ch32_read_chip_id(ch32_swio_t *s, uint32_t *id);

/* Memory access through the debug module. Needs a halted core. */
ch32_status_t ch32_read_word(ch32_swio_t *s, uint32_t addr, uint32_t *data);
ch32_status_t ch32_write_word(ch32_swio_t *s, uint32_t addr, uint32_t data);

ch32_status_t ch32_unlock_flash(ch32_swio_t *s);
ch32_status_t ch32_erase_page(ch32_swio_t *s, uint32_t addr);
ch32_status_t ch32_erase_all(ch32_swio_t *s);

/* Erase and program len bytes at addr (64-byte aligned). Short final blocks
 * are padded with 0xFF. */
ch32_status_t ch32_write_flash(ch32_swio_t *s, uint32_t addr,
                               const uint8_t *data, uint32_t len);
/* CH32_OK when flash at addr matches data. */
ch32_status_t ch32_verify(ch32_swio_t *s, uint32_t addr,
                          const uint8_t *data, uint32_t len);

/* Whole job: detect, halt with reset, flash image at 0x08000000, verify,
 * reboot. */
ch32_status_t ch32_program(ch32_swio_t *s, const uint8_t *image, uint32_t len);

#endif /* CH32V003_SWIO_H */
