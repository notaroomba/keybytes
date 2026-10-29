#include "app_common.h"
#include "main.h"
#include "stm32_seq.h"
#include "shci.h"
#include "utilities_conf.h"
#include "hids_app.h"
#include "batt.h"
#include "app_ble.h"
#include "proto.h"
#include "keymap.h"
#include "usbd_kw.h"

#define KEYS_PERIOD_MS 5
#define MAP_MAGIC      0x50414D4BUL   /* "KMAP" */
#define MAP_VERSION    5   /* 2: kind + code, 3: hold, 4: macros, 5: short (32-bit) UIDs */
#define SEEN_MAX       16

/* Owned by main.c's Keybit code, refreshed from its own poll calls in the
 * main loop. The key task runs from the same loop, so no locking. */
extern keybit_info_t keybits[KEYBIT_MAX];
extern int keybit_count;
extern volatile uint32_t keybit_changed;
extern keybit_remote_t keybit_remotes[KEYBIT_REMOTE_MAX];
extern int keybit_remote_count;

_Static_assert(sizeof(map_entry_t) % 8 == 0, "map entries are written as doublewords");
_Static_assert(sizeof(map_image_t) <= FLASH_PAGE_SIZE, "keymap must fit its flash page");

static keybit_info_t present[KEYBIT_MAX];
static int present_count;
/* Tap/hold state for every key: direct Keybits in [0, KEYBIT_MAX)
 * following present[], remote ones at KEYBIT_MAX + their (fixed) index. */
static keyact_key_t key_state[KEYBIT_MAX + KEYBIT_REMOTE_MAX];
static uint8_t remote_shown[KEYBIT_REMOTE_MAX];   /* KEY+ sent, no KEY- yet */
static uint8_t remote_down[KEYBIT_REMOTE_MAX];    /* last PRESS sent */
static uint8_t remote_back[KEYBIT_REMOTE_MAX];    /* last BACKWARDS sent */
/* Full UIDs of Keybits seen directly, so one that moves further out can
 * still be named in full. */
static uint8_t seen[SEEN_MAX][UID_LEN];
static int seen_count;
static char sides[3];   /* last SIDES sent: L, R, T present */
static uint8_t keys_timer_id;

/* One page under SFSA (the first page CPU2 owns). With the v1.24 full
 * stack at 0x080D0000 that is page 0xCF at 0x080CF000. Reading SFSA keeps
 * it right if a different stack moves the boundary. */
static uint32_t Map_Page(void)
{
  return (FLASH->SFR & FLASH_SFR_SFSA) - 1;
}

static const map_image_t *Map_Flash(void)
{
  return (const map_image_t *)(FLASH_BASE + Map_Page() * FLASH_PAGE_SIZE);
}

static uint32_t Map_Size(const map_image_t *m)
{
  return 8 + m->count * sizeof(map_entry_t);
}

const keybit_info_t *KEYMAP_Present(int *count)
{
  *count = present_count;
  return present;
}

/* One erase or program, done when neither CPU is blocking flash access:
 * ST's sequence from BLE_RfWithFlash's flash_driver.c. CPU2 was told to
 * use semaphore 7 in APP_BLE_Init. Returns 0 when it ran. */
static int Flash_Op(uint8_t erase, uint32_t addr_or_page, uint64_t data)
{
  for (int tries = 0; tries < 1000; tries++)
  {
    int done = 0;
    while (LL_FLASH_IsActiveFlag_OperationSuspended());
    UTILS_ENTER_CRITICAL_SECTION();
    if (!LL_HSEM_GetStatus(HSEM, CFG_HW_BLOCK_FLASH_REQ_BY_CPU1_SEMID)
        && LL_HSEM_1StepLock(HSEM, CFG_HW_BLOCK_FLASH_REQ_BY_CPU2_SEMID) == 0)
    {
      if (erase)
      {
        FLASH_EraseInitTypeDef e = {.TypeErase = FLASH_TYPEERASE_PAGES, .Page = addr_or_page, .NbPages = 1};
        uint32_t page_error;
        HAL_FLASHEx_Erase(&e, &page_error);
      }
      else
      {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr_or_page, data);
      }
      LL_HSEM_ReleaseLock(HSEM, CFG_HW_BLOCK_FLASH_REQ_BY_CPU2_SEMID, 0);
      done = 1;
    }
    UTILS_EXIT_CRITICAL_SECTION();
    if (done)
    {
      while (__HAL_FLASH_GET_FLAG(FLASH_FLAG_CFGBSY));
      return 0;
    }
    HAL_Delay(1);   /* CPU2 is in a radio event, try again */
  }
  return -1;
}

int KEYMAP_Save(void)
{
  keytable.magic = MAP_MAGIC;
  keytable.version = MAP_VERSION;
  uint32_t size = Map_Size(&keytable);
  if (memcmp(Map_Flash(), &keytable, size) == 0) return 0;   /* nothing changed */

  int err = 0;
  while (LL_HSEM_1StepLock(HSEM, CFG_HW_FLASH_SEMID));
  HAL_FLASH_Unlock();
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);

  /* Without the BLE stack running there is nobody to answer SHCI. */
  if (APP_BLE_Ready()) SHCI_C2_FLASH_EraseActivity(ERASE_ACTIVITY_ON);
  err |= Flash_Op(1, Map_Page(), 0);
  if (APP_BLE_Ready()) SHCI_C2_FLASH_EraseActivity(ERASE_ACTIVITY_OFF);

  uint32_t addr = (uint32_t)Map_Flash();
  const uint64_t *src = (const uint64_t *)&keytable;
  for (uint32_t i = 0; i < size / 8 && !err; i++)
  {
    err |= Flash_Op(0, addr + 8 * i, src[i]);
  }

  HAL_FLASH_Lock();
  LL_HSEM_ReleaseLock(HSEM, CFG_HW_FLASH_SEMID, 0);
  if (err || memcmp(Map_Flash(), &keytable, size) != 0) return -1;
  return 0;
}

static void Map_Load(void)
{
  const map_image_t *f = Map_Flash();
  if (f->magic == MAP_MAGIC && f->version == MAP_VERSION && f->count <= KEYMAP_MAX)
  {
    memcpy(&keytable, f, Map_Size(f));
  }
  else
  {
    keytable.count = 0;
  }
}

static const keybit_info_t *Find_Keybit(const keybit_info_t *list, int n, const uint8_t uid[UID_LEN])
{
  for (int i = 0; i < n; i++)
  {
    if (memcmp(list[i].uid, uid, UID_LEN) == 0) return &list[i];
  }
  return NULL;
}

static uint32_t Low32(const uint8_t uid[UID_LEN])
{
  return (uint32_t)uid[0] | (uint32_t)uid[1] << 8 | (uint32_t)uid[2] << 16 | (uint32_t)uid[3] << 24;
}

/* ponytail: the oldest of SEEN_MAX sightings is forgotten first */
static void Seen(const uint8_t uid[UID_LEN])
{
  int n = seen_count < SEEN_MAX ? seen_count : SEEN_MAX;
  for (int i = 0; i < n; i++)
  {
    if (memcmp(seen[i], uid, UID_LEN) == 0) return;
  }
  memcpy(seen[seen_count % SEEN_MAX], uid, UID_LEN);
  seen_count++;
}

/* A remote Keybit's UID as the protocol prints it: all 24 hex digits when
 * the keymap or an earlier direct sighting knows them, else the 8 it
 * reports. */
static void Remote_Uid(char *out, uint32_t low)
{
  const uint8_t *full = KEYMAP_FullUid(low);
  int n = seen_count < SEEN_MAX ? seen_count : SEEN_MAX;
  for (int i = 0; !full && i < n; i++)
  {
    if (Low32(seen[i]) == low) full = seen[i];
  }
  if (full) PROTO_FormatUid(out, full);
  else snprintf(out, 9, "%08lX", (unsigned long)low);
}

/* via is the address of the touching Keybit it came through; "?" if that
 * one dropped out of the last scan. */
static void Remote_Line(char *out, size_t n, const keybit_remote_t *r)
{
  char uid[UID_LEN * 2 + 1], via[UID_LEN * 2 + 1] = "?";
  Remote_Uid(uid, r->uid);
  for (int i = 0; i < present_count; i++)
  {
    if (present[i].addr == r->via) PROTO_FormatUid(via, present[i].uid);
  }
  int len = snprintf(out, n, "KEY+ uid=%s at=X via=%s depth=%u", uid, via, r->depth);
  /* v11: what it hangs off, which of that one's sides, and its own side facing it */
  if (r->face && r->face != '?' && len > 0 && (size_t)len < n)
    snprintf(out + len, n - (size_t)len, " parent=%08lX pside=%c face=%c", (unsigned long)r->parent, r->pside, r->face);
}

int KEYMAP_KeyLine(int i, char *out, size_t n)
{
  char uid[UID_LEN * 2 + 1];
  if (i < present_count)
  {
    PROTO_FormatUid(uid, present[i].uid);
    /* v: Keybit reg 0x01; face: its own side touching the keyword */
    snprintf(out, n, "KEY+ uid=%s at=%c v=%u face=%c", uid, present[i].at, present[i].fw, present[i].keybit_side);
    return 1;
  }
  i -= present_count;
  for (int r = 0; r < keybit_remote_count; r++)
  {
    if (remote_shown[r] && i-- == 0)
    {
      Remote_Line(out, n, &keybit_remotes[r]);
      return 1;
    }
  }
  return 0;
}

/* Direct Keybits: diff keybits[] against what we reported last time, by
 * UID, since indexes move every time main.c rescans the bus. */
/* BACKWARDS uid=<uid> sides=<letters>: a Keybit sees a neighbour on
 * backwards on those of its own sides (N, E, S, W), "?" when it's relayed
 * and we only know that it has one, "-" once it's gone. */
static void Send_Backwards(const char *uid, uint8_t mask, int known)
{
  char sides[5] = "-", *p = sides;
  if (mask && !known) sides[0] = '?';
  for (int b = 0; known && b < 4; b++)
  {
    if (mask & (1u << b)) *p++ = "NESW"[b];
  }
  if (known && mask) *p = 0;
  PROTO_Send("BACKWARDS uid=%s sides=%s", uid, sides);
}

static void Direct_Keys(void)
{
  char uid[UID_LEN * 2 + 1];
  char line[PROTO_LINE_MAX];

  for (int i = 0; i < present_count; i++)
  {
    if (!Find_Keybit(keybits, keybit_count, present[i].uid))
    {
      PROTO_FormatUid(uid, present[i].uid);
      PROTO_Send("KEY- uid=%s", uid);
    }
  }

  keybit_info_t old[KEYBIT_MAX];
  keyact_key_t state[KEYBIT_MAX];
  int old_count = present_count;
  memcpy(old, present, sizeof(keybit_info_t) * old_count);
  present_count = keybit_count;
  memcpy(present, keybits, sizeof(keybit_info_t) * keybit_count);
  /* A tap the key poll missed: down for one pass, then up on the next. */
  for (int i = 0; i < keybit_count; i++)
  {
    if (keybits[i].tap == 1) { present[i].key_down = 1; keybits[i].tap = 2; }
    else if (keybits[i].tap == 2) keybits[i].tap = 0;
  }

  for (int i = 0; i < keybit_count; i++)
  {
    const keybit_info_t *was = Find_Keybit(old, old_count, keybits[i].uid);
    PROTO_FormatUid(uid, keybits[i].uid);
    if (!was)
    {
      Seen(keybits[i].uid);
      KEYMAP_KeyLine(i, line, sizeof(line));
      PROTO_Send("%s", line);
    }
    if ((was ? was->key_down : 0) != present[i].key_down)
    {
      PROTO_Send("PRESS uid=%s down=%d", uid, present[i].key_down);
    }
    if ((was ? was->backwards : 0) != keybits[i].backwards)
    {
      Send_Backwards(uid, keybits[i].backwards, 1);
    }
    if (was) state[i] = key_state[was - old];
    else memset(&state[i], 0, sizeof(state[i]));
    state[i].down = present[i].key_down;
  }
  memcpy(key_state, state, sizeof(keyact_key_t) * keybit_count);
  for (int i = keybit_count; i < KEYBIT_MAX; i++)
  {
    memset(&key_state[i], 0, sizeof(key_state[i]));   /* free slot */
  }

  char now[3] = {0, 0, 0};
  for (int i = 0; i < present_count; i++)
  {
    if (present[i].at == 'L') now[0] = 1;
    if (present[i].at == 'R') now[1] = 1;
    if (present[i].at == 'T') now[2] = 1;
  }
  if (memcmp(now, sides, sizeof(sides)) != 0)
  {
    memcpy(sides, now, sizeof(sides));
    PROTO_Send("SIDES L=%d R=%d T=%d", sides[0], sides[1], sides[2]);
  }
}

/* Remote Keybits: entries never move, so their index is their identity. */
static void Remote_Keys(void)
{
  char uid[UID_LEN * 2 + 1];
  char line[PROTO_LINE_MAX];

  for (int r = 0; r < keybit_remote_count; r++)
  {
    keybit_remote_t *k = &keybit_remotes[r];
    keyact_key_t *s = &key_state[KEYBIT_MAX + r];
    uint8_t down = k->present && (k->key_down || k->tap == 1);
    uint8_t back = k->present && k->backwards;
    /* A tap that came and went in one relay batch: down this pass, up the next. */
    if (k->tap == 1) { k->tap = 2; k->dirty = 1; }
    else if (k->tap == 2) k->tap = 0;
    if (down) k->fresh_down = 0;
    if (!k->dirty && k->present == remote_shown[r] && down == remote_down[r] && back == remote_back[r]) continue;
    k->dirty = 0;

    Remote_Uid(uid, k->uid);
    if (k->present && !remote_shown[r])
    {
      remote_shown[r] = 1;
      k->moved = 0;
      memset(s, 0, sizeof(*s));
      Remote_Line(line, sizeof(line), k);
      PROTO_Send("%s", line);
    }
    else if (k->present && k->moved)
    {
      k->moved = 0;   /* same Keybit, now known (or known to be) somewhere else */
      Remote_Line(line, sizeof(line), k);
      PROTO_Send("%s", line);
    }
    if (down != remote_down[r])
    {
      remote_down[r] = down;
      PROTO_Send("PRESS uid=%s down=%d", uid, down);
    }
    if (back != remote_back[r])
    {
      remote_back[r] = back;
      if (k->present) Send_Backwards(uid, back, 0);
    }
    if (!k->present && remote_shown[r])
    {
      remote_shown[r] = 0;
      PROTO_Send("KEY- uid=%s", uid);
    }
    s->down = down;
  }
}

static void Keys_Task(void)
{
  keybit_changed = 0;   /* only main-loop code sets it, no ISR */
  Direct_Keys();
  Remote_Keys();

  /* Direct Keybits by full UID, remote ones by their low 32 bits (which
   * also finds an entry stored with the full UID). Unmapped ones get NULL
   * and send nothing. */
  static const keyact_t *acts[KEYBIT_MAX + KEYBIT_REMOTE_MAX];
  keyid_t id;
  for (int i = 0; i < KEYBIT_MAX; i++)
  {
    acts[i] = NULL;
    if (i >= present_count) continue;
    memcpy(id.uid, present[i].uid, UID_LEN);
    id.len = UID_LEN;
    acts[i] = KEYMAP_Lookup(&id);
  }
  for (int r = 0; r < keybit_remote_count; r++)
  {
    memset(&id, 0, sizeof(id));
    for (int b = 0; b < UID_SHORT_LEN; b++) id.uid[b] = (uint8_t)(keybit_remotes[r].uid >> (8 * b));
    id.len = UID_SHORT_LEN;
    acts[KEYBIT_MAX + r] = remote_shown[r] ? KEYMAP_Lookup(&id) : NULL;
  }

  uint8_t report[8];
  uint16_t consumer;
  KEYACT_Step(key_state, acts, KEYBIT_MAX + keybit_remote_count, HAL_GetTick(), report, &consumer);
  /* Plugged into a computer: type over USB, from the keymap in flash or
   * whatever the app last sent, with or without the app running. Only
   * over Bluetooth otherwise, so a computer that has both doesn't get
   * every key twice. */
  extern USBD_HandleTypeDef hUsbDeviceFS;
  if (USBD_KW_Ready(&hUsbDeviceFS))
  {
    USBD_KW_Keyboard(&hUsbDeviceFS, report);
    USBD_KW_Media(&hUsbDeviceFS, consumer);
  }
  else
  {
    HIDSAPP_SendKeyboard(report);
    HIDSAPP_SendConsumer(consumer);
  }

  /* same task as the keys: the charger shares I2C3 with the Keybits */
  BATT_Poll(HAL_GetTick());
}

void KEYMAP_Kick(void)
{
  UTIL_SEQ_SetTask(1 << CFG_TASK_KEYS_ID, CFG_SCH_PRIO_0);
}

static void Keys_Tick(void)
{
  UTIL_SEQ_SetTask(1 << CFG_TASK_KEYS_ID, CFG_SCH_PRIO_0);
}

void KEYMAP_Init(void)
{
  Map_Load();
  UTIL_SEQ_RegTask(1 << CFG_TASK_KEYS_ID, UTIL_SEQ_RFU, Keys_Task);
  HW_TS_Create(CFG_TIM_PROC_ID_ISR, &keys_timer_id, hw_ts_Repeated, Keys_Tick);
  HW_TS_Start(keys_timer_id, MS_TO_TS_TICKS(KEYS_PERIOD_MS));
}
