/*
 * Boot option bytes (FLASH_OPTR). BOOTLOCK makes the keyword always boot
 * from flash: nSWBOOT0 = 0 (BOOT0 pin ignored) and nBOOT0 = 1. It changes
 * only those two bits. The inline part has no HAL, so it runs in the host
 * test (test/bootcfg_test.c).
 */
#ifndef BOOTCFG_H
#define BOOTCFG_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* FLASH_OPTR bits, RM0434 (same values as stm32wb55xx.h) */
#define BOOTCFG_RDP        0x000000FFu
#define BOOTCFG_nBOOT1     (1u << 23)
#define BOOTCFG_nSWBOOT0   (1u << 26)
#define BOOTCFG_nBOOT0     (1u << 27)
#define BOOTCFG_RDP_LEVEL0 0xAAu
#define BOOTCFG_MIN_UPTIME 10000u   /* ms, same as BOOT_HEALTHY_MS */

/* OPTR with only nSWBOOT0 cleared and nBOOT0 set. */
static inline uint32_t BOOTCFG_LockValue(uint32_t optr)
{
  return (optr & ~BOOTCFG_nSWBOOT0) | BOOTCFG_nBOOT0;
}

static inline int BOOTCFG_IsLocked(uint32_t optr)
{
  return BOOTCFG_LockValue(optr) == optr;
}

/* NULL when BOOTLOCK may write, else why not. */
static inline const char *BOOTCFG_Check(uint32_t optr, uint32_t uptime_ms, int stack_running)
{
  if (uptime_ms <= BOOTCFG_MIN_UPTIME) return "app has not run 10 s yet";
  if (!stack_running) return "BLE stack is not running";
  if ((optr & BOOTCFG_RDP) != BOOTCFG_RDP_LEVEL0) return "RDP is not level 0";
  return NULL;
}

static inline void BOOTCFG_Format(uint32_t optr, char *out, size_t n)
{
  snprintf(out, n, "nBOOT0=%u nSWBOOT0=%u nBOOT1=%u RDP=0x%02X",
           (unsigned)!!(optr & BOOTCFG_nBOOT0), (unsigned)!!(optr & BOOTCFG_nSWBOOT0),
           (unsigned)!!(optr & BOOTCFG_nBOOT1), (unsigned)(optr & BOOTCFG_RDP));
}

/* "nBOOT0=1 nSWBOOT0=1 nBOOT1=1 RDP=0xAA" from the live option bytes. */
void BOOTCFG_Info(char *out, size_t n);

/* 1 when the option bytes already boot from flash only. */
int BOOTCFG_Locked(void);

/* Writes nSWBOOT0 = 0, nBOOT0 = 1 and schedules the option byte reload
 * (a reset) in 500 ms. NULL on success, else why it didn't write. */
const char *BOOTCFG_Lock(void);

#endif /* BOOTCFG_H */
