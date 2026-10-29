/*
 * Host simulator interface: what the Keybit and Keyword code sees instead
 * of registers. Every call costs the calling node some time on its own
 * clock, and the scheduler keeps all nodes in step to within one call.
 */
#ifndef SIM_H
#define SIM_H

#include <stdint.h>

typedef struct { volatile uint32_t CNT; } sim_systick_t;
typedef struct { volatile uint32_t INDR; } sim_gpio_t;
typedef struct { uint32_t UNIID1, UNIID2, UNIID3; } sim_esig_t;
typedef struct { volatile uint32_t KEYR, MODEKEYR, CTLR, STATR; volatile uintptr_t ADDR; } sim_flash_t;

sim_systick_t *sim_systick(void);
sim_gpio_t *sim_gpio(uint8_t pin);
sim_esig_t *sim_esig(void);
sim_flash_t *sim_flash(void);
void *sim_config_page(void);

int sim_pin_read(uint8_t pin);
void sim_pin_write(uint8_t pin, int high);
void sim_pin_mode(uint8_t pin, int mode);
uint32_t sim_port_read(const volatile uint32_t *in);
void sim_delay_ticks(uint32_t n);
void sim_cycles(uint32_t n);       /* charge n CPU cycles of plain computation */

/* Keyword side */
int64_t sim_now(void);              /* ps */
void sim_sleep(int64_t ps);
void sim_kw_drive(int pin, int high);   /* pin 0 SCL, 1 SDA, open drain */
int sim_kw_read(int pin);
void kw_main(void);
void kw_report(uint32_t uid, int down);  /* the keys task saw this key change */
void sim_i2c_timing(uint32_t timingr);   /* I2C3 TIMINGR: SCLL and SCLH in 0.25 us ticks */

#define PS_NS 1000LL
#define PS_US 1000000LL
#define PS_MS 1000000000LL

#endif
