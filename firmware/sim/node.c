/* One Keybit: its real main.c, plus a few getters for the simulator. */
#define main keybit_main
#include "main.c"   /* the Keybit's, found through -I */
#undef main

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
