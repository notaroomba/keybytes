#include <string.h>
#include "keytable.h"

map_image_t keytable __attribute__((aligned(8)));

static int Exact(const map_entry_t *e, const keyid_t *id)
{
  return e->uid_len == id->len && memcmp(e->uid, id->uid, id->len) == 0;
}

/* Low 32 bits agree and one side only has those. */
static int Loose(const map_entry_t *e, const keyid_t *id)
{
  return (e->uid_len == UID_SHORT_LEN || id->len == UID_SHORT_LEN)
         && memcmp(e->uid, id->uid, UID_SHORT_LEN) == 0;
}

static int Find(const keyid_t *id)
{
  for (int i = 0; i < keytable.count; i++)
  {
    if (Exact(&keytable.entries[i], id)) return i;
  }
  for (int i = 0; i < keytable.count; i++)
  {
    if (Loose(&keytable.entries[i], id)) return i;
  }
  return -1;
}

/* Entry for id, created empty if needed. NULL when the table is full. */
static keyact_t *Entry(const keyid_t *id)
{
  int i = Find(id);
  if (i < 0)
  {
    if (keytable.count >= KEYMAP_MAX) return NULL;
    i = keytable.count++;
    memset(&keytable.entries[i], 0, sizeof(map_entry_t));
  }
  map_entry_t *e = &keytable.entries[i];
  if (e->uid_len != UID_LEN)   /* new, or a short entry now seen in full */
  {
    memcpy(e->uid, id->uid, id->len);
    e->uid_len = id->len;
  }
  return &e->act;
}

static int Put(const keyid_t *id, uint8_t kind, uint8_t mods, uint16_t code)
{
  keyact_t *a = Entry(id);
  if (!a) return -1;
  a->kind = kind;
  a->mods = mods;
  a->code = code;
  a->macro_len = 0;   /* MAP/MAPC replace a macro too */
  return 0;
}

int KEYMAP_Set(const keyid_t *id, uint8_t mods, uint8_t key)
{
  return Put(id, KEYACT_KEY, mods, key);
}

int KEYMAP_SetConsumer(const keyid_t *id, uint16_t usage)
{
  return Put(id, KEYACT_CONSUMER, 0, usage);
}

int KEYMAP_SetMacro(const keyid_t *id, const uint8_t seq[][2], uint8_t len)
{
  if (len > KEYACT_MACRO_MAX) return -1;
  keyact_t *a = Entry(id);
  if (!a) return -1;
  a->kind = KEYACT_MACRO;
  a->mods = 0;
  a->code = 0;
  a->macro_len = len;
  memset(a->macro, 0, sizeof(a->macro));
  memcpy(a->macro, seq, (size_t)len * 2);
  return 0;
}

int KEYMAP_SetHold(const keyid_t *id, uint8_t mods, uint8_t key, uint16_t ms)
{
  if (!mods && !key)
  {
    int i = Find(id);
    if (i < 0) return 0;
    keyact_t *a = &keytable.entries[i].act;
    a->hold_mods = a->hold_key = 0;
    a->hold_ms = 0;
    if (a->kind == KEYACT_NONE) KEYMAP_Unmap(id);   /* nothing left */
    return 0;
  }
  keyact_t *a = Entry(id);
  if (!a) return -1;
  a->hold_mods = mods;
  a->hold_key = key;
  a->hold_ms = ms;
  return 0;
}

const keyact_t *KEYMAP_Lookup(const keyid_t *id)
{
  int i = Find(id);
  return i < 0 ? NULL : &keytable.entries[i].act;
}

void KEYMAP_Unmap(const keyid_t *id)
{
  int i = Find(id);
  if (i < 0) return;
  keytable.entries[i] = keytable.entries[--keytable.count];
}

void KEYMAP_Clear(void)
{
  keytable.count = 0;
}

const uint8_t *KEYMAP_FullUid(uint32_t low)
{
  for (int i = 0; i < keytable.count; i++)
  {
    const map_entry_t *e = &keytable.entries[i];
    uint32_t v = (uint32_t)e->uid[0] | (uint32_t)e->uid[1] << 8 | (uint32_t)e->uid[2] << 16 | (uint32_t)e->uid[3] << 24;
    if (e->uid_len == UID_LEN && v == low) return e->uid;
  }
  return NULL;
}
