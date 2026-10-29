/* Stand-in for ch32fun.h so the Keybit sources build on the host. */
#ifndef CH32FUN_SIM_H
#define CH32FUN_SIM_H

#include <stdint.h>
#include <stddef.h>
#include "sim.h"

#define PA0 0x00
#define PA1 0x01
#define PA2 0x02
#define PC0 0x20
#define PC1 0x21
#define PC2 0x22
#define PC3 0x23
#define PC4 0x24
#define PD0 0x30
#define PD1 0x31
#define PD2 0x32
#define PD4 0x34
#define PD5 0x35
#define PD6 0x36

enum {
  GPIO_CFGLR_IN_FLOATING = 0x4,
  GPIO_CFGLR_IN_PUPD = 0x8,
  GPIO_CFGLR_OUT_10Mhz_PP = 0x1,
  GPIO_CFGLR_OUT_50Mhz_PP = 0x3,
  GPIO_CFGLR_OUT_10Mhz_OD = 0x5,
};
#define FUN_HIGH 1
#define FUN_LOW  0

#define funDigitalRead(p)      sim_pin_read(p)
#define funDigitalWrite(p, v)  sim_pin_write((p), (v))
#define funPinMode(p, m)       sim_pin_mode((p), (m))
#define funGpioInitAll()       ((void)0)
#define SystemInit()           ((void)0)

/* SysTick at HCLK/8 = 6 MHz, as on the real part. */
#define DELAY_US_TIME 6
#define DELAY_MS_TIME 6000
#define DelaySysTick(n) sim_delay_ticks(n)
#define Delay_Us(n)     sim_delay_ticks((uint32_t)(n) * DELAY_US_TIME)

#define SysTick (sim_systick())
#define GpioOf(p) (sim_gpio(p))
#define READ_PORT(in) sim_port_read(in)
#define ESIG (sim_esig())
#define FLASH (sim_flash())
#define CONFIG_PAGE ((uintptr_t)sim_config_page())

#define FLASH_KEY1 0x45670123u
#define FLASH_KEY2 0xCDEF89ABu
#define CR_PAGE_ER 0x00020000u
#define CR_PAGE_PG 0x00010000u
#define CR_BUF_RST 0x00080000u
#define CR_STRT_Set 0x00000040u
#define CR_LOCK_Set 0x00000080u
#define FLASH_CTLR_BUF_LOAD 0x00040000u
#define FLASH_STATR_BSY 0x01u

#endif
