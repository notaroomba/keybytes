/* One Keybit: its real main.c, plus a few getters for the simulator. */
#define main keybit_main
#include "../keybit/main.c"
#undef main

uint8_t sim_node_depth(void) { return my_depth; }
uint8_t sim_node_qcount(void) { return q_count; }
const uint8_t *sim_node_diag(void) { return &diag[0][0]; }
uint8_t sim_node_backwards(void) { return backwards; }
