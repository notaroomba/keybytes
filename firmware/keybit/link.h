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
 *   Ack       The child pulls DT low while CK is low; if it was ringing the
 *             doorbell (DT already low), it lets DT go instead. The parent
 *             then releases CK, the child releases DT, and both lines high
 *             again marks the start of the frame.
 *   Bits      The parent clocks every bit. Each half period is
 *             LINK_HALF_US. The parent's bits are sampled on CK rising;
 *             the child sets its bits on CK falling and the parent reads
 *             them just before CK falls again. MSB first.
 *   Frame     parent -> child: 0xA7, parent depth, parent uid32 (4),
 *                              the parent's side this is (N/E/S/W)
 *             child -> parent: child depth, n (<= 8), n events of 11
 *                              bytes (uid32, flags, parent uid32, parent
 *                              side, face), CRC-8 of all of those
 *             parent -> child: kept, ~kept
 *             The child drops the first `kept` events from its queue.
 *
 * A frame with one event is 13 bytes, about 60 us at LINK_HALF_US 1 plus
 * the handshake.
 */
#ifndef LINK_H
#define LINK_H

#include <stdint.h>

#define LINK_MAGIC   0xA7   /* v12; v11 used 0xA6 (doorbell ack was a pulse), v10 0xA5 */
#define LINK_EVENTS  8
#define LINK_EV_SIZE 11  /* uid32, flags, parent uid32, parent side, face */
#define LINK_ME_SIZE 5   /* what a parent tells a child about itself: uid32, side */

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

/* What link_poll did. */
enum { LINK_OK = 1, LINK_SKIPPED = 0, LINK_NO_ACK = -1, LINK_ABORTED = -2, LINK_BAD_FRAME = -3 };

/* Parent: run one frame. doorbell says the child was ringing (DT low
 * before we started). On success fills *child and returns LINK_OK, else
 * one of the other codes; *kept is the number of events we told the child
 * we kept (the caller takes those). */
int link_poll(const link_pins_t *p, int doorbell, uint8_t my_depth, const uint8_t me[LINK_ME_SIZE],
              link_reply_t *child, uint8_t room, uint8_t *kept, link_idle_fn idle);

/* CRC-8 (poly 0x07), as the frames use. */
uint8_t link_crc8(uint8_t crc, uint8_t v);

/* Child: CK is low on this side and the caller decided it's a request.
 * Answers with *reply. On success sets *parent_depth, parent (its uid32
 * and the side of it we're on) and *kept, and returns 1. */
int link_serve(const link_pins_t *p, int ringing, const link_reply_t *reply,
               uint8_t *parent_depth, uint8_t parent[LINK_ME_SIZE], uint8_t *kept);

#endif
