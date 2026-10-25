#include "stackimg.h"

static uint32_t Word(const uint8_t *p)
{
  return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

uint32_t STACKIMG_Find(const uint8_t *data, uint32_t len)
{
  if (len < STACKIMG_FOOTER_LEN || Word(data) == 0xFFFFFFFFu) return 0;
  for (uint32_t off = 0; off + STACKIMG_FOOTER_LEN <= len; off += 4)
  {
    if (Word(data + off) == 0x12341234u && Word(data + off + 4) == 0xABCDABCDu
        && Word(data + off + 8) == 0xFFFF5740u && Word(data + off + 16) == 0xD3A12C5Eu)
    {
      return off + STACKIMG_FOOTER_LEN;
    }
  }
  return 0;
}

int STACKIMG_ShouldAutoInstall(int fus_running, uint8_t fus_state, uint32_t image_len, uint32_t attempts)
{
  return fus_running && fus_state == FUS_STATE_IDLE && image_len > 0 && attempts < 2;
}
