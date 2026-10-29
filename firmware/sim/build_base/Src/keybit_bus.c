#include "keybit_bus.h"
#include <string.h>

#define I2C_TIMEOUT_MS 50   /* at 20 kHz a full 8-event relay read alone takes ~19 ms */
#define ADDR_FIRST 0x08
#define ADDR_LAST  0x77
#define CHARGER_ADDR 0x6A  /* BQ25895 shares I2C3 */
#define LINK_ADDR    0x31  /* Keybit-to-Keybit links */

#define REG_MAGIC  0x00
#define REG_KEY    0x06
#define REG_UID    0x10
#define REG_ASSIGN 0x20
#define REG_EV_COUNT 0x52
#define REG_EV_DATA  0x53
#define REG_EV_POP   0x7B
#define REG_EV_DATA11 0x80  /* v11: 11-byte events with where the Keybit is */
#define EV_SIZE11    11
#define EV_BATCH     8
#define EV_PRESSED   0x01
#define EV_HEARTBEAT 0x02
#define EV_BACKWARDS 0x04
#define MAGIC      0x4B

/* I2C3's pins (see stm32wbxx_hal_msp.c); IDR still reads them in AF mode. */
#define BUS_PORT GPIOB
#define BUS_SCL  GPIO_PIN_13
#define BUS_SDA  GPIO_PIN_14
#define JAM_RETRY_MS 1000

int keybit_bus_held(void)
{
  return HAL_GPIO_ReadPin(BUS_PORT, BUS_SDA) == GPIO_PIN_RESET &&
         HAL_GPIO_ReadPin(BUS_PORT, BUS_SCL) == GPIO_PIN_SET;
}

int keybit_bus_unstick(I2C_HandleTypeDef *i2c)
{
  static uint32_t low_since;
  uint32_t now = HAL_GetTick();
  if (HAL_GPIO_ReadPin(BUS_PORT, BUS_SCL) == GPIO_PIN_SET || HAL_I2C_GetState(i2c) != HAL_I2C_STATE_READY) {
    low_since = now;
    return 0;
  }
  if (now - low_since < 5) return 0;

  HAL_I2C_DeInit(i2c);
  GPIO_InitTypeDef g = {0};
  g.Pin = BUS_SCL | BUS_SDA;
  g.Mode = GPIO_MODE_OUTPUT_OD;
  g.Pull = GPIO_NOPULL;
  g.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_WritePin(BUS_PORT, BUS_SCL | BUS_SDA, GPIO_PIN_SET);
  HAL_GPIO_Init(BUS_PORT, &g);
  for (int i = 0; i < 9; i++) {   /* 9 clocks finish any byte and its ACK */
    HAL_GPIO_WritePin(BUS_PORT, BUS_SCL, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(BUS_PORT, BUS_SCL, GPIO_PIN_SET);
    HAL_Delay(1);
  }
  HAL_GPIO_WritePin(BUS_PORT, BUS_SDA, GPIO_PIN_RESET);   /* STOP: SDA rises while SCL is high */
  HAL_Delay(1);
  HAL_GPIO_WritePin(BUS_PORT, BUS_SDA, GPIO_PIN_SET);
  HAL_I2C_Init(i2c);   /* MspInit puts the pins back on I2C3 */
  low_since = HAL_GetTick();
  return 1;
}

int keybit_bus_jammed(void)
{
  static uint32_t last_try;
  if (!keybit_bus_held()) return 0;
  /* Let one attempt through now and then: its few clocks free a Keybit
   * that really did get stuck mid-byte, at a tiny fraction of the usual
   * poll rate. */
  if (HAL_GetTick() - last_try >= JAM_RETRY_MS) {
    last_try = HAL_GetTick();
    return 0;
  }
  return 1;
}

static int is_ready(I2C_HandleTypeDef *i2c, uint8_t addr)
{
  /* Just the address byte (~0.5 ms at 20 kHz): a short timeout, or a
   * misbehaving bus makes a full scan outlast the bootloader's ~8 s watchdog. */
  return HAL_I2C_IsDeviceReady(i2c, (uint16_t)(addr << 1), 1, 3) == HAL_OK;
}

static int read_regs(I2C_HandleTypeDef *i2c, uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
  return HAL_I2C_Mem_Read(i2c, (uint16_t)(addr << 1), reg, I2C_MEMADD_SIZE_8BIT,
                          buf, len, I2C_TIMEOUT_MS) == HAL_OK;
}

static uint8_t free_address(I2C_HandleTypeDef *i2c)
{
  for (uint8_t a = ADDR_FIRST; a <= ADDR_LAST; a++) {
    if (a == KEYBIT_DEFAULT_ADDR || a == LINK_ADDR || a == CHARGER_ADDR) continue;
    if (!is_ready(i2c, a)) return a;
  }
  return 0;
}

int keybit_bus_assign(I2C_HandleTypeDef *i2c)
{
  int assigned = 0;
  /* Each round, all unassigned Keybits answer the UID read together and
   * arbitrate down to the lowest UID; that one gets an address and leaves
   * the default address, so the next round finds the next one. */
  for (int round = 0; round < KEYBIT_MAX; round++) {
    uint8_t msg[13];
    if (!read_regs(i2c, KEYBIT_DEFAULT_ADDR, REG_UID, msg, 12)) break;
    uint8_t addr = free_address(i2c);
    if (!addr) break;
    msg[12] = addr;
    if (HAL_I2C_Mem_Write(i2c, KEYBIT_DEFAULT_ADDR << 1, REG_ASSIGN, I2C_MEMADD_SIZE_8BIT,
                          msg, sizeof(msg), I2C_TIMEOUT_MS) != HAL_OK)
      break;
    HAL_Delay(15);  /* it saves the address to flash, about 6 ms */
    assigned++;
  }
  return assigned;
}

/* The Keybit's side touching us gives its place on the keyword. It can only
 * mate upright, so its east side means it sits on our left, and so on. */
static char keyword_side(char keybit_side)
{
  switch (keybit_side) {
  case 'E': return 'L';
  case 'W': return 'R';
  case 'N': return 'R';   /* turned 90 degrees: only the right side meets an N side the right way round */
  case 'S': return 'T';
  default:  return '?';
  }
}

int keybit_bus_scan(I2C_HandleTypeDef *i2c, keybit_info_t *out, int max)
{
  int n = 0;
  for (uint8_t a = ADDR_FIRST; a <= ADDR_LAST && n < max; a++) {
    IWDG->KR = 0xAAAA;
    if (a == KEYBIT_DEFAULT_ADDR || a == LINK_ADDR || a == CHARGER_ADDR) continue;
    if (!is_ready(i2c, a)) continue;

    uint8_t head[10];
    if (!read_regs(i2c, a, REG_MAGIC, head, sizeof(head)) || head[0] != MAGIC) continue;

    keybit_info_t *k = &out[n];
    memset(k, 0, sizeof(*k));
    /* Reading the UID also catches two Keybits on one address: the loser of
     * the arbitration drops its address and turns up on the default one. */
    if (!read_regs(i2c, a, REG_UID, k->uid, sizeof(k->uid))) continue;
    k->addr = a;
    k->fw = head[1];
    k->key_down = head[6] & 1;
    k->keybit_side = (char)head[7];
    k->at = keyword_side(k->keybit_side);
    k->backwards = k->fw >= 6 ? head[9] & 0x0F : 0;  /* reg 0x09 is new in v6 */
    n++;
  }
  return n;
}

uint32_t keybit_bus_poll_keys(I2C_HandleTypeDef *i2c, keybit_info_t *k, int n)
{
  uint32_t changed = 0;
  for (int i = 0; i < n; i++) {
    uint8_t v;
    if (!read_regs(i2c, k[i].addr, REG_KEY, &v, 1)) continue;  /* next scan drops it */
    if (v & 1) {
      k[i].saw_down = 1;
      k[i].pend = 0;
    }
    if ((v & 1) != k[i].key_down) {
      k[i].key_down = v & 1;
      changed |= 1u << i;
    }
  }
  return changed;
}

static uint32_t uid32(const uint8_t *b)
{
  return (uint32_t)b[0] | (uint32_t)b[1] << 8 | (uint32_t)b[2] << 16 | (uint32_t)b[3] << 24;
}

static keybit_remote_t *remote_slot(keybit_remote_t *r, int *rn, uint32_t uid)
{
  keybit_remote_t *free_slot = NULL;
  for (int i = 0; i < *rn; i++) {
    if (r[i].uid == uid) return &r[i];
    if (!r[i].present && !r[i].dirty && !free_slot) free_slot = &r[i];
  }
  if (*rn < KEYBIT_REMOTE_MAX) return &r[(*rn)++];
  return free_slot;  /* NULL when the table is full */
}

int keybit_bus_poll_relay(I2C_HandleTypeDef *i2c, keybit_info_t *k, int n,
                          keybit_remote_t *r, int *rn)
{
  int changed = 0;
  uint32_t now = HAL_GetTick();

  for (int i = 0; i < n; i++) {
    uint8_t count;
    if (!read_regs(i2c, k[i].addr, REG_EV_COUNT, &count, 1) || count == 0) continue;
    if (count > EV_BATCH) count = EV_BATCH;
    /* v11 Keybits also say where each Keybit is; older ones only uid + flags. */
    int size = k[i].fw >= 11 ? EV_SIZE11 : 5;
    uint8_t ev[EV_BATCH * EV_SIZE11];
    if (!read_regs(i2c, k[i].addr, size == 5 ? REG_EV_DATA : REG_EV_DATA11, ev, (uint16_t)(count * size))) continue;
    HAL_I2C_Mem_Write(i2c, (uint16_t)(k[i].addr << 1), REG_EV_POP, I2C_MEMADD_SIZE_8BIT,
                      &count, 1, I2C_TIMEOUT_MS);

    for (int e = 0; e < count; e++) {
      const uint8_t *x8 = &ev[e * size];
      uint32_t uid = uid32(x8);
      uint8_t flags = x8[4];

      /* Touching Keybits queue their own key events too. Their key is read
       * directly, but a tap shorter than the key poll only shows up here:
       * a press the poll never saw, then its release, is a missed tap. */
      int direct = 0;
      for (int j = 0; j < n; j++) {
        if (uid32(k[j].uid) != uid) continue;
        direct = 1;
        if (flags & EV_HEARTBEAT) continue;   /* only key changes count */
        if (flags & EV_PRESSED) {
          if (!k[j].saw_down) k[j].pend = 1;
        } else {
          if (k[j].pend && !k[j].saw_down && !k[j].tap) k[j].tap = 1;
          k[j].pend = 0;
        }
      }
      if (direct) continue;

      keybit_remote_t *x = remote_slot(r, rn, uid);
      if (!x) continue;
      uint8_t down = flags & EV_PRESSED;
      uint8_t back = (flags & EV_BACKWARDS) != 0;
      /* Down and up again before the keys task saw the down: keep it as a tap. */
      if (down && !x->key_down) x->fresh_down = 1;
      if (!down && x->key_down && x->fresh_down && !x->tap) x->tap = 1;
      if (x->uid != uid || !x->present || x->key_down != down || x->backwards != back) {
        x->dirty = 1;
        changed = 1;
      }
      x->uid = uid;
      x->present = 1;
      x->key_down = down;
      x->backwards = back;
      x->depth = flags >> 4;
      x->via = k[i].addr;
      if (size == EV_SIZE11 && x8[10] != '?') {
        uint32_t parent = uid32(&x8[5]);
        if (parent != x->parent || x8[9] != x->pside || x8[10] != x->face) {
          x->parent = parent;
          x->pside = (char)x8[9];
          x->face = (char)x8[10];
          x->moved = 1;
          x->dirty = 1;
          changed = 1;
        }
      }
      x->last_seen = now;
    }
  }

  /* The next batch's events are measured against the polls until then. */
  for (int i = 0; i < n; i++) k[i].saw_down = k[i].key_down;

  for (int i = 0; i < *rn; i++) {
    if (r[i].present && now - r[i].last_seen > KEYBIT_REMOTE_TIMEOUT_MS) {
      r[i].present = 0;
      r[i].key_down = 0;
      r[i].backwards = 0;
      r[i].dirty = 1;
      changed = 1;
    }
  }
  return changed;
}

int keybit_bus_read(I2C_HandleTypeDef *i2c, uint8_t addr7, uint8_t reg, uint8_t *buf, uint8_t len)
{
  if (keybit_bus_jammed()) return 0;
  return read_regs(i2c, addr7, reg, buf, len);
}

int keybit_bus_write(I2C_HandleTypeDef *i2c, uint8_t addr7, uint8_t reg, const uint8_t *buf, uint8_t len)
{
  if (keybit_bus_jammed()) return 0;
  return HAL_I2C_Mem_Write(i2c, (uint16_t)(addr7 << 1), reg, I2C_MEMADD_SIZE_8BIT,
                           (uint8_t *)buf, len, I2C_TIMEOUT_MS) == HAL_OK;
}
