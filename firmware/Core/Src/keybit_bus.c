#include "keybit_bus.h"
#include <string.h>

#define I2C_TIMEOUT_MS 10
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
#define EV_BATCH     8
#define EV_PRESSED   0x01
#define MAGIC      0x4B

static int is_ready(I2C_HandleTypeDef *i2c, uint8_t addr)
{
  return HAL_I2C_IsDeviceReady(i2c, (uint16_t)(addr << 1), 1, I2C_TIMEOUT_MS) == HAL_OK;
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
  case 'S': return 'T';
  default:  return '?';
  }
}

int keybit_bus_scan(I2C_HandleTypeDef *i2c, keybit_info_t *out, int max)
{
  int n = 0;
  for (uint8_t a = ADDR_FIRST; a <= ADDR_LAST && n < max; a++) {
    if (a == KEYBIT_DEFAULT_ADDR || a == LINK_ADDR || a == CHARGER_ADDR) continue;
    if (!is_ready(i2c, a)) continue;

    uint8_t head[9];
    if (!read_regs(i2c, a, REG_MAGIC, head, sizeof(head)) || head[0] != MAGIC) continue;

    keybit_info_t *k = &out[n];
    /* Reading the UID also catches two Keybits on one address: the loser of
     * the arbitration drops its address and turns up on the default one. */
    if (!read_regs(i2c, a, REG_UID, k->uid, sizeof(k->uid))) continue;
    k->addr = a;
    k->fw = head[1];
    k->key_down = head[6] & 1;
    k->keybit_side = (char)head[7];
    k->at = keyword_side(k->keybit_side);
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

int keybit_bus_poll_relay(I2C_HandleTypeDef *i2c, const keybit_info_t *k, int n,
                          keybit_remote_t *r, int *rn)
{
  int changed = 0;
  uint32_t now = HAL_GetTick();

  for (int i = 0; i < n; i++) {
    uint8_t count;
    if (!read_regs(i2c, k[i].addr, REG_EV_COUNT, &count, 1) || count == 0) continue;
    if (count > EV_BATCH) count = EV_BATCH;
    uint8_t ev[EV_BATCH * 5];
    if (!read_regs(i2c, k[i].addr, REG_EV_DATA, ev, (uint16_t)(count * 5))) continue;
    HAL_I2C_Mem_Write(i2c, (uint16_t)(k[i].addr << 1), REG_EV_POP, I2C_MEMADD_SIZE_8BIT,
                      &count, 1, I2C_TIMEOUT_MS);

    for (int e = 0; e < count; e++) {
      uint32_t uid = uid32(&ev[e * 5]);
      uint8_t flags = ev[e * 5 + 4];

      /* Touching Keybits queue their own events too; we already read
       * those directly. */
      int direct = 0;
      for (int j = 0; j < n; j++) if (uid32(k[j].uid) == uid) direct = 1;
      if (direct) continue;

      keybit_remote_t *x = remote_slot(r, rn, uid);
      if (!x) continue;
      uint8_t down = flags & EV_PRESSED;
      if (x->uid != uid || !x->present || x->key_down != down) {
        x->dirty = 1;
        changed = 1;
      }
      x->uid = uid;
      x->present = 1;
      x->key_down = down;
      x->depth = flags >> 4;
      x->via = k[i].addr;
      x->last_seen = now;
    }
  }

  for (int i = 0; i < *rn; i++) {
    if (r[i].present && now - r[i].last_seen > KEYBIT_REMOTE_TIMEOUT_MS) {
      r[i].present = 0;
      r[i].key_down = 0;
      r[i].dirty = 1;
      changed = 1;
    }
  }
  return changed;
}

int keybit_bus_read(I2C_HandleTypeDef *i2c, uint8_t addr7, uint8_t reg, uint8_t *buf, uint8_t len)
{
  return read_regs(i2c, addr7, reg, buf, len);
}

int keybit_bus_write(I2C_HandleTypeDef *i2c, uint8_t addr7, uint8_t reg, const uint8_t *buf, uint8_t len)
{
  return HAL_I2C_Mem_Write(i2c, (uint16_t)(addr7 << 1), reg, I2C_MEMADD_SIZE_8BIT,
                           (uint8_t *)buf, len, I2C_TIMEOUT_MS) == HAL_OK;
}
