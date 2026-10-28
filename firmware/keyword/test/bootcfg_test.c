/*
 * Host check for the option byte math in Core/Inc/bootcfg.h: BOOTLOCK
 * changes nSWBOOT0 and nBOOT0 and nothing else, RDP above all.
 *   cc -std=c99 -ICore/Inc test/bootcfg_test.c -o /tmp/bootcfg_test && /tmp/bootcfg_test
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "bootcfg.h"

/* An OPTR with RDP level 0 and BOOT0 from the pin (nSWBOOT0 = 1) */
#define FACTORY 0x3FFFF1AAu

static uint32_t rng = 1;
static uint32_t next(void)
{
  rng = rng * 1103515245u + 12345u;
  return (rng >> 16) | (rng << 16);
}

int main(void)
{
  const uint32_t two = BOOTCFG_nSWBOOT0 | BOOTCFG_nBOOT0;

  /* the bits match the CMSIS header (stm32wb55xx.h FLASH_OPTR_*) */
  assert(BOOTCFG_nBOOT1 == 0x00800000u && BOOTCFG_nSWBOOT0 == 0x04000000u);
  assert(BOOTCFG_nBOOT0 == 0x08000000u && BOOTCFG_RDP == 0xFFu);

  uint32_t v = BOOTCFG_LockValue(FACTORY);
  assert(!(v & BOOTCFG_nSWBOOT0) && (v & BOOTCFG_nBOOT0));
  assert((v & ~two) == (FACTORY & ~two));
  assert(BOOTCFG_IsLocked(v) && !BOOTCFG_IsLocked(FACTORY));
  assert(BOOTCFG_LockValue(v) == v);   /* a second BOOTLOCK writes nothing new */

  /* every other bit, RDP included, comes through untouched for any OPTR */
  for (int i = 0; i < 100000; i++)
  {
    uint32_t o = next();
    uint32_t n = BOOTCFG_LockValue(o);
    assert((n & ~two) == (o & ~two));
    assert((n & BOOTCFG_RDP) == (o & BOOTCFG_RDP));
    assert((n & two) == BOOTCFG_nBOOT0);
  }

  /* refused unless 10 s up, stack running and RDP level 0 */
  assert(strcmp(BOOTCFG_Check(FACTORY, 10000, 1), "app has not run 10 s yet") == 0);
  assert(strcmp(BOOTCFG_Check(FACTORY, 20000, 0), "BLE stack is not running") == 0);
  assert(strcmp(BOOTCFG_Check((FACTORY & ~0xFFu) | 0xBB, 20000, 1), "RDP is not level 0") == 0);
  assert(strcmp(BOOTCFG_Check((FACTORY & ~0xFFu) | 0xCC, 20000, 1), "RDP is not level 0") == 0);
  assert(BOOTCFG_Check(FACTORY, 10001, 1) == NULL);

  char s[64];
  BOOTCFG_Format(FACTORY, s, sizeof(s));
  assert(strcmp(s, "nBOOT0=1 nSWBOOT0=1 nBOOT1=1 RDP=0xAA") == 0);
  BOOTCFG_Format(v, s, sizeof(s));
  assert(strcmp(s, "nBOOT0=1 nSWBOOT0=0 nBOOT1=1 RDP=0xAA") == 0);

  puts("bootcfg_test: ok");
  return 0;
}
