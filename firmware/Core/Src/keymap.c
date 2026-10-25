#include "app_common.h"
#include "main.h"
#include "stm32_seq.h"
#include "shci.h"
#include "utilities_conf.h"
#include "hids_app.h"
#include "app_ble.h"
#include "proto.h"
#include "keymap.h"

#define KEYS_PERIOD_MS 5
#define MAP_MAGIC      0x50414D4BUL   /* "KMAP" */
#define MAP_VERSION    4   /* 2: kind + 16-bit code, 3: hold action, 4: macros */

/* Owned by main.c's Keybit code, refreshed from its own poll calls in the
 * main loop. The key task runs from the same loop, so no locking. */
extern keybit_info_t keybits[KEYBIT_MAX];
extern int keybit_count;
extern volatile uint32_t keybit_changed;

/* 56 bytes, seven flash doublewords. 32 of them plus the header are
 * 1800 bytes, under half the 4 KB page. */
typedef struct {
  uint8_t uid[UID_LEN];
  keyact_t act;
  uint8_t pad[2];
} map_entry_t;

/* Flash image: one header doubleword, then the entries. */
typedef struct {
  uint32_t magic;
  uint16_t version;
  uint16_t count;
  map_entry_t entries[KEYMAP_MAX];
} map_image_t;

_Static_assert(sizeof(map_entry_t) % 8 == 0, "map entries are written as doublewords");
_Static_assert(sizeof(map_image_t) <= FLASH_PAGE_SIZE, "keymap must fit its flash page");

static map_image_t map __attribute__((aligned(8)));
static keybit_info_t present[KEYBIT_MAX];
static keyact_key_t present_key[KEYBIT_MAX];   /* tap/hold state, follows present[] */
static int present_count;
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

static int Find(const uint8_t uid[UID_LEN])
{
  for (int i = 0; i < map.count; i++)
  {
    if (memcmp(map.entries[i].uid, uid, UID_LEN) == 0) return i;
  }
  return -1;
}

/* Entry for uid, created empty if needed. NULL when the table is full. */
static keyact_t *Entry(const uint8_t uid[UID_LEN])
{
  int i = Find(uid);
  if (i < 0)
  {
    if (map.count >= KEYMAP_MAX) return NULL;
    i = map.count++;
    memset(&map.entries[i], 0, sizeof(map_entry_t));
    memcpy(map.entries[i].uid, uid, UID_LEN);
  }
  return &map.entries[i].act;
}

/* A Keybit has one mapping, so setting either kind replaces the other. */
static int Put(const uint8_t uid[UID_LEN], uint8_t kind, uint8_t mods, uint16_t code)
{
  keyact_t *a = Entry(uid);
  if (!a) return -1;
  a->kind = kind;
  a->mods = mods;
  a->code = code;
  a->macro_len = 0;   /* MAP/MAPC replace a macro too */
  return 0;
}

int KEYMAP_Set(const uint8_t uid[UID_LEN], uint8_t mods, uint8_t key)
{
  return Put(uid, KEYACT_KEY, mods, key);
}

int KEYMAP_SetConsumer(const uint8_t uid[UID_LEN], uint16_t usage)
{
  return Put(uid, KEYACT_CONSUMER, 0, usage);
}

int KEYMAP_SetMacro(const uint8_t uid[UID_LEN], const uint8_t seq[][2], uint8_t len)
{
  if (len > KEYACT_MACRO_MAX) return -1;
  keyact_t *a = Entry(uid);
  if (!a) return -1;
  a->kind = KEYACT_MACRO;
  a->mods = 0;
  a->code = 0;
  a->macro_len = len;
  memset(a->macro, 0, sizeof(a->macro));
  memcpy(a->macro, seq, (size_t)len * 2);
  return 0;
}

int KEYMAP_SetHold(const uint8_t uid[UID_LEN], uint8_t mods, uint8_t key, uint16_t ms)
{
  if (!mods && !key)
  {
    int i = Find(uid);
    if (i < 0) return 0;
    keyact_t *a = &map.entries[i].act;
    a->hold_mods = a->hold_key = 0;
    a->hold_ms = 0;
    if (a->kind == KEYACT_NONE) KEYMAP_Unmap(uid);   /* nothing left */
    return 0;
  }
  keyact_t *a = Entry(uid);
  if (!a) return -1;
  a->hold_mods = mods;
  a->hold_key = key;
  a->hold_ms = ms;
  return 0;
}

const keyact_t *KEYMAP_Lookup(const uint8_t uid[UID_LEN])
{
  int i = Find(uid);
  return i < 0 ? NULL : &map.entries[i].act;
}

void KEYMAP_Unmap(const uint8_t uid[UID_LEN])
{
  int i = Find(uid);
  if (i < 0) return;
  map.entries[i] = map.entries[--map.count];
}

void KEYMAP_Clear(void)
{
  map.count = 0;
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
  map.magic = MAP_MAGIC;
  map.version = MAP_VERSION;
  uint32_t size = Map_Size(&map);
  if (memcmp(Map_Flash(), &map, size) == 0) return 0;   /* nothing changed */

  int err = 0;
  while (LL_HSEM_1StepLock(HSEM, CFG_HW_FLASH_SEMID));
  HAL_FLASH_Unlock();
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);

  /* Without the BLE stack running there is nobody to answer SHCI. */
  if (APP_BLE_Ready()) SHCI_C2_FLASH_EraseActivity(ERASE_ACTIVITY_ON);
  err |= Flash_Op(1, Map_Page(), 0);
  if (APP_BLE_Ready()) SHCI_C2_FLASH_EraseActivity(ERASE_ACTIVITY_OFF);

  uint32_t addr = (uint32_t)Map_Flash();
  const uint64_t *src = (const uint64_t *)&map;
  for (uint32_t i = 0; i < size / 8 && !err; i++)
  {
    err |= Flash_Op(0, addr + 8 * i, src[i]);
  }

  HAL_FLASH_Lock();
  LL_HSEM_ReleaseLock(HSEM, CFG_HW_FLASH_SEMID, 0);
  if (err || memcmp(Map_Flash(), &map, size) != 0) return -1;
  return 0;
}

static void Map_Load(void)
{
  const map_image_t *f = Map_Flash();
  if (f->magic == MAP_MAGIC && f->version == MAP_VERSION && f->count <= KEYMAP_MAX)
  {
    memcpy(&map, f, Map_Size(f));
  }
  else
  {
    map.count = 0;
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

/* Diff keybits[] against what we reported last time, by UID: indexes move
 * every time main.c rescans the bus. */
static void Keys_Task(void)
{
  char uid[UID_LEN * 2 + 1];
  keybit_changed = 0;   /* only main-loop code sets it, no ISR */

  for (int i = 0; i < present_count; i++)
  {
    if (!Find_Keybit(keybits, keybit_count, present[i].uid))
    {
      PROTO_FormatUid(uid, present[i].uid);
      PROTO_Send("KEY- uid=%s", uid);
    }
  }
  keyact_key_t state[KEYBIT_MAX];
  for (int i = 0; i < keybit_count; i++)
  {
    const keybit_info_t *old = Find_Keybit(present, present_count, keybits[i].uid);
    uint8_t was_down = old ? old->key_down : 0;
    PROTO_FormatUid(uid, keybits[i].uid);
    if (!old) PROTO_Send("KEY+ uid=%s at=%c v=%u", uid, keybits[i].at, keybits[i].fw);   /* v: Keybit reg 0x01 */
    if (was_down != keybits[i].key_down)
    {
      PROTO_Send("PRESS uid=%s down=%d", uid, keybits[i].key_down);
    }
    if (old) state[i] = present_key[old - present];
    else memset(&state[i], 0, sizeof(state[i]));
    state[i].down = keybits[i].key_down;
  }

  present_count = keybit_count;
  memcpy(present, keybits, sizeof(keybit_info_t) * keybit_count);
  memcpy(present_key, state, sizeof(keyact_key_t) * keybit_count);

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

  /* Unmapped Keybits get a NULL action and send nothing. A Keybit that
   * leaves while held just drops out of the reports. */
  const keyact_t *acts[KEYBIT_MAX];
  for (int i = 0; i < present_count; i++) acts[i] = KEYMAP_Lookup(present[i].uid);
  uint8_t report[8];
  uint16_t consumer;
  KEYACT_Step(present_key, acts, present_count, HAL_GetTick(), report, &consumer);
  HIDSAPP_SendKeyboard(report);
  HIDSAPP_SendConsumer(consumer);
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
