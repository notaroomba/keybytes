/*
 * Host check for Core/Src/stackimg.c with a made-up image that ends in
 * ST's CPU2 footer (the same layout as stm32wb5x_BLE_Stack_full_fw.bin).
 *   cc -std=c99 -ICore/Inc test/stackimg_test.c Core/Src/stackimg.c -o /tmp/stackimg_test && /tmp/stackimg_test
 *
 * With a real binary as argument it also checks that one:
 *   /tmp/stackimg_test stm32wb5x_BLE_Stack_full_fw.bin
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "stackimg.h"

static uint8_t flash[0x2000];

static void put(uint32_t off, uint32_t w)
{
  flash[off] = (uint8_t)w;
  flash[off + 1] = (uint8_t)(w >> 8);
  flash[off + 2] = (uint8_t)(w >> 16);
  flash[off + 3] = (uint8_t)(w >> 24);
}

int main(int argc, char **argv)
{
  /* erased flash: nothing */
  memset(flash, 0xFF, sizeof(flash));
  assert(STACKIMG_Find(flash, sizeof(flash)) == 0);

  /* an image of 0x1234 bytes (word sized, like ST's) with the footer at its end */
  for (uint32_t i = 0; i < 0x1234 - STACKIMG_FOOTER_LEN; i++) flash[i] = (uint8_t)(i * 7);
  put(0, 0x20030000);   /* first word not erased */
  uint32_t f = 0x1234 - STACKIMG_FOOTER_LEN;
  put(f, 0x12341234);
  put(f + 4, 0xABCDABCD);
  put(f + 8, 0xFFFF5740);
  put(f + 12, 0x22110045);
  put(f + 16, 0xD3A12C5E);
  assert(STACKIMG_Find(flash, sizeof(flash)) == 0x1234);

  /* a broken magic or a missing marker word is not an image */
  put(f + 16, 0xD3A12C5F);
  assert(STACKIMG_Find(flash, sizeof(flash)) == 0);
  put(f + 16, 0xD3A12C5E);
  put(f + 4, 0xABCDABCE);
  assert(STACKIMG_Find(flash, sizeof(flash)) == 0);
  put(f + 4, 0xABCDABCD);

  /* footer must fit in the window */
  assert(STACKIMG_Find(flash, 0x1234 - 1) == 0);
  assert(STACKIMG_Find(flash, 0x1234) == 0x1234);

  /* first word erased: nothing staged even if a footer is somewhere */
  put(0, 0xFFFFFFFF);
  assert(STACKIMG_Find(flash, sizeof(flash)) == 0);

  /* auto install only when FUS runs, is idle, an image is there, < 2 tries */
  assert(STACKIMG_ShouldAutoInstall(1, FUS_STATE_IDLE, 0x1234, 0));
  assert(STACKIMG_ShouldAutoInstall(1, FUS_STATE_IDLE, 0x1234, 1));
  assert(!STACKIMG_ShouldAutoInstall(1, FUS_STATE_IDLE, 0x1234, 2));
  assert(!STACKIMG_ShouldAutoInstall(0, FUS_STATE_IDLE, 0x1234, 0));   /* stack already running */
  assert(!STACKIMG_ShouldAutoInstall(1, 0x10, 0x1234, 0));             /* already upgrading */
  assert(!STACKIMG_ShouldAutoInstall(1, FUS_STATE_ERROR, 0x1234, 0));
  assert(!STACKIMG_ShouldAutoInstall(1, FUS_STATE_IDLE, 0, 0));        /* nothing staged */

  if (argc > 1)
  {
    static uint8_t img[0x40000];
    FILE *fp = fopen(argv[1], "rb");
    assert(fp);
    size_t n = fread(img, 1, sizeof(img), fp);
    fclose(fp);
    memset(img + n, 0xFF, sizeof(img) - n);   /* erased flash after it */
    uint32_t got = STACKIMG_Find(img, sizeof(img));
    printf("%s: %zu bytes, footer found -> %u bytes\n", argv[1], n, got);
    assert(got == n);
  }

  puts("stackimg_test: ok");
  return 0;
}
