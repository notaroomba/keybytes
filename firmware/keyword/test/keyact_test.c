/*
 * Host check for the tap/hold state machine in Core/Src/keyact.c.
 *   cc -std=c99 -ICore/Inc test/keyact_test.c Core/Src/keyact.c -o /tmp/keyact_test && /tmp/keyact_test
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "keyact.h"

static keyact_key_t keys[3];
static const keyact_t *acts[3];
static uint8_t report[8];
static uint16_t consumer;
static uint32_t now;

/* run 5 ms ticks until t, like keymap.c does */
static void run_to(uint32_t t)
{
  for (; now <= t; now += 5) KEYACT_Step(keys, acts, 3, now, report, &consumer);
  now -= 5;
}

static int has_key(uint8_t key)
{
  for (int i = 2; i < 8; i++) if (report[i] == key) return 1;
  return 0;
}

static void reset(void)
{
  KEYACT_Reset();
  memset(keys, 0, sizeof(keys));
  memset(acts, 0, sizeof(acts));
  now = 1000;
}

int main(void)
{
  const keyact_t a = {.kind = KEYACT_KEY, .code = 0x04};
  const keyact_t b = {.kind = KEYACT_KEY, .mods = 0x02, .code = 0x05};
  const keyact_t vol = {.kind = KEYACT_CONSUMER, .code = 0x00E9};
  const keyact_t mute = {.kind = KEYACT_CONSUMER, .code = 0x00E2};
  /* tap: Escape, hold: left ctrl */
  const keyact_t esc_ctrl = {.kind = KEYACT_KEY, .code = 0x29, .hold_mods = 0x01, .hold_ms = 200};

  /* plain key: in the report exactly while held */
  reset();
  acts[0] = &a;
  run_to(1010);
  assert(!has_key(0x04));
  keys[0].down = 1;
  run_to(1020);
  assert(has_key(0x04) && report[0] == 0);
  run_to(2000);
  assert(has_key(0x04));
  keys[0].down = 0;
  run_to(2005);
  assert(!has_key(0x04));

  /* two keys plus modifiers */
  reset();
  acts[0] = &a;
  acts[1] = &b;
  keys[0].down = keys[1].down = 1;
  run_to(1000);
  assert(has_key(0x04) && has_key(0x05) && report[0] == 0x02);

  /* unmapped keys send nothing */
  reset();
  keys[2].down = 1;
  run_to(1100);
  assert(report[0] == 0 && report[2] == 0 && consumer == 0);

  /* consumer: most recent press wins, falls back to the older one */
  reset();
  acts[0] = &vol;
  acts[1] = &mute;
  keys[0].down = 1;
  run_to(1000);
  assert(consumer == 0x00E9);
  keys[1].down = 1;
  run_to(1010);
  assert(consumer == 0x00E2);
  keys[1].down = 0;
  run_to(1020);
  assert(consumer == 0x00E9);
  keys[0].down = 0;
  run_to(1030);
  assert(consumer == 0);

  /* hold key, short press: nothing while held, a tap of Escape on release */
  reset();
  acts[0] = &esc_ctrl;
  keys[0].down = 1;
  run_to(1150);
  assert(!has_key(0x29) && report[0] == 0);
  keys[0].down = 0;
  run_to(1155);
  assert(has_key(0x29) && report[0] == 0);
  run_to(1155 + KEYACT_TAP_MS);
  assert(!has_key(0x29));

  /* hold key, long press: ctrl from 200 ms until release, no Escape */
  reset();
  acts[0] = &esc_ctrl;
  keys[0].down = 1;
  run_to(1195);
  assert(report[0] == 0);
  run_to(1200);
  assert(report[0] == 0x01 && !has_key(0x29));
  run_to(1600);
  assert(report[0] == 0x01);
  keys[0].down = 0;
  run_to(1605);
  assert(report[0] == 0 && !has_key(0x29));
  run_to(1700);
  assert(!has_key(0x29));

  /* ctrl held + another key = ctrl-a (the usual home row mod use) */
  reset();
  acts[0] = &esc_ctrl;
  acts[1] = &a;
  keys[0].down = 1;
  run_to(1250);
  keys[1].down = 1;
  run_to(1260);
  assert(report[0] == 0x01 && has_key(0x04));

  /* hold on a consumer key: tap sends the usage briefly */
  reset();
  const keyact_t vol_shift = {.kind = KEYACT_CONSUMER, .code = 0x00E9, .hold_mods = 0x02};
  acts[0] = &vol_shift;
  keys[0].down = 1;
  run_to(1050);
  assert(consumer == 0);
  keys[0].down = 0;
  run_to(1055);
  assert(consumer == 0x00E9);
  run_to(1055 + KEYACT_TAP_MS);
  assert(consumer == 0);

  /* seven keys: ErrorRollOver in every slot */
  reset();
  static keyact_key_t many[7];
  static const keyact_t k7[7] = {
    {.kind = KEYACT_KEY, .code = 4}, {.kind = KEYACT_KEY, .code = 5}, {.kind = KEYACT_KEY, .code = 6},
    {.kind = KEYACT_KEY, .code = 7}, {.kind = KEYACT_KEY, .code = 8}, {.kind = KEYACT_KEY, .code = 9},
    {.kind = KEYACT_KEY, .code = 10}};
  const keyact_t *p7[7];
  for (int i = 0; i < 7; i++) { p7[i] = &k7[i]; many[i].down = 1; }
  KEYACT_Step(many, p7, 6, 1000, report, &consumer);
  assert(report[2] == 4 && report[7] == 9);
  KEYACT_Step(many, p7, 7, 1005, report, &consumer);
  for (int i = 2; i < 8; i++) assert(report[i] == 0x01);

  /* macro: each tap pressed for one 5 ms tick, released for the next */
  reset();
  const keyact_t hi = {.kind = KEYACT_MACRO, .macro_len = 3,
                       .macro = {{0x02, 0x0B}, {0x00, 0x0C}, {0x00, 0x0C}}};   /* H i i */
  acts[0] = &hi;
  keys[0].down = 1;
  KEYACT_Step(keys, acts, 3, 1000, report, &consumer);
  assert(report[0] == 0x02 && has_key(0x0B));
  keys[0].down = 0;   /* releasing the key doesn't stop the macro */
  KEYACT_Step(keys, acts, 3, 1005, report, &consumer);
  assert(report[0] == 0 && report[2] == 0);
  KEYACT_Step(keys, acts, 3, 1010, report, &consumer);
  assert(report[0] == 0 && has_key(0x0C));
  KEYACT_Step(keys, acts, 3, 1015, report, &consumer);
  assert(report[2] == 0);   /* release between the two i's */
  KEYACT_Step(keys, acts, 3, 1020, report, &consumer);
  assert(has_key(0x0C));
  KEYACT_Step(keys, acts, 3, 1025, report, &consumer);
  KEYACT_Step(keys, acts, 3, 1030, report, &consumer);
  KEYACT_Step(keys, acts, 3, 1035, report, &consumer);
  assert(report[0] == 0 && report[2] == 0);   /* done */

  /* a second press runs it again; held keys stay in the reports alongside */
  acts[1] = &a;
  keys[1].down = 1;
  keys[0].down = 1;
  KEYACT_Step(keys, acts, 3, 1040, report, &consumer);
  assert(has_key(0x0B) && has_key(0x04));

  /* macro as the tap action of a hold key */
  reset();
  const keyact_t hi_ctrl = {.kind = KEYACT_MACRO, .macro_len = 1, .macro = {{0x00, 0x0B}}, .hold_mods = 0x01};
  acts[0] = &hi_ctrl;
  keys[0].down = 1;
  run_to(1050);
  assert(report[2] == 0);
  keys[0].down = 0;
  KEYACT_Step(keys, acts, 3, 1055, report, &consumer);
  assert(has_key(0x0B) && report[0] == 0);

  puts("keyact_test: ok");
  return 0;
}
