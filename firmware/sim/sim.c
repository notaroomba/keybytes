/*
 * Keybit network simulator.
 *
 * Every Keybit runs the real firmware (main.c, link.c, soft_i2c.c), loaded
 * as its own copy of node.bundle so each has its own globals, on its own
 * clock (48 MHz HSI give or take 1%, SysTick at HCLK/8). The Keyword runs
 * the real keybit_bus.c over a model of I2C3 (kw.c). Each is a coroutine;
 * every register access costs the caller some cycles and the one furthest
 * behind in time always runs next, so all of them stay within one access
 * of each other.
 *
 * Wires: each side contact pair is a wire with the 10k pull-ups of every
 * Keybit on it (and the Keyword's on its bus), open drain or push-pull
 * drivers, and an RC rise after the last low driver lets go. Two Keybits
 * meet contact i to contact 3-i; a side's contacts going clockwise are
 * 3V3 SDA GND SCL on N and W and SCL GND SDA 3V3 on E and S (keybit.kicad_pcb),
 * so an N or W side must meet an E or S side. The Keyword's L and T sides
 * are like N/W, its R side like E/S. A Keybit is powered while a 3V3 path
 * of correct contacts reaches the Keyword; it boots from scratch whenever
 * it gains power.
 *
 * Apple arm64 only (ctx_arm64.S).
 */
#include <dlfcn.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "sim.h"
#include "ch32fun.h"

#define MAXN   40
#define NPIN   64
#define STACK  (256 * 1024)
#define LOG_MAX 200000

void ctx_switch(void **save, void *to);
void ctx_boot(void);

enum { KEYWORD, KEYBIT };
enum { N_, E_, S_, W_ };
enum { R_V, R_G, R_SDA, R_SCL };

typedef struct {
  int kind, idx;
  int64_t t, t_boot, op_ps, spin_ps, pass_ps;
  double ps_per_tick;
  uint32_t tick0;
  void *sp;
  char *stack;
  int placed, powered, runnable;
  int x, y, rot;
  uint8_t mode[NPIN], out[NPIN];
  int button;
  int (*entry)(void);
  uint8_t (*depth)(void);
  uint8_t (*qcount)(void);
  const uint8_t *(*diag)(void);
  int (*has_press)(uint32_t);
  uint8_t (*addr)(void);
  uint8_t (*key)(void);
  sim_systick_t st;
  sim_gpio_t gpio[4];
  sim_esig_t esig;
  sim_flash_t flash;
  uint32_t cfg[16];
  int boots;
} node_t;

typedef struct {
  int tie;            /* +1 on someone's 3V3, -1 on someone's GND */
  int low, pp_high;
  int npull;
  int64_t rel_at, rise;
  int64_t glitch_from, glitch_to;
  int m[24], nm;      /* members: node * NPIN + pin */
} wire_t;

static node_t nodes[MAXN];
static int n_nodes;
static node_t *cur;
static void *host_sp;
static int64_t t_end, next_min, sim_t;
static wire_t wires[MAXN * 8 + 4];
static int n_wires;
static int pin_wire[MAXN][NPIN];
static long shorts, fights;

/* knobs */
static int op_cycles = 16;    /* a pin or SysTick access, with the code around it */
static int spin_cycles = 5;   /* one pass of link.c's tight spin on the input register */
/* The main loop's own work between register accesses (depth, doorbell,
 * side bookkeeping), charged once per pass on its one read of the key pin.
 * Fitted to the real v10 Keybits on 2026-10-29: `I2CRD 0a 00 4` after
 * I2CSPEED succeeded 29/30 at 10 kHz, 30/30 at 15, 26/30 at 20, 13/20 at 25,
 * 7/20 at 35 and 0/20 at 50 kHz. With 1100 the sim gets 20, 20, 18, 12, 10
 * and 3 of 20 (`PROBE_READ=1 ./sim node_v10.bundle speed`). */
static int pass_cycles = 1100;
static double c_pf = 15, kw_pullup_k = 4.7;
static unsigned seed = 1;
static uint32_t tick0_all;
static char bundle[1024], tmpdir[512];
static const char *bundle2;   /* BUNDLE2: firmware for the Keybits listed in MIX */
static int mixed[MAXN];
static double noise_per_s;    /* NOISE: glitches per second on random wires */

static const uint8_t SDA_PIN[4] = { PA1, PC1, PC4, PD4 };   /* N E S W */
static const uint8_t SCL_PIN[4] = { PA2, PC2, PD2, PD5 };
static const int ROLE[2][4] = { { R_V, R_SDA, R_G, R_SCL },    /* N, W and the Keyword's L, T */
                                { R_SCL, R_G, R_SDA, R_V } };  /* E, S and the Keyword's R */
static const int TYPE[4] = { 0, 1, 1, 0 };
static const int DX[4] = { 0, 1, 0, -1 }, DY[4] = { -1, 0, 1, 0 };

static uint32_t rnd(void) { seed = seed * 1103515245u + 12345u; return seed >> 8; }
static double frand(void) { return (rnd() & 0xFFFFFF) / (double)0x1000000; }

/* ------------------------------------------------------------------------ */
/* Scheduler                                                                */

static void set_next_min(void)
{
  int64_t m = t_end;
  for (int i = 0; i < n_nodes; i++)
    if (&nodes[i] != cur && nodes[i].runnable && nodes[i].t < m) m = nodes[i].t;
  next_min = m;
}

static void schedule(void)
{
  node_t *m = NULL;
  for (int i = 0; i < n_nodes; i++)
    if (nodes[i].runnable && (!m || nodes[i].t < m->t)) m = &nodes[i];
  node_t *from = cur;
  if (!m || m->t >= t_end) { cur = NULL; ctx_switch(&from->sp, host_sp); return; }
  if (m == from) { set_next_min(); return; }
  cur = m;
  set_next_min();
  ctx_switch(&from->sp, m->sp);
}

static inline void advance(int64_t ps)
{
  cur->t += ps;
  if (cur->t > next_min) schedule();
}

void node_entry(node_t *n)
{
  if (n->kind == KEYBIT) n->entry();
  else kw_main();
  n->runnable = 0;
  for (;;) schedule();
}

static void run_until(int64_t t)
{
  t_end = t;
  for (;;) {
    node_t *m = NULL;
    for (int i = 0; i < n_nodes; i++)
      if (nodes[i].runnable && (!m || nodes[i].t < m->t)) m = &nodes[i];
    if (!m || m->t >= t) break;
    cur = m;
    set_next_min();
    ctx_switch(&host_sp, m->sp);
  }
  sim_t = t;
}

static void start_coroutine(node_t *n)
{
  if (!n->stack) n->stack = malloc(STACK);
  void **sp = (void **)(n->stack + STACK - 160);
  memset(sp, 0, 160);
  sp[0] = n;                    /* x19 */
  sp[11] = (void *)ctx_boot;    /* x30 */
  n->sp = sp;
}

/* ------------------------------------------------------------------------ */
/* Wires                                                                    */

static int wire_read(int wi, int64_t t)
{
  wire_t *w = &wires[wi];
  if (w->tie) return w->tie > 0;
  if (t >= w->glitch_from && t < w->glitch_to) return 0;
  if (w->low) return 0;
  if (w->pp_high) return 1;
  return t >= w->rel_at + w->rise;
}

static int drives_low(int id)
{
  node_t *n = &nodes[id / NPIN];
  int p = id % NPIN;
  if (!n->powered) return 0;
  if (n->kind == KEYWORD) return !n->out[p];
  return (n->mode[p] == GPIO_CFGLR_OUT_10Mhz_OD || n->mode[p] == GPIO_CFGLR_OUT_50Mhz_PP ||
          n->mode[p] == GPIO_CFGLR_OUT_10Mhz_PP) && !n->out[p];
}

static int drives_high(int id)
{
  node_t *n = &nodes[id / NPIN];
  int p = id % NPIN;
  return n->powered && n->kind == KEYBIT &&
         (n->mode[p] == GPIO_CFGLR_OUT_50Mhz_PP || n->mode[p] == GPIO_CFGLR_OUT_10Mhz_PP) && n->out[p];
}

static void wire_update(int wi, int64_t t)
{
  wire_t *w = &wires[wi];
  int low = 0, hi = 0;
  for (int i = 0; i < w->nm; i++) { low |= drives_low(w->m[i]); hi |= drives_high(w->m[i]); }
  if (w->low && !low) w->rel_at = t;
  if (low && hi) fights++;
  static int64_t tr_from = -2, tr_to;
  if (tr_from == -2) { const char *e = getenv("WTRACE"); tr_from = e ? (int64_t)(atof(e) * PS_MS) : -1; tr_to = tr_from + 2 * PS_MS; }
  if (tr_from >= 0 && t >= tr_from && t < tr_to && wi == pin_wire[0][0] && w->low != low) printf("%12.3f us SCL %s by %d\n", t / 1e6, low ? "low " : "rel ", cur ? cur->idx : -1);
  if (tr_from >= 0 && t >= tr_from && t < tr_to && wi == pin_wire[0][1] && w->low != low) printf("%12.3f us        SDA %s by %d\n", t / 1e6, low ? "low " : "rel ", cur ? cur->idx : -1);
  if ((w->tie > 0 && low) || (w->tie < 0 && hi)) shorts++;
  w->low = low;
  w->pp_high = hi;
}

/* ------------------------------------------------------------------------ */
/* Topology                                                                 */

static int uf[MAXN * NPIN];
static int vv[MAXN][MAXN];   /* the right way round to each other (3V3 meets 3V3) */
static int find(int a) { while (uf[a] != a) a = uf[a] = uf[uf[a]]; return a; }

static int side_facing(const node_t *n, int dx, int dy)
{
  for (int s = 0; s < 4; s++) {
    int x = DX[s], y = DY[s];
    for (int r = 0; r < n->rot; r++) { int t = x; x = -y; y = t; }
    if (x == dx && y == dy) return s;
  }
  return -1;
}

static int keybit_at(int x, int y)
{
  for (int i = 1; i < n_nodes; i++)
    if (nodes[i].placed && nodes[i].x == x && nodes[i].y == y) return i;
  return x == 0 && y == 0 ? 0 : -1;
}

static int pin_of(int node, int side, int role)
{
  if (node == 0) return node * NPIN + (role == R_SCL ? 0 : 1);
  return node * NPIN + (role == R_SCL ? SCL_PIN[side] : SDA_PIN[side]);
}

static void build(void)
{
  int ties[MAXN * NPIN];
  memset(ties, 0, sizeof(ties));
  for (int i = 0; i < MAXN * NPIN; i++) uf[i] = i;
  memset(vv, 0, sizeof(vv));

  for (int a = 1; a < n_nodes; a++) {
    node_t *A = &nodes[a];
    if (!A->placed) continue;
    for (int d = 0; d < 4; d++) {
      int b = keybit_at(A->x + DX[d], A->y + DY[d]);
      if (b < 0 || (b > 0 && b < a)) continue;
      int sa = side_facing(A, DX[d], DY[d]);
      int tb, sb = -1;
      if (b == 0) {
        if (DX[d] == 0 && DY[d] == -1) continue;   /* the Keyword's bottom: USB, no contacts */
        tb = DX[d] == -1 ? 1 : 0;                   /* we're on its right: its R side is E/S-like */
      } else {
        sb = side_facing(&nodes[b], -DX[d], -DY[d]);
        tb = TYPE[sb];
      }
      for (int i = 0; i < 4; i++) {
        int ra = ROLE[TYPE[sa]][i], rb = ROLE[tb][3 - i];
        if ((ra == R_SDA || ra == R_SCL) && ra == rb) {
          uf[find(pin_of(a, sa, ra))] = find(pin_of(b, sb, rb));
        } else if (ra == R_V && rb == R_V) {
          vv[a][b] = vv[b][a] = 1;
        } else if ((ra == R_SDA || ra == R_SCL) && (rb == R_V || rb == R_G)) {
          ties[pin_of(a, sa, ra)] = rb == R_V ? 1 : -1;
        } else if ((rb == R_SDA || rb == R_SCL) && (ra == R_V || ra == R_G)) {
          ties[pin_of(b, sb, rb)] = ra == R_V ? 1 : -1;
        }
      }
    }
  }

  /* power: 3V3 contacts back to the Keyword */
  int pw[MAXN] = { 1 }, changed = 1;
  while (changed) {
    changed = 0;
    for (int a = 0; a < n_nodes; a++) if (pw[a])
      for (int b = 1; b < n_nodes; b++) if (!pw[b] && vv[a][b] && nodes[b].placed) pw[b] = changed = 1;
  }

  /* wires from the union-find */
  int root_wire[MAXN * NPIN];
  for (int i = 0; i < MAXN * NPIN; i++) root_wire[i] = -1;
  n_wires = 0;
  memset(pin_wire, -1, sizeof(pin_wire));
  for (int n = 0; n < n_nodes; n++) {
    int pins[8], np = 0;
    if (n == 0) { pins[np++] = 0; pins[np++] = 1; }
    else for (int s = 0; s < 4; s++) { pins[np++] = SDA_PIN[s]; pins[np++] = SCL_PIN[s]; }
    for (int k = 0; k < np; k++) {
      int id = n * NPIN + pins[k], r = find(id);
      if (root_wire[r] < 0) {
        root_wire[r] = n_wires;
        wire_t *w = &wires[n_wires++];
        memset(w, 0, sizeof(*w));
        w->rel_at = -PS_MS;
        w->glitch_from = w->glitch_to = -1;
      }
      wire_t *w = &wires[root_wire[r]];
      if (w->nm < 24) w->m[w->nm++] = id;
      if (ties[id]) w->tie = ties[id];
      pin_wire[n][pins[k]] = root_wire[r];
    }
  }
  for (int i = 0; i < n_wires; i++) {
    wire_t *w = &wires[i];
    double g = 0;
    for (int k = 0; k < w->nm; k++) g += w->m[k] / NPIN == 0 ? 1 / kw_pullup_k : 1 / 10.0;
    double r_k = 1 / g, c = c_pf * (w->nm + 1);
    w->rise = (int64_t)(1.2 * r_k * 1e3 * c * 1e-12 * 1e12);   /* to VIH ~0.7 VDD */
  }

  /* power changes: boot or kill */
  for (int n = 1; n < n_nodes; n++) {
    node_t *k = &nodes[n];
    int want = pw[n] && k->placed;
    if (want && !k->powered) {
      char path[700];
      snprintf(path, sizeof(path), "%s/n%d_%d.bundle", tmpdir, n, k->boots);
      char cmd[1600];
      snprintf(cmd, sizeof(cmd), "cp '%s' '%s'", mixed[n] && bundle2 ? bundle2 : bundle, path);
      if (system(cmd) != 0) { fprintf(stderr, "copy failed\n"); exit(1); }
      void *dl = dlopen(path, RTLD_NOW | RTLD_LOCAL);
      if (!dl) { fprintf(stderr, "%s\n", dlerror()); exit(1); }
      k->entry = (int (*)(void))dlsym(dl, "keybit_main");
      k->depth = (uint8_t (*)(void))dlsym(dl, "sim_node_depth");
      k->qcount = (uint8_t (*)(void))dlsym(dl, "sim_node_qcount");
      k->diag = (const uint8_t *(*)(void))dlsym(dl, "sim_node_diag");
      k->has_press = (int (*)(uint32_t))dlsym(dl, "sim_node_has_press");
      k->addr = (uint8_t (*)(void))dlsym(dl, "sim_node_addr");
      k->key = (uint8_t (*)(void))dlsym(dl, "sim_node_key");
      memset(k->mode, GPIO_CFGLR_IN_FLOATING, sizeof(k->mode));
      memset(k->out, 1, sizeof(k->out));
      k->t = k->t_boot = sim_t + (int64_t)(1 + 2 * frand()) * PS_MS;
      k->tick0 = tick0_all;
      k->powered = k->runnable = 1;
      k->boots++;
      start_coroutine(k);
    } else if (!want && k->powered) {
      k->powered = k->runnable = 0;
    }
  }
  for (int i = 0; i < n_wires; i++) wire_update(i, sim_t);
}

/* ------------------------------------------------------------------------ */
/* What the code calls                                                      */

sim_systick_t *sim_systick(void)
{
  advance(cur->op_ps);
  cur->st.CNT = cur->tick0 + (uint32_t)(uint64_t)((cur->t - cur->t_boot) / cur->ps_per_tick);
  return &cur->st;
}

sim_gpio_t *sim_gpio(uint8_t pin) { return &cur->gpio[pin >> 4]; }
sim_esig_t *sim_esig(void) { return &cur->esig; }
sim_flash_t *sim_flash(void) { advance(120 * PS_US); return &cur->flash; }
void *sim_config_page(void) { return cur->cfg; }

static long btn_reads[MAXN];
static int read_pin(node_t *n, uint8_t pin)
{
  if (pin == PD6) { btn_reads[n->idx]++; return !n->button; }
  int wi = pin_wire[n->idx][pin];
  return wi < 0 ? 1 : wire_read(wi, n->t);
}

int sim_pin_read(uint8_t pin)
{
  static int64_t rt_from = -2, last;
  if (rt_from == -2) { const char *e = getenv("RTRACE"); rt_from = e ? (int64_t)(atof(e) * PS_MS) : -1; }
  if (rt_from >= 0 && cur->idx == 1 && getenv("RALL") && cur->t >= rt_from && cur->t < rt_from + PS_MS) {
    static int lastpin = -1; static long rep;
    if (pin != lastpin) { printf("%12.3f us kb1 rd %02x (x%ld before)\n", cur->t / 1e6, pin, rep); rep = 0; lastpin = pin; } else rep++;
  }
  if (rt_from >= 0 && cur->idx == 1 && pin == PD4 && cur->t >= rt_from && cur->t < rt_from + PS_MS) {
    if (cur->t - last > 4 * PS_US) printf("%12.3f us kb1 reads SDA after %.1f us gap\n", cur->t / 1e6, (cur->t - last) / 1e6);
    last = cur->t;
  }
  /* passes vary with the branches taken: +-30% */
  advance(pin == PD6 ? cur->pass_ps * (70 + (int64_t)(rnd() % 61)) / 100 : cur->op_ps);
  return read_pin(cur, pin);
}

void sim_pin_write(uint8_t pin, int high)
{
  advance(cur->op_ps);
  cur->out[pin] = (uint8_t)high;
  int wi = pin_wire[cur->idx][pin];
  if (wi >= 0) wire_update(wi, cur->t);
}

void sim_pin_mode(uint8_t pin, int mode)
{
  advance(cur->op_ps);
  cur->mode[pin] = (uint8_t)mode;
  int wi = pin_wire[cur->idx][pin];
  if (wi >= 0) wire_update(wi, cur->t);
}

uint32_t sim_port_read(const volatile uint32_t *in)
{
  advance(cur->spin_ps);
  int port = (int)((const sim_gpio_t *)in - cur->gpio);
  uint32_t v = 0;
  for (int b = 0; b < 8; b++) if (read_pin(cur, (uint8_t)(port << 4 | b))) v |= 1u << b;
  return v;
}

/* Like ch32fun's DelaySysTick: spin until the counter has moved n times,
 * so it comes up short by however far into the current tick we already were. */
void sim_delay_ticks(uint32_t n)
{
  double ticks = (cur->t - cur->t_boot) / cur->ps_per_tick;
  double into = ticks - (int64_t)ticks;
  advance((int64_t)((n - into) * cur->ps_per_tick) + cur->op_ps);
}

int64_t sim_now(void) { return cur->t; }
void sim_sleep(int64_t ps) { advance(ps); }
void sim_kw_drive(int pin, int high)
{
  advance(10 * PS_NS);
  cur->out[pin] = (uint8_t)high;
  wire_update(pin_wire[0][pin], cur->t);
}
int sim_kw_read(int pin)
{
  advance(10 * PS_NS);
  return wire_read(pin_wire[0][pin], cur->t);
}

/* ------------------------------------------------------------------------ */
/* Reports and presses                                                      */

typedef struct { int64_t t; uint32_t uid; int down; } report_t;
static report_t reports[LOG_MAX];
static int n_reports;
void kw_report(uint32_t uid, int down)
{
  if (n_reports < LOG_MAX) reports[n_reports++] = (report_t){ cur->t, uid, down };
}

typedef struct { int node; int64_t down, up; } press_t;
static press_t presses[LOG_MAX];
static int n_presses;

extern int kw_present(uint32_t uid);
extern int kw_where(uint32_t uid, uint32_t *parent, char *pside, char *face);
extern int kw_task_ms;

static uint32_t uid_of(int n) { return nodes[n].esig.UNIID1; }

static void add_keybit(int x, int y, int rot)
{
  node_t *k = &nodes[n_nodes];
  memset(k, 0, sizeof(*k));
  k->kind = KEYBIT;
  k->idx = n_nodes;
  k->placed = 1;
  k->x = x; k->y = y; k->rot = rot;
  double f = 48e6 * (1 + 0.02 * (frand() - 0.5));
  k->ps_per_tick = 8e12 / f;
  k->op_ps = (int64_t)(op_cycles * 1e12 / f);
  k->spin_ps = (int64_t)(spin_cycles * 1e12 / f);
  k->pass_ps = (int64_t)((op_cycles + pass_cycles) * 1e12 / f);
  k->esig.UNIID1 = 0x1560AB00u + (uint32_t)n_nodes * 0x01000011u;
  k->esig.UNIID2 = rnd();
  k->esig.UNIID3 = 0xFFFFFFFFu;
  memset(k->cfg, 0xFF, sizeof(k->cfg));
  n_nodes++;
}

static void reset_world(void)
{
  memset(nodes, 0, sizeof(nodes));
  n_nodes = 1;
  node_t *kw = &nodes[0];
  kw->kind = KEYWORD;
  kw->powered = kw->runnable = 1;
  kw->out[0] = kw->out[1] = 1;
  start_coroutine(kw);
  n_reports = n_presses = 0;
  shorts = fights = 0;
  sim_t = 0;
}

/* ------------------------------------------------------------------------ */
/* Metrics                                                                  */

static int cmp64(const void *a, const void *b)
{
  int64_t x = *(const int64_t *)a, y = *(const int64_t *)b;
  return x < y ? -1 : x > y;
}

typedef struct { int n, lost, extra; double p50, p99, max; } lat_t;

/* Latency of each press (down) per depth bucket. A press is lost if no
 * down report for its Keybit lands between its press and the next one's. */
static void measure(lat_t *by_depth, int max_depth, const int *depth_of)
{
  static int64_t buf[16][LOG_MAX / 8];
  int nb[16] = { 0 };
  for (int d = 0; d <= max_depth; d++) memset(&by_depth[d], 0, sizeof(lat_t));
  for (int i = 0; i < n_presses; i++) {
    press_t *p = &presses[i];
    int64_t until = INT64_MAX;
    for (int j = i + 1; j < n_presses; j++) if (presses[j].node == p->node) { until = presses[j].down; break; }
    int d = depth_of[p->node];
    if (d > max_depth) d = max_depth;
    by_depth[d].n++;
    int64_t got = -1;
    for (int r = 0; r < n_reports; r++)
      if (reports[r].uid == uid_of(p->node) && reports[r].down && reports[r].t >= p->down && reports[r].t < until) {
        got = reports[r].t; break;
      }
    static FILE *dump;
    static int dump_init;
    if (!dump_init) { dump_init = 1; const char *f = getenv("LAT_DUMP"); if (f) dump = fopen(f, "a"); }
    if (dump) fprintf(dump, "%d %.3f\n", d, got < 0 ? -1.0 : (got - p->down) / 1e9);
    if (getenv("SHOW_SLOW") && (got < 0 || got - p->down > atoi(getenv("SHOW_SLOW")) * PS_MS))
      printf("  slow: kb%d (depth %d) pressed at %.1f ms, reported %.1f ms later\n", p->node, d, p->down / 1e9, got < 0 ? -1.0 : (got - p->down) / 1e9);
    if (got < 0) by_depth[d].lost++;
    else if (nb[d] < LOG_MAX / 8) buf[d][nb[d]++] = got - p->down;
  }
  for (int d = 0; d <= max_depth; d++) {
    if (!nb[d]) continue;
    qsort(buf[d], nb[d], sizeof(int64_t), cmp64);
    by_depth[d].p50 = buf[d][nb[d] / 2] / 1e9;
    by_depth[d].p99 = buf[d][(nb[d] * 99) / 100] / 1e9;
    by_depth[d].max = buf[d][nb[d] - 1] / 1e9;
  }
}

/* Every placed, powered Keybit known to the Keyword. */
static int all_present(void)
{
  for (int n = 1; n < n_nodes; n++)
    if (nodes[n].powered && !kw_present(uid_of(n))) return 0;
  return 1;
}

static int64_t wait_present(int64_t limit)
{
  int64_t t0 = sim_t;
  while (sim_t - t0 < limit) {
    run_until(sim_t + 50 * PS_MS);
    if (all_present()) return sim_t - t0;
  }
  return -1;
}

/* Taps on random powered Keybits: hold hold_ms (+ up to 50%), gap gap_ms (+ up to 100%). */
/* Run until t, in 1 ms steps with random short glitches low on random
 * wires when NOISE is set (EMI or a contact chattering). */
static long glitches;
static void run_noisy(int64_t t)
{
  static int track = -2;
  static int64_t track_from;
  if (track == -2) {
    track = getenv("TRACK_NODE") ? atoi(getenv("TRACK_NODE")) : -1;
    track_from = getenv("TRACK_FROM") ? (int64_t)(atof(getenv("TRACK_FROM")) * PS_MS) : 0;
  }
  if (track > 0 && sim_t + 1000 * PS_MS > track_from && sim_t < track_from + 1500 * PS_MS) {
    /* where a press of node `track` sits, every 0.5 ms */
    static int64_t had[MAXN];
    while (sim_t < t) {
      run_until(sim_t + PS_MS / 2 < t ? sim_t + PS_MS / 2 : t);
      if (sim_t < track_from) continue;
      static uint8_t last_addr[MAXN];
      for (int k = 1; k < n_nodes; k++) {
        if (nodes[k].powered && nodes[k].addr() != last_addr[k]) {
          printf("  %.1f ms: kb%d address %02x -> %02x\n", sim_t / 1e9, k, last_addr[k], nodes[k].addr());
          last_addr[k] = nodes[k].addr();
        }
        int h = nodes[k].powered && nodes[k].has_press(uid_of(track));
        if (h && !had[k]) printf("  %.1f ms: kb%d (depth %d, q=%d) has it\n", sim_t / 1e9, k, nodes[k].depth(), nodes[k].qcount());
        if (!h && had[k]) printf("  %.1f ms: kb%d passed it on\n", sim_t / 1e9, k);
        had[k] = h;
      }
    }
    return;
  }
  if (noise_per_s <= 0) { run_until(t); return; }
  while (sim_t < t) {
    int64_t step = t - sim_t < PS_MS ? t - sim_t : PS_MS;
    if (frand() < noise_per_s / 1000) {
      wire_t *w = &wires[rnd() % (unsigned)n_wires];
      w->glitch_from = sim_t + (int64_t)(frand() * step);
      w->glitch_to = w->glitch_from + (int64_t)((0.1 + 1.9 * frand()) * PS_US);
      glitches++;
    }
    run_until(sim_t + step);
  }
}

static int dropouts;   /* times a powered Keybit wasn't in the Keyword's list between taps */
static void type_keys(int count, int hold_ms, int gap_ms)
{
  for (int i = 0; i < count; i++) {
    int n;
    do n = 1 + (int)(rnd() % (unsigned)(n_nodes - 1)); while (!nodes[n].powered);
    int64_t hold = (int64_t)(hold_ms * (1 + 0.5 * frand()) * PS_MS);
    int64_t gap = (int64_t)(gap_ms * (1 + frand()) * PS_MS);
    nodes[n].button = 1;
    presses[n_presses++] = (press_t){ n, sim_t, sim_t + hold };
    run_noisy(sim_t + hold);
    nodes[n].button = 0;
    run_noisy(sim_t + gap);
    for (int k = 1; k < n_nodes; k++)
      if (nodes[k].powered && !kw_present(uid_of(k))) {
        dropouts++;
        if (getenv("SHOW_DROPS")) printf("  kb%d missing at %.0f ms\n", k, sim_t / 1e9);
      }
  }
  run_until(sim_t + 200 * PS_MS);   /* let the last ones arrive */
}

static void print_diag(void)
{
  static const char S[4] = { 'S', 'E', 'W', 'N' };
  for (int n = 1; n < n_nodes; n++) {
    if (!nodes[n].powered) continue;
    const uint8_t *d = nodes[n].diag();
    printf("  kb%-2d (%2d,%2d) r%d depth=%d q=%d", n, nodes[n].x, nodes[n].y, nodes[n].rot, nodes[n].depth(), nodes[n].qcount());
    for (int s = 0; s < 4; s++)
      if (d[s * 8] || d[s * 8 + 5])
        printf("  %c req=%d ok=%d noack=%d abort=%d bad=%d served=%d fail=%d", S[s], d[s * 8], d[s * 8 + 1],
               d[s * 8 + 2], d[s * 8 + 3], d[s * 8 + 4], d[s * 8 + 5], d[s * 8 + 6]);
    printf("\n");
  }
}

/* ------------------------------------------------------------------------ */
/* Scenarios                                                                */

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("  FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

static void report_latency(const char *name, int max_depth)
{
  int depth_of[MAXN];
  for (int n = 1; n < n_nodes; n++) depth_of[n] = nodes[n].powered ? nodes[n].depth() : 0;
  lat_t lat[16];
  measure(lat, max_depth, depth_of);
  int lost = 0;
  for (int d = 1; d <= max_depth; d++) {
    if (!lat[d].n) continue;
    printf("  %s depth %d: n=%d lost=%d  p50=%.1f ms p99=%.1f ms max=%.1f ms\n", name, d, lat[d].n, lat[d].lost,
           lat[d].p50, lat[d].p99, lat[d].max);
    lost += lat[d].lost;
  }
  CHECK(lost == 0, "%s: %d presses never reached the Keyword", name, lost);
  /* A down the Keyword reported with no press of that Keybit in the 500 ms
   * before it: a phantom key (or a UID nobody has). */
  int phantom = 0;
  for (int r = 0; r < n_reports; r++) {
    if (!reports[r].down) continue;
    int real = 0;
    for (int p = 0; p < n_presses && !real; p++)
      real = uid_of(presses[p].node) == reports[r].uid && presses[p].down <= reports[r].t &&
             reports[r].t - presses[p].down < 500 * PS_MS;
    if (!real && getenv("SHOW_PHANTOMS")) {
      printf("  phantom down uid %08x at %.1f ms; its presses:", reports[r].uid, reports[r].t / 1e9);
      for (int p = 0; p < n_presses; p++) if (uid_of(presses[p].node) == reports[r].uid) printf(" %.0f", presses[p].down / 1e9);
      printf("\n");
    }
    phantom += !real;
  }
  CHECK(phantom == 0, "%s: %d key downs with no press behind them", name, phantom);
  CHECK(dropouts == 0, "%s: Keybits dropped out of the Keyword's list %d times", name, dropouts);
}

/* Hops from the Keyword over contacts the right way round, 0 if unpowered. */
static void expected_depths(int *depth)
{
  for (int n = 0; n < n_nodes; n++) depth[n] = 0;
  int q[MAXN], h = 0, t = 0, seen[MAXN] = { 1 };
  q[t++] = 0;
  while (h < t) {
    int a = q[h++];
    for (int b = 1; b < n_nodes; b++)
      if (!seen[b] && vv[a][b] && nodes[b].placed) { seen[b] = 1; depth[b] = depth[a] + 1; q[t++] = b; }
  }
}

static const char COMPASS[4] = { 'N', 'E', 'S', 'W' };

static int node_by_uid(uint32_t uid)
{
  for (int n = 1; n < n_nodes; n++) if (nodes[n].placed && uid_of(n) == uid) return n;
  return -1;
}

/* Depths, and for v11+ Keybits where the Keyword thinks each chained one is. */
static void check_places(void)
{
  int want[MAXN];
  expected_depths(want);
  int placed = 0;
  for (int n = 1; n < n_nodes; n++) {
    if (!want[n]) continue;
    CHECK(nodes[n].powered && nodes[n].depth() == want[n], "kb%d depth %d, want %d", n,
          nodes[n].powered ? nodes[n].depth() : -1, want[n]);
    uint32_t parent;
    char pside, face;
    if (want[n] < 2 || !kw_where(uid_of(n), &parent, &pside, &face)) continue;
    int p = node_by_uid(parent);
    /* any neighbour it really meets the right way round: a Keybit may hang
     * off a longer path for a moment when its nearest parent is busy */
    int ok = p > 0 && vv[p][n];
    if (ok) {
      int dx = nodes[n].x - nodes[p].x, dy = nodes[n].y - nodes[p].y;
      ok = COMPASS[side_facing(&nodes[p], dx, dy)] == pside && COMPASS[side_facing(&nodes[n], -dx, -dy)] == face;
    }
    CHECK(ok, "kb%d reported on kb%d's %c side facing it with %c: wrong", n, p, pside, face);
    placed += ok;
  }
  printf("  depths checked, %d chained Keybits placed right\n", placed);
}

typedef struct { int x, y, rot; } cell_t;

static void settle(const char *name)
{
  int64_t t = wait_present(10000 * PS_MS);
  printf("  %s: all present after %.0f ms\n", name, t / 1e9);
  CHECK(t >= 0, "%s: not every powered Keybit showed up", name);
  run_until(sim_t + 1500 * PS_MS);   /* positions settle after the first heartbeats */
  check_places();
}

static void finish(void)
{
  extern unsigned kw_bus_khz(void);
  printf("  fast bus at %u kHz by the end%s\n", kw_bus_khz(), glitches ? "" : "");
  if (glitches) printf("  %ld glitches injected\n", glitches);
  print_diag();
  printf("  shorts=%ld fights=%ld  kb1 main loop pass %.1f us\n", shorts, fights,
         btn_reads[1] ? (sim_t - nodes[1].t_boot) / 1e6 / btn_reads[1] : 0);
}

static void sc_layout(const char *name, const cell_t *c, int n, int presses_n)
{
  printf("%s (%d Keybits)\n", name, n);
  reset_world();
  for (int i = 0; i < n; i++) add_keybit(c[i].x, c[i].y, c[i].rot);
  build();
  settle(name);
  type_keys(presses_n, 60, 120);
  report_latency("tap", 8);
  CHECK(shorts == 0, "a pin drove against a supply %ld times", shorts);
  finish();
}

/* A straight line out of the Keyword's right side, every Keybit the usual way round. */
static void sc_chain(int len, int presses_n)
{
  cell_t c[MAXN];
  for (int i = 0; i < len; i++) c[i] = (cell_t){ i + 1, 0, 0 };
  sc_layout("chain", c, len, presses_n);
}

/* The same, every other Keybit in the second turn that meets its neighbours
 * the right way: N to the west instead of W, so S to the east. */
static void sc_turns(int len, int presses_n)
{
  cell_t c[MAXN];
  for (int i = 0; i < len; i++) c[i] = (cell_t){ i + 1, 0, i & 1 ? 3 : 0 };
  sc_layout("turns", c, len, presses_n);
}

/* The Keyword in a corner of a 2x2 square. */
static void sc_square(int presses_n)
{
  static const cell_t c[] = { { -1, 0, 0 }, { 0, -1, 0 }, { -1, -1, 0 } };
  sc_layout("square", c, 3, presses_n);
}

/* 20 Keybits around the Keyword, three of them touching it. */
static void sc_block(int presses_n)
{
  cell_t c[MAXN];
  int n = 0;
  for (int y = -2; y <= 0; y++)
    for (int x = -3; x <= 3; x++)
      if (y < 0 || x != 0) c[n++] = (cell_t){ x, y, 0 };
  sc_layout("block", c, n, presses_n);
}

/* A chain of 3 with a 4th turned the wrong way round at the end: it gets no
 * power, kb3 must flag it and leave that side alone, the rest keep working. */
static void sc_reversed(int presses_n)
{
  static const cell_t c[] = { { 1, 0, 0 }, { 2, 0, 0 }, { 3, 0, 0 }, { 4, 0, 2 } };
  printf("reversed\n");
  reset_world();
  for (int i = 0; i < 4; i++) add_keybit(c[i].x, c[i].y, c[i].rot);
  build();
  CHECK(!nodes[4].powered, "the backwards Keybit has power");
  settle("reversed");
  type_keys(presses_n, 60, 120);
  report_latency("tap", 8);
  CHECK(shorts == 0, "a pin drove against a supply %ld times", shorts);
  finish();
}

/* Pull the middle of a chain out and put it back. */
static void sc_joinleave(int presses_n)
{
  printf("join/leave\n");
  reset_world();
  for (int i = 1; i <= 4; i++) add_keybit(i, 0, 0);
  build();
  settle("start");
  type_keys(presses_n / 2, 60, 120);
  nodes[2].placed = 0;
  build();
  CHECK(!nodes[3].powered && !nodes[4].powered, "the Keybits behind the gap kept power");
  run_until(sim_t + 4000 * PS_MS);
  CHECK(kw_present(uid_of(1)) && !kw_present(uid_of(2)) && !kw_present(uid_of(3)), "the Keyword still lists Keybits that left");
  nodes[2].placed = 1;
  build();
  settle("rejoined");
  n_presses = n_reports = 0;
  type_keys(presses_n / 2, 60, 120);
  report_latency("tap", 8);
  finish();
}

/* A Keybit pushed onto the end of a chain bounces: contact on and off every
 * 1-3 ms for 40 ms while someone types on the others. */
static void sc_bounce(int presses_n)
{
  printf("bounce\n");
  reset_world();
  for (int i = 1; i <= 4; i++) add_keybit(i, 0, 0);
  nodes[4].placed = 0;
  build();
  settle("start");
  nodes[1].button = 1;
  presses[n_presses++] = (press_t){ 1, sim_t, sim_t + 30 * PS_MS };
  for (int64_t end = sim_t + 40 * PS_MS; sim_t < end;) {
    nodes[4].placed ^= 1;
    build();
    run_until(sim_t + (1 + rnd() % 3) * PS_MS);
    if (nodes[1].button && sim_t >= presses[0].up) nodes[1].button = 0;
  }
  nodes[1].button = 0;
  nodes[4].placed = 1;
  build();
  settle("after");
  type_keys(presses_n, 60, 120);
  report_latency("tap", 8);
  CHECK(shorts == 0, "a pin drove against a supply %ld times", shorts);
  finish();
}

/* Fast typing: 15-22 ms taps 30-60 ms apart. */
/* Where one press spends its time, hop by hop, on a chain of len. */
static void sc_trace(int len, int presses_n)
{
  printf("trace, chain of %d\n", len);
  reset_world();
  for (int i = 1; i <= len; i++) add_keybit(i, 0, 0);
  build();
  settle("trace");
  for (int p = 0; p < presses_n; p++) {
    int n = len;
    int64_t t0 = sim_t, seen[MAXN] = { 0 }, deb = 0, rep = 0;
    int r0 = n_reports;
    nodes[n].button = 1;
    while (sim_t - t0 < 400 * PS_MS && !rep) {
      run_until(sim_t + 250 * PS_US);
      if (!deb && nodes[n].key()) deb = sim_t - t0;
      for (int k = 1; k <= len; k++) if (!seen[k] && nodes[k].has_press(uid_of(n))) seen[k] = sim_t - t0;
      for (int r = r0; r < n_reports; r++) if (reports[r].uid == uid_of(n) && reports[r].down) rep = reports[r].t - t0;
      if (sim_t - t0 > 60 * PS_MS) nodes[n].button = 0;
    }
    nodes[n].button = 0;
    printf("  press %2d at %.1f ms: debounced %.1f", p, t0 / 1e9, deb / 1e9);
    for (int k = len; k >= 1; k--) printf("  kb%d %.1f", k, seen[k] / 1e9);
    printf("  reported %.1f ms\n", rep / 1e9);
    run_until(sim_t + (100 + rnd() % 100) * PS_MS);
  }
}

static void sc_fast(int presses_n)
{
  printf("fast taps, chain of 5\n");
  reset_world();
  for (int i = 1; i <= 5; i++) add_keybit(i, 0, 0);
  build();
  settle("fast");
  type_keys(presses_n, 15, 30);
  report_latency("fast", 8);
  finish();
}

/* How often the first Keybit answers an address at each bus speed. */
static void sc_speed(void)
{
  extern int kw_probe_left, kw_probe_acks, kw_probe_read;
  kw_probe_read = getenv("PROBE_READ") != NULL;   /* I2CRD 0a 00 4 instead of I2CSCAN */
  extern int64_t kw_probe_sdadel;
  if (getenv("SDADEL_TICKS")) kw_probe_sdadel = atoi(getenv("SDADEL_TICKS")) * 250 * PS_NS;
  extern int64_t kw_probe_half;
  printf("speed (1 direct + 3 chained)\n");
  reset_world();
  for (int i = 1; i <= 4; i++) add_keybit(i, 0, 0);
  build();
  CHECK(wait_present(10000 * PS_MS) >= 0, "not every Keybit showed up");
  static const int khz[] = { 20, 25, 35, 50 };
  for (int i = 0; i < 4; i++) {
    kw_probe_half = PS_MS / khz[i] / 2;
    kw_probe_acks = 0;
    kw_probe_left = 20;
    for (int g = 0; kw_probe_left > 0 && g < 500; g++) run_until(sim_t + 20 * PS_MS);
    printf("  %d kHz: %d/20 acks\n", khz[i], kw_probe_acks);
  }
}

int main(int argc, char **argv)
{
  if (argc < 2) { fprintf(stderr, "usage: %s node.bundle [scenario]\n", argv[0]); return 2; }
  strncpy(bundle, argv[1], sizeof(bundle) - 1);
  snprintf(tmpdir, sizeof(tmpdir), "/tmp/keybit-sim-XXXXXX");
  if (!mkdtemp(tmpdir)) return 1;
  const char *e;
  /* Two fits to the same hardware numbers; a change has to hold up under both.
   * lump: the main loop's own work in one block per pass (default).
   * even: no block, every pin or SysTick access costs 45 cycles. */
  if ((e = getenv("MODEL")) && !strcmp(e, "even")) { op_cycles = 45; pass_cycles = 0; }
  if ((e = getenv("OP_CYCLES"))) op_cycles = atoi(e);
  if ((e = getenv("SPIN_CYCLES"))) spin_cycles = atoi(e);
  if ((e = getenv("PASS_CYCLES"))) pass_cycles = atoi(e);
  if ((e = getenv("KW_HALF_US"))) { extern int64_t kw_t_low, kw_t_high; kw_t_low = kw_t_high = (int64_t)(atof(e) * PS_US); }
  if ((e = getenv("SEED"))) seed = (unsigned)atoi(e);
  if ((e = getenv("TICK0"))) tick0_all = (uint32_t)strtoul(e, NULL, 0);   /* SysTick start, e.g. 2 s before it wraps */
  if ((e = getenv("NOISE"))) noise_per_s = atof(e);
  bundle2 = getenv("BUNDLE2");
  for (e = getenv("MIX"); e && *e; e++) if (*e >= '0' && *e <= '9') { mixed[atoi(e)] = 1; while (e[1] >= '0' && e[1] <= '9') e++; }
  const char *sc = argc > 2 ? argv[2] : "chain";
  int len = argc > 3 ? atoi(argv[3]) : 5;
  int np = argc > 4 ? atoi(argv[4]) : 40;
  static const char *suite[] = { "chain", "turns", "square", "block", "reversed", "joinleave", "bounce", "fast" };
  int all = !strcmp(sc, "all");
  for (int i = 0; i < (int)(sizeof(suite) / sizeof(suite[0])); i++) {
    if (!all && strcmp(sc, suite[i])) continue;
    pid_t pid = all ? fork() : 0;   /* each scenario in its own process: the Keyword keeps state */
    if (pid == 0) {
      if (i == 0) sc_chain(len, np);
      if (i == 1) sc_turns(len, np);
      if (i == 2) sc_square(np);
      if (i == 3) sc_block(np);
      if (i == 4) sc_reversed(np);
      if (i == 5) sc_joinleave(np);
      if (i == 6) sc_bounce(np);
      if (i == 7) sc_fast(np);
      if (all) { fflush(stdout); _exit(fails != 0); }
    } else {
      int st;
      waitpid(pid, &st, 0);
      if (!WIFEXITED(st) || WEXITSTATUS(st)) fails++;
    }
  }
  if (!strcmp(sc, "speed")) sc_speed();
  if (!strcmp(sc, "trace")) sc_trace(len, np);
  if (!strcmp(sc, "cells")) {   /* CELLS="x,y,rot;x,y,rot;..." */
    cell_t c[MAXN];
    int n = 0;
    for (const char *p = getenv("CELLS"); p && *p && n < MAXN - 1;) {
      if (sscanf(p, "%d,%d,%d", &c[n].x, &c[n].y, &c[n].rot) == 3) n++;
      p = strchr(p, ';');
      if (p) p++;
    }
    sc_layout("cells", c, n, np);
  }
  char cmd[600];
  snprintf(cmd, sizeof(cmd), "rm -rf '%s'", tmpdir);
  system(cmd);
  printf(fails ? "FAILED (%d)\n" : "ok\n", fails);
  return fails != 0;
}
