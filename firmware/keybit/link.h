/*
 * Keybit-to-Keybit link: a two-wire protocol on a side's SDA/SCL pair,
 * used only between two Keybits (never on the keyword's I2C bus).
 *
 * The lines keep their I2C names here as DT (SDA) and CK (SCL). Both stay
 * open-drain on the 10k pull-ups, so a clash or a wrong rotation can't
 * fight a push-pull driver.
 *
 *   Request   The parent pulls CK low with DT high.
 *   Doorbell  A child with events pulls DT low with CK high, so its parent
 *             polls it now instead of at the next periodic poll.
 *   Ack       The child pulls DT low while CK is low (if it was ringing the
 *             doorbell, it lets DT go high first so the edge is visible).
 *             The parent then releases CK, the child releases DT, and both
 *             lines high again marks the start of the frame.
 *   Bits      The parent clocks every bit. Each half period is
 *             LINK_HALF_US. The parent's bits are sampled on CK rising;
 *             the child sets its bits on CK falling and the parent reads
 *             them just before CK falls again. MSB first.
 *   Frame     parent -> child: 0xA5, parent depth
 *             child -> parent: child depth, n (<= 8), n events of 5
 *                              bytes, CRC-8 of all of those
 *             parent -> child: kept, ~kept
 *             The child drops the first `kept` events from its queue.
 *
 * A frame with one event is 13 bytes, about 60 us at LINK_HALF_US 1 plus
 * the handshake.
 */
#ifndef LINK_H
#define LINK_H

#include <stdint.h>

#define LINK_MAGIC   0xA5
#define LINK_EVENTS  8
#define LINK_EV_SIZE 5

typedef struct {
  uint8_t dt, ck;  /* ch32fun pin numbers: the side's SDA and SCL */
} link_pins_t;

/* What the child side answers with, filled in by the caller. */
typedef struct {
  uint8_t depth;
  uint8_t n;
  uint8_t ev[LINK_EVENTS * LINK_EV_SIZE];
} link_reply_t;

/* Return 1 to abort a link request we're waiting on, e.g. because the
 * keyword just started a transaction on another side and needs serving. */
typedef int (*link_idle_fn)(void);

/* Parent: run one frame. doorbell says the child was ringing (DT low
 * before we started). On success fills *child and returns 1; *kept is the
 * number of events we told the child we kept (the caller takes those). */
int link_poll(const link_pins_t *p, int doorbell, uint8_t my_depth,
              link_reply_t *child, uint8_t room, uint8_t *kept, link_idle_fn idle);

/* Child: CK is low on this side and the caller decided it's a request.
 * Answers with *reply. On success sets *parent_depth and *kept and
 * returns 1. */
int link_serve(const link_pins_t *p, int ringing, const link_reply_t *reply,
               uint8_t *parent_depth, uint8_t *kept);

#endif
