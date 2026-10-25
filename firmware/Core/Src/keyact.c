#include <string.h>
#include "keyact.h"

enum { ST_IDLE, ST_ACTIVE, ST_PENDING, ST_HOLDING };

#define PULSES 4

/* Taps fired on release: a key or usage that stays in the reports for
 * KEYACT_TAP_MS and then goes away on its own. */
typedef struct
{
  uint8_t used;
  uint8_t mods;
  uint8_t key;
  uint16_t usage;
  uint32_t until;
} pulse_t;

static pulse_t pulses[PULSES];
static uint32_t press_counter;

void KEYACT_Reset(void)
{
  memset(pulses, 0, sizeof(pulses));
  press_counter = 0;
}

static int Has_Hold(const keyact_t *a)
{
  return a && (a->hold_mods || a->hold_key);
}

static void Pulse(const keyact_t *a, uint32_t now)
{
  if (!a || a->kind == KEYACT_NONE) return;
  /* reuse the oldest slot if all are busy */
  pulse_t *p = &pulses[0];
  for (int i = 0; i < PULSES; i++)
  {
    if (!pulses[i].used) { p = &pulses[i]; break; }
    if ((int32_t)(pulses[i].until - p->until) < 0) p = &pulses[i];
  }
  p->used = 1;
  p->mods = a->kind == KEYACT_KEY ? a->mods : 0;
  p->key = a->kind == KEYACT_KEY ? (uint8_t)a->code : 0;
  p->usage = a->kind == KEYACT_CONSUMER ? a->code : 0;
  p->until = now + KEYACT_TAP_MS;
}

/* 6KRO: modifiers OR together, the first six keys fill the slots and a
 * seventh turns every slot into ErrorRollOver as HID asks. */
static void Add(uint8_t report[8], int *slots, uint8_t mods, uint8_t key)
{
  report[0] |= mods;
  if (!key) return;
  for (int s = 0; s < 6 && s < *slots; s++)
  {
    if (report[2 + s] == key) return;
  }
  if (*slots < 6) report[2 + *slots] = key;
  else memset(&report[2], 0x01, 6);
  (*slots)++;
}

void KEYACT_Step(keyact_key_t *keys, const keyact_t *const *acts, int n, uint32_t now,
                 uint8_t report[8], uint16_t *consumer)
{
  int slots = 0;
  uint32_t consumer_seq = 0;
  memset(report, 0, 8);
  *consumer = 0;

  for (int i = 0; i < n; i++)
  {
    keyact_key_t *k = &keys[i];
    const keyact_t *a = acts[i];

    if (k->down && !k->was)
    {
      k->t0 = now;
      k->seq = ++press_counter;
      k->state = Has_Hold(a) ? ST_PENDING : ST_ACTIVE;
    }
    if (k->down && k->state == ST_PENDING
        && (uint32_t)(now - k->t0) >= (a->hold_ms ? a->hold_ms : KEYACT_HOLD_MS_DEF))
    {
      k->state = ST_HOLDING;
    }
    if (!k->down && k->was)
    {
      if (k->state == ST_PENDING) Pulse(a, now);   /* short press: tap */
      k->state = ST_IDLE;
    }
    k->was = k->down;

    if (!k->down || !a) continue;
    if (k->state == ST_HOLDING)
    {
      Add(report, &slots, a->hold_mods, a->hold_key);
    }
    else if (k->state == ST_ACTIVE)
    {
      if (a->kind == KEYACT_KEY) Add(report, &slots, a->mods, (uint8_t)a->code);
      /* most recently pressed consumer key wins */
      if (a->kind == KEYACT_CONSUMER && k->seq > consumer_seq)
      {
        consumer_seq = k->seq;
        *consumer = a->code;
      }
    }
  }

  for (int i = 0; i < PULSES; i++)
  {
    pulse_t *p = &pulses[i];
    if (!p->used) continue;
    if ((int32_t)(now - p->until) >= 0) { p->used = 0; continue; }
    Add(report, &slots, p->mods, p->key);
    if (p->usage) *consumer = p->usage;
  }
}
