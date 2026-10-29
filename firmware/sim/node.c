/* One Keybit: its real main.c, plus a few getters for the simulator. */
#define main keybit_main
#include "main.c"   /* the Keybit's, found through -I */
#undef main

/* link.c is built with its CRC renamed, so main.c's calls land here and pay
 * for the work: the bitwise CRC-8 is ~120 cycles a byte on the CH32V003.
 * The status stream works out an event's CRC while the Keyword is clocking
 * the read, which is where this matters. CRC_CYCLES overrides it. */
#include <stdlib.h>
uint8_t link_crc8_real(uint8_t crc, uint8_t v);
uint8_t link_crc8(uint8_t crc, uint8_t v)
{
  static int cycles = -1;
  if (cycles < 0) { const char *e = getenv("CRC_CYCLES"); cycles = e ? atoi(e) : 120; }
  sim_cycles((uint32_t)cycles);
  return link_crc8_real(crc, v);
}

uint8_t sim_node_depth(void) { return my_depth; }
uint8_t sim_node_qcount(void) { return q_count; }
const uint8_t *sim_node_diag(void) { return &diag[0][0]; }
uint8_t sim_node_backwards(void) { return backwards; }

/* Is a press event from this UID (low 32 bits) in our queue? */
int sim_node_has_press(uint32_t u)
{
  for (int i = 0; i < q_count; i++) {
    const event_t *e = queue_at(i);
    uint32_t v = (uint32_t)e->uid[0] | (uint32_t)e->uid[1] << 8 | (uint32_t)e->uid[2] << 16 | (uint32_t)e->uid[3] << 24;
    if (v == u && (e->flags & EV_PRESSED)) return 1;
  }
  return 0;
}
uint8_t sim_node_key(void) { return key_down; }
uint8_t sim_node_addr(void) { return my_addr; }
