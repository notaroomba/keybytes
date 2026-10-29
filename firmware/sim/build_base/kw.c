/*
 * The Keyword in the simulator: the real keybit_bus.c over an I2C3 model
 * with the real timing (0xF0426363 at 64 MHz: 25 us low, 25 us high, ~20
 * kHz, clock stretching honoured), the bus poll loop from main.c and the
 * key handling from keymap.c, reporting every key change it would send.
 */
#include <stdio.h>
#include <stdlib.h>
#include "stm32wbxx_hal.h"
#include "Src/keybit_bus.c"

GPIO_TypeDef sim_gpiob;
IWDG_TypeDef sim_iwdg;

/* Timing knobs, settable from sim.c */
int kw_keys_ms = 15, kw_relay_ms = 60, kw_scan_ms = 500, kw_task_ms = 5;
int64_t kw_loop_ps = 50 * PS_US;      /* rest of the main loop: BLE, USB */
int64_t kw_t_low = 25 * PS_US, kw_t_high = 25 * PS_US;
#define T_SDADEL (500 * PS_NS)
#define T_POLL   (250 * PS_NS)
#define T_HAL    (20 * PS_US)          /* HAL call overhead */

/* ------------------------------------------------------------------------ */
/* I2C3 controller                                                          */

static int64_t deadline;
static int timed_out;

static int scl_up(void)
{
  sim_kw_drive(0, 1);
  while (!sim_kw_read(0)) {
    if (sim_now() > deadline) { timed_out = 1; return 0; }
    sim_sleep(T_POLL);
  }
  return 1;
}

/* SCL is low on entry and on return. */
static int put_bit(int b)
{
  sim_sleep(T_SDADEL);
  sim_kw_drive(1, b);
  sim_sleep(kw_t_low - T_SDADEL);
  if (!scl_up()) return 0;
  sim_sleep(kw_t_high);
  sim_kw_drive(0, 0);
  return 1;
}

static int get_bit(int *b)
{
  sim_sleep(T_SDADEL);
  sim_kw_drive(1, 1);
  sim_sleep(kw_t_low - T_SDADEL);
  if (!scl_up()) return 0;
  sim_sleep(kw_t_high);
  *b = sim_kw_read(1);
  sim_kw_drive(0, 0);
  return 1;
}

/* 1 ack, 0 nack, -1 timeout */
static int put_byte(uint8_t v)
{
  for (int i = 7; i >= 0; i--)
    if (!put_bit((v >> i) & 1)) return -1;
  int b;
  if (!get_bit(&b)) return -1;
  return !b;
}

static int get_byte(uint8_t *v, int ack)
{
  uint8_t x = 0;
  for (int i = 0; i < 8; i++) {
    int b;
    if (!get_bit(&b)) return 0;
    x = (uint8_t)(x << 1 | b);
  }
  *v = x;
  return put_bit(!ack);
}

/* A START, or a repeated START (SCL low on entry: a full low period first). */
static int start(void)
{
  int repeated = !sim_kw_read(0);
  sim_sleep(T_SDADEL);
  sim_kw_drive(1, 1);
  if (repeated) sim_sleep(kw_t_low - T_SDADEL);
  if (!scl_up()) return 0;
  sim_sleep(kw_t_high);
  sim_kw_drive(1, 0);
  sim_sleep(kw_t_high);
  sim_kw_drive(0, 0);
  return 1;
}

static void stop(void)
{
  sim_sleep(T_SDADEL);
  sim_kw_drive(1, 0);
  sim_sleep(kw_t_low);
  scl_up();
  sim_sleep(kw_t_high);
  sim_kw_drive(1, 1);
  sim_sleep(kw_t_low);   /* bus free time */
}

static HAL_StatusTypeDef fail(void)
{
  if (getenv("KW_TRACE2")) printf("%9.3f ms fail%s\n", sim_now() / 1e9, timed_out ? " (timeout)" : "");
  if (timed_out) {
    sim_kw_drive(0, 1);
    sim_kw_drive(1, 1);
    return HAL_TIMEOUT;
  }
  stop();
  return HAL_ERROR;
}

static void begin(uint32_t timeout)
{
  sim_sleep(T_HAL);
  deadline = sim_now() + (int64_t)timeout * PS_MS;
  timed_out = 0;
}

HAL_StatusTypeDef HAL_I2C_IsDeviceReady(I2C_HandleTypeDef *h, uint16_t addr, uint32_t trials, uint32_t timeout)
{
  (void)h; (void)trials;
  begin(timeout);
  if (!start()) return fail();
  int a = put_byte((uint8_t)addr);
  if (a < 0) return fail();
  stop();
  return a ? HAL_OK : HAL_ERROR;
}

HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *h, uint16_t addr, uint16_t reg, uint16_t regsize,
                                   uint8_t *buf, uint16_t len, uint32_t timeout)
{
  (void)h; (void)regsize;
  begin(timeout);
  if (!start() || put_byte((uint8_t)addr) != 1 || put_byte((uint8_t)reg) != 1) return fail();
  if (!start() || put_byte((uint8_t)(addr | 1)) != 1) return fail();
  for (int i = 0; i < len; i++)
    if (!get_byte(&buf[i], i < len - 1)) return fail();
  stop();
  if (getenv("KW_TRACE")) {
    printf("%9.3f ms read %02x reg %02x:", sim_now() / 1e9, addr >> 1, reg);
    for (int i = 0; i < len; i++) printf(" %02x", buf[i]);
    printf("\n");
  }
  return HAL_OK;
}

HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *h, uint16_t addr, uint16_t reg, uint16_t regsize,
                                    uint8_t *buf, uint16_t len, uint32_t timeout)
{
  (void)h; (void)regsize;
  begin(timeout);
  if (!start() || put_byte((uint8_t)addr) != 1 || put_byte((uint8_t)reg) != 1) return fail();
  for (int i = 0; i < len; i++)
    if (put_byte(buf[i]) != 1) return fail();
  stop();
  return HAL_OK;
}

uint32_t HAL_GetTick(void) { return (uint32_t)(sim_now() / PS_MS); }
void HAL_Delay(uint32_t ms) { sim_sleep((int64_t)ms * PS_MS); }
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin)
{
  (void)port;
  return sim_kw_read(pin == GPIO_PIN_13 ? 0 : 1) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}
void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState s)
{
  (void)port;
  if (pin & GPIO_PIN_13) sim_kw_drive(0, s == GPIO_PIN_SET);
  if (pin & GPIO_PIN_14) sim_kw_drive(1, s == GPIO_PIN_SET);
}
void HAL_GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *init) { (void)port; (void)init; }
HAL_StatusTypeDef HAL_I2C_Init(I2C_HandleTypeDef *h) { (void)h; sim_kw_drive(0, 1); sim_kw_drive(1, 1); return HAL_OK; }
HAL_StatusTypeDef HAL_I2C_DeInit(I2C_HandleTypeDef *h) { (void)h; return HAL_OK; }
HAL_I2C_StateTypeDef HAL_I2C_GetState(I2C_HandleTypeDef *h) { (void)h; return HAL_I2C_STATE_READY; }

/* ------------------------------------------------------------------------ */
/* main.c: Keybit_Bus_Poll, without the host messages                       */

#define KEYBIT_MISSES_MAX 2

static I2C_HandleTypeDef hi2c3;
keybit_info_t keybits[KEYBIT_MAX];
int keybit_count;
keybit_remote_t keybit_remotes[KEYBIT_REMOTE_MAX];
int keybit_remote_count;

static void Keybit_Bus_Poll(void)
{
  static uint32_t last_scan, last_keys, last_relay;

  keybit_bus_unstick(&hi2c3);
  if (keybit_bus_jammed()) return;
  if (HAL_GetTick() - last_keys >= (uint32_t)kw_keys_ms) {
    last_keys = HAL_GetTick();
    keybit_bus_poll_keys(&hi2c3, keybits, keybit_count);
  }
  if (HAL_GetTick() - last_relay >= (uint32_t)kw_relay_ms) {
    last_relay = HAL_GetTick();
    keybit_bus_poll_relay(&hi2c3, keybits, keybit_count, keybit_remotes, &keybit_remote_count);
  }
  if (HAL_GetTick() - last_scan < (uint32_t)kw_scan_ms) return;
  last_scan = HAL_GetTick();

  keybit_bus_assign(&hi2c3);
  static uint8_t missed[KEYBIT_MAX];
  keybit_info_t found[KEYBIT_MAX];
  uint8_t found_missed[KEYBIT_MAX] = {0};
  int n = keybit_bus_scan(&hi2c3, found, KEYBIT_MAX);
  for (int i = 0; i < keybit_count && n < KEYBIT_MAX; i++) {
    int here = 0;
    for (int j = 0; j < n && !here; j++)
      if (memcmp(found[j].uid, keybits[i].uid, sizeof(found[j].uid)) == 0) {
        here = 1;
        found[j].saw_down = keybits[i].saw_down;
        found[j].pend = keybits[i].pend;
        found[j].tap = keybits[i].tap;
      }
    if (!here && missed[i] < KEYBIT_MISSES_MAX) {
      found[n] = keybits[i];
      found_missed[n++] = (uint8_t)(missed[i] + 1);
    }
  }
  memcpy(keybits, found, n * sizeof(found[0]));
  memcpy(missed, found_missed, sizeof(missed));
  keybit_count = n;
}

/* ------------------------------------------------------------------------ */
/* keymap.c: what the keys task would report, per Keybit                    */

#define SHOWN_MAX 64
static struct { uint32_t uid; uint8_t down; } shown[SHOWN_MAX];
static int shown_n;

static void show(uint32_t uid, int down)
{
  for (int i = 0; i < shown_n; i++)
    if (shown[i].uid == uid) {
      if (shown[i].down != down) { shown[i].down = (uint8_t)down; kw_report(uid, down); }
      return;
    }
  if (shown_n < SHOWN_MAX) {
    shown[shown_n].uid = uid;
    shown[shown_n++].down = (uint8_t)down;
    if (down) kw_report(uid, down);
  }
}

static void Keys_Task(void)
{
  for (int i = 0; i < keybit_count; i++) {
    int down = keybits[i].key_down;
    if (keybits[i].tap == 1) { down = 1; keybits[i].tap = 2; }
    else if (keybits[i].tap == 2) keybits[i].tap = 0;
    show(uid32(keybits[i].uid), down);
  }
  for (int r = 0; r < keybit_remote_count; r++) {
    keybit_remote_t *k = &keybit_remotes[r];
    int down = k->present && (k->key_down || k->tap == 1);
    if (k->tap == 1) { k->tap = 2; k->dirty = 1; }
    else if (k->tap == 2) k->tap = 0;
    if (down) k->fresh_down = 0;
    k->dirty = 0;
    k->moved = 0;
    show(k->uid, down);
  }
}

/* Probe mode: instead of the poll loop, IsDeviceReady the first Keybit at
 * the given half period, like I2CSCAN after I2CSPEED. */
int kw_probe_left, kw_probe_acks;
int64_t kw_probe_half;

void kw_main(void)
{
  sim_kw_drive(0, 1);
  sim_kw_drive(1, 1);
  sim_sleep(5 * PS_MS);
  uint32_t last_task = 0;
  for (;;) {
    if (kw_probe_left > 0 && keybit_count > 0) {
      /* I2CSCAN: every address in a row, as proto.c does it */
      int64_t lo = kw_t_low, hi = kw_t_high;
      kw_t_low = kw_t_high = kw_probe_half;
      for (uint8_t a = 0x08; a <= 0x77; a++) {
        int ok = HAL_I2C_IsDeviceReady(&hi2c3, (uint16_t)(a << 1), 1, 10) == HAL_OK;
        if (a == keybits[0].addr) {
          kw_probe_acks += ok;
          if (getenv("KW_TRACE")) printf("probe %02x at %.3f ms: %s\n", a, sim_now() / 1e9, ok ? "ack" : "NACK");
        }
      }
      kw_probe_left--;
      kw_t_low = lo; kw_t_high = hi;
      /* the normal poll loop runs between I2CSCAN commands */
      int64_t until = sim_now() + (100 + rand() % 200) * PS_MS;
      while (sim_now() < until) { Keybit_Bus_Poll(); sim_sleep(kw_loop_ps); }
      continue;
    }
    Keybit_Bus_Poll();
    if (HAL_GetTick() - last_task >= (uint32_t)kw_task_ms) {
      last_task = HAL_GetTick();
      Keys_Task();
    }
    sim_sleep(kw_loop_ps);
  }
}

int kw_present(uint32_t uid)
{
  for (int i = 0; i < keybit_count; i++) if (uid32(keybits[i].uid) == uid) return 1;
  for (int r = 0; r < keybit_remote_count; r++) if (keybit_remotes[r].uid == uid && keybit_remotes[r].present) return 2;
  return 0;
}

/* The face a v11 Keybit reported, or 0. */
int kw_where(uint32_t uid, uint32_t *parent, char *pside, char *face)
{
  for (int r = 0; r < keybit_remote_count; r++)
    if (keybit_remotes[r].uid == uid && keybit_remotes[r].present && keybit_remotes[r].face) {
      *parent = keybit_remotes[r].parent;
      *pside = keybit_remotes[r].pside;
      *face = keybit_remotes[r].face;
      return 1;
    }
  return 0;
}
