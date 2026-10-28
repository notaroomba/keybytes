/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "app_entry.h"
#include "usb_device.h"
#include "ch32v003_swio.h"
#include "keybit_image.h"
#include "keybit_bus.h"
#include "proto.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* U14 (XL-1608RGBC) is common anode to +3.3V, so a pin driven LOW lights its colour. */
#define LED_STEP_MS 500
#define GREEN_BLINK_MS 150   /* 3 green blinks, this long on and off, once a Keybit is flashed */
#define KEYBIT_POLL_MS 250
#define KEYBIT_SCAN_MS 250
#define KEYBIT_KEYS_MS 5

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c3;

QSPI_HandleTypeDef hqspi;

PCD_HandleTypeDef hpcd_USB_FS;

/* USER CODE BEGIN PV */
static ch32_swio_t keybit_swio;
uint32_t keybit_dock_pause_until;   /* HAL tick; the dock poll stays off the line until then */
/* Keybits touching the keyword's sides: the list is rebuilt every
 * KEYBIT_SCAN_MS, key_down refreshed every KEYBIT_KEYS_MS. keybit_changed
 * collects bits of entries whose key_down changed; whoever consumes key
 * events clears the bits it has handled. */
keybit_info_t keybits[KEYBIT_MAX];
int keybit_count;
volatile uint32_t keybit_changed;
/* Keybits further out, from events relayed by their neighbours. */
keybit_remote_t keybit_remotes[KEYBIT_REMOTE_MAX];
int keybit_remote_count;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C3_Init(void);
static void MX_USB_PCD_Init(void);
static void MX_QUADSPI_Init(void);
static void MX_RF_Init(void);
/* USER CODE BEGIN PFP */
static void LED_Set(uint8_t r, uint8_t g, uint8_t b);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void LED_Set(uint8_t r, uint8_t g, uint8_t b)
{
  HAL_GPIO_WritePin(STATUS_R_GPIO_Port, STATUS_R_Pin, r ? GPIO_PIN_RESET : GPIO_PIN_SET);
  HAL_GPIO_WritePin(STATUS_G_GPIO_Port, STATUS_G_Pin, g ? GPIO_PIN_RESET : GPIO_PIN_SET);
  HAL_GPIO_WritePin(STATUS_B_GPIO_Port, STATUS_B_Pin, b ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

/* Status LED, highest priority first:
 *   red    Keybit on the dock failed to flash (until it's taken off)
 *   red    blinking: SDA held low, a Keybit is on backwards (bus paused)
 *   white  blinking: a Keybit just connected on a side
 *   blue   Keybit on the dock (flashed or up to date)
 *   white  idle */
/* Dock: EMPTY until something answers, WORKING while flashing (blue), then
 * DONE (green) or FAILED (red, gave up) until that board is taken off. */
enum { DOCK_EMPTY, DOCK_WORKING, DOCK_DONE, DOCK_FAILED };
static uint8_t dock_state, bus_held;
static uint32_t white_until;
static uint32_t green_until;   /* green blink after a Keybit is flashed or found up to date */

#define WHITE_BLINK_MS 1200
#define BUS_HELD_MS 100

static void LED_Update(void)
{
  uint32_t now = HAL_GetTick();
  if (dock_state == DOCK_FAILED) LED_Set(1, 0, 0);
  else if ((int32_t)(green_until - now) > 0) LED_Set(0, ((green_until - now) / GREEN_BLINK_MS) & 1, 0);
  else if (bus_held) LED_Set(((now / 250) & 1) == 0, 0, 0);
  else if ((int32_t)(white_until - now) > 0) {
    uint8_t on = ((now / 100) & 1) == 0;
    LED_Set(on, on, on);
  }
  else if (dock_state == DOCK_DONE) LED_Set(0, 1, 0);
  else if (dock_state == DOCK_WORKING) LED_Set(0, 0, 1);
  else LED_Set(1, 1, 1);
}

/* Keybit programming dock: CN7/CN8/CN9 on the keyword's back are 3V3, SWIO
 * and GND, lined up with a Keybit's TP1/TP2/TP4. Poll for a Keybit there
 * and flash keybit_image onto it. Until one has been flashed (or found up
 * to date) it keeps retrying every poll, even after a failed attempt, and
 * reports every attempt that got any answer over serial as DOCK lines. */
/* Halt whatever is on the dock and read its 96-bit UID, trusting it only
 * when two reads agree. */
static ch32_status_t Keybit_Dock_Uid(uint32_t uid[3])
{
  ch32_status_t r = ch32_halt(&keybit_swio, 0);
  if (r != CH32_OK && ch32_detect(&keybit_swio) == CH32_OK) r = ch32_halt(&keybit_swio, 0);
  if (r != CH32_OK) return r;
  for (int t = 0; t < 3; t++) {
    int ok = 1;
    for (int i = 0; i < 3 && ok; i++) {
      uint32_t again = 0;
      ok = ch32_read_word(&keybit_swio, 0x1FFFF7E8u + 4u * i, &uid[i]) == CH32_OK &&
           ch32_read_word(&keybit_swio, 0x1FFFF7E8u + 4u * i, &again) == CH32_OK && uid[i] == again;
    }
    /* All 1s or all 0s is a line nobody is driving, not a UID. */
    if (ok && !(uid[0] == uid[1] && uid[1] == uid[2] && (uid[0] == 0 || uid[0] == 0xFFFFFFFFu))) return CH32_OK;
  }
  return CH32_ERR_VERIFY;
}

/* One flash attempt on the docked board: done if it verifies or programs,
 * failed once 5 attempts in a row get nowhere. */
static int32_t best_blk;   /* furthest block the current board's attempts got to */
static uint8_t stalls;     /* attempts in a row that got no further than where they started */

/* Boards flashed or found up to date since boot; the image is fixed per
 * build, so one of these coming back doesn't need checking again. */
static uint32_t done_uids[8][3];
static uint8_t done_count, done_next;

static void Keybit_Dock_Done(const uint32_t uid[3])
{
  dock_state = DOCK_DONE;
  stalls = 0;
  green_until = HAL_GetTick() + 6 * GREEN_BLINK_MS;
  for (int i = 0; i < done_count; i++)
    if (memcmp(done_uids[i], uid, 12) == 0) return;
  memcpy(done_uids[done_next], uid, 12);
  done_next = (done_next + 1) % 8;
  if (done_count < 8) done_count++;
}

static void Keybit_Dock_Start(const uint32_t uid[3])
{
  for (int i = 0; i < done_count; i++)
    if (memcmp(done_uids[i], uid, 12) == 0) {
      ch32_reboot(&keybit_swio);  /* it was halted for the UID read */
      Keybit_Dock_Done(uid);
      PROTO_Send("DOCK up to date (seen)");
      return;
    }
  dock_state = DOCK_WORKING;
  best_blk = -1;
  stalls = 0;
  LED_Update();    /* blue while flashing */
}

static void Keybit_Dock_Attempt(const uint32_t uid[3])
{
  static uint8_t blank_polls;
  if (dock_state != DOCK_WORKING) return;

  /* No reset: with the Keybit firmware running, a reset-halt loses the line.
   * If the halt fails anyway, detect again and give it one more go. */
  ch32_status_t h = ch32_halt(&keybit_swio, 0);
  if (h != CH32_OK && ch32_detect(&keybit_swio) == CH32_OK) h = ch32_halt(&keybit_swio, 0);
  /* The first words all reading blank is more likely a board that isn't
   * seated yet than an erased one, so give it about a second before
   * believing it (an attempt that died after erasing block 0 does leave it blank). */
  if (h == CH32_OK) {
    uint32_t w = 0;
    int blank = 1;
    for (int i = 0; i < 4 && blank; i++)
      blank = ch32_read_word(&keybit_swio, CH32_FLASH_BASE + 4u * i, &w) == CH32_OK && w == 0xFFFFFFFFu;
    if (blank && ++blank_polls < 4) return;
  }
  blank_polls = 0;
  ch32_status_t v = h == CH32_OK ? ch32_verify(&keybit_swio, CH32_FLASH_BASE, keybit_image, keybit_image_len) : h;
  /* Only a real mismatch means it needs flashing; a garbled read just gets checked again. */
  for (int i = 0; i < 3 && h == CH32_OK && v != CH32_OK && v != CH32_ERR_VERIFY; i++)
    v = ch32_verify(&keybit_swio, CH32_FLASH_BASE, keybit_image, keybit_image_len);
  /* A mismatch has to show up twice before it's worth a reflash. */
  if (v == CH32_ERR_VERIFY) {
    PROTO_Send("DOCK mismatch at %08lx want %08lx got %08lx", (unsigned long)ch32_bad_addr,
               (unsigned long)ch32_bad_want, (unsigned long)ch32_bad_got);
    v = ch32_verify(&keybit_swio, CH32_FLASH_BASE, keybit_image, keybit_image_len);
  }
  if (v == CH32_OK) {
    ch32_reboot(&keybit_swio);
    Keybit_Dock_Done(uid);
    PROTO_Send("DOCK up to date");
    return;
  }
  ch32_status_t p = ch32_program(&keybit_swio, keybit_image, keybit_image_len);
  PROTO_Send("DOCK flash halt=%d verify=%d program=%d acs=%08lx retries=%lu blk=%ld from=%lu", h, v, p,
             (unsigned long)ch32_last_acs, (unsigned long)ch32_retries, (long)ch32_fail_blk,
             (unsigned long)ch32_resume_from);
  if (p == CH32_OK) {
    Keybit_Dock_Done(uid);
    return;
  }
  /* Progress means this attempt got past the block it resumed from. */
  if (ch32_fail_blk > (int32_t)ch32_resume_from) {
    if (ch32_fail_blk > best_blk) best_blk = ch32_fail_blk;
    stalls = 0;
  } else if (++stalls >= 5) {
    dock_state = DOCK_FAILED;
    stalls = 0;
    PROTO_Send("DOCK failed at block %ld", (long)best_blk);
  }
}

static void Keybit_Dock_Poll(void)
{
  static uint32_t last_poll, uid[3];
  static uint8_t misses, settle;

  if (HAL_GetTick() - last_poll < KEYBIT_POLL_MS) return;
  if ((int32_t)(keybit_dock_pause_until - HAL_GetTick()) > 0) return;
  last_poll = HAL_GetTick();

  if (ch32_detect(&keybit_swio) != CH32_OK) {
    settle = 0;
    /* Only call it gone after about a second of misses: detect still drops one now and then. */
    if (dock_state != DOCK_EMPTY && ++misses >= 4) {
      dock_state = DOCK_EMPTY;
      misses = 0;
      PROTO_Send("DOCK gone");
    }
    return;
  }

  if (dock_state == DOCK_EMPTY) {
    /* Wait for three answers in a row (~500 ms) so a board still being seated isn't read half-on. */
    if (++settle < 3) return;
    if (Keybit_Dock_Uid(uid) != CH32_OK) return;
    misses = 0;
    Keybit_Dock_Start(uid);
  } else if (misses) {
    /* It answered again after a gap: the same board carries on where it was,
     * a different one starts over. */
    uint32_t now_uid[3];
    misses = 0;
    if (Keybit_Dock_Uid(now_uid) != CH32_OK) return;
    if (memcmp(now_uid, uid, sizeof(uid)) != 0) {
      memcpy(uid, now_uid, sizeof(uid));
      Keybit_Dock_Start(uid);
    } else if (dock_state != DOCK_WORKING) {
      ch32_reboot(&keybit_swio);  /* back to running its firmware */
    }
  }
  Keybit_Dock_Attempt(uid);
}

/* DOCK? over serial: the raw answers from whatever is on the dock, to debug detection. */
int Keybit_Dock_Probe(uint32_t *cfg, uint32_t *status)
{
  *cfg = *status = 0;
  int r = ch32_detect(&keybit_swio);
  ch32_read_reg(&keybit_swio, CH32_DMCFGR, cfg);
  ch32_read_reg(&keybit_swio, CH32_DMSTATUS, status);
  return r;
}

/* DOCKSCAN: after a successful probe, read DMCFGR n more times and count
 * how many come back exactly as the first good read, to rank timings by
 * how reliably they read rather than by one lucky answer. */
/* n reads alternating DMCFGR and DMHARTINFO against the values a V003
 * gives; returns how many came back right. */
int Keybit_Dock_Reads(int n)
{
  int good = 0;
  for (int i = 0; i < n; i++) {
    uint32_t v = 0;
    uint8_t reg = (i & 1) ? CH32_DMHARTINFO : CH32_DMCFGR;
    uint32_t want = (i & 1) ? 0x002120F4u : 0x5AA50401u;
    if (ch32_read_reg(&keybit_swio, reg, &v) == CH32_OK && v == want) good++;
  }
  return good;
}

/* Halts the core, then n write/read-back round trips through DMDATA0 (four
 * fixed patterns, then an LCG), then lets it run again. Returns how many DATA0 ones matched, or -1 if the halt
 * failed (its result in *hr). The first DATA0 mismatch goes in *bw / *br,
 * and *bad says whether there was one. */
int Keybit_Dock_Score(int n, uint32_t *bw, uint32_t *br, int *bad, int *hr)
{
  static const uint32_t fixed[4] = { 0x00000000u, 0xFFFFFFFFu, 0xAAAAAAAAu, 0x55555555u };
  uint32_t x = 0x12345678u;
  int good = 0;
  *bad = 0;
  *hr = ch32_halt(&keybit_swio, 0);
  if (*hr != CH32_OK) return -1;
  ch32_write_reg(&keybit_swio, CH32_DMABSTRACTAUTO, 0);
  for (int i = 0; i < n; i++) {
    uint32_t w = i < 4 ? fixed[i] : (x = x * 1664525u + 1013904223u), r = 0;
    ch32_write_reg(&keybit_swio, CH32_DMDATA0, w);
    if (ch32_read_reg(&keybit_swio, CH32_DMDATA0, &r) == CH32_OK && r == w) good++;
    else if (!*bad) { *bad = 1; *bw = w; *br = r; }
  }
  ch32_reboot(&keybit_swio);
  return good;
}

/* Detect whatever is on the dock, optionally request a halt (DMCONTROL
 * written twice) and wait delay_ms, then dump the debug module registers
 * (HARTINFO four times) into v[0..7]; 0xEEEEEEEE for a failed read.
 * x=1 or 2 runs ch32_halt (2 with reset) instead of the halt writes and
 * puts its result in *hr. */
int Keybit_Dock_Regs(uint32_t *v, uint32_t delay_ms, int halt, int x, int *hr)
{
  static const uint8_t regs[8] = { CH32_DMSTATUS, CH32_DMHARTINFO, CH32_DMHARTINFO, CH32_DMHARTINFO,
                                   CH32_DMHARTINFO, CH32_DMABSTRACTCS, CH32_DMCFGR, CH32_DMCONTROL };
  int r = ch32_detect(&keybit_swio);
  *hr = 0;
  if (r == CH32_OK && x) *hr = ch32_halt(&keybit_swio, x == 2);
  else if (r == CH32_OK && halt) {
    ch32_write_reg(&keybit_swio, CH32_DMCONTROL, 0x80000001u);
    ch32_write_reg(&keybit_swio, CH32_DMCONTROL, 0x80000001u);
  }
  if (delay_ms) HAL_Delay(delay_ms);
  for (int i = 0; i < 8; i++)
    if (ch32_read_reg(&keybit_swio, regs[i], &v[i]) != CH32_OK) v[i] = 0xEEEEEEEEu;
  return r;
}

/* DOCKID: walk detect, halt, chip ID, a few flash and FLASH register reads
 * and the unlock, printing each step's result into out. Stops at the first
 * failure, adding the last DMABSTRACTCS. */
void Keybit_Dock_Id(char *out, size_t len, int reset)
{
  static const struct { const char *name; uint32_t addr; } words[] = {
    { "w0", 0x08000000u }, { "w4", 0x08000004u }, { "ctlr", 0x40022010u }, { "statr", 0x4002200Cu },
  };
  size_t n = 0;
  uint32_t v = 0;
#define STEP(fmt, ...) n += (size_t)snprintf(out + n, n < len ? len - n : 0, fmt, __VA_ARGS__)
#define FAIL(r) do { if (r != CH32_OK) { STEP(" acs=%08lx", (unsigned long)ch32_last_acs); return; } } while (0)
  out[0] = 0;
  int r = ch32_detect(&keybit_swio);
  STEP("detect=%d", r);
  FAIL(r);
  r = ch32_halt(&keybit_swio, reset);
  STEP(" halt=%d", r);
  FAIL(r);
  r = ch32_read_chip_id(&keybit_swio, &v);
  STEP(" id=%d:%08lx", r, (unsigned long)v);
  FAIL(r);
  for (unsigned i = 0; i < sizeof(words) / sizeof(words[0]); i++) {
    v = 0;
    r = ch32_read_word(&keybit_swio, words[i].addr, &v);
    STEP(" %s=%d:%08lx", words[i].name, r, (unsigned long)v);
    FAIL(r);
  }
  r = ch32_unlock_flash(&keybit_swio);
  STEP(" unlock=%d", r);
  FAIL(r);
  v = 0;
  r = ch32_read_word(&keybit_swio, 0x40022010u, &v);
  STEP(" ctlr2=%d:%08lx", r, (unsigned long)v);
  FAIL(r);
#undef STEP
#undef FAIL
}

/* Hand addresses to new Keybits, then refresh the list of who is there. */
static void Keybit_Bus_Poll(void)
{
  static uint32_t last_scan, last_keys, held_since;

  /* A backwards Keybit holds SDA low; tell the host once it has lasted
   * BUS_HELD_MS and once it clears, and leave the bus alone meanwhile. */
  uint32_t now = HAL_GetTick();
  if (!keybit_bus_held()) {
    held_since = now;
    if (bus_held) {
      bus_held = 0;
      PROTO_Send("OK BUS clear");
    }
  } else if (!bus_held && now - held_since >= BUS_HELD_MS) {
    bus_held = 1;
    PROTO_Send("ERR bus SDA held low, a Keybit on the Keyword may be turned the wrong way");
  }
  if (keybit_bus_unstick(&hi2c3)) PROTO_Send("OK BUS recovered");
  if (keybit_bus_jammed()) return;
  if (HAL_GetTick() - last_keys >= KEYBIT_KEYS_MS) {
    last_keys = HAL_GetTick();
    keybit_changed |= keybit_bus_poll_keys(&hi2c3, keybits, keybit_count);
    keybit_bus_poll_relay(&hi2c3, keybits, keybit_count, keybit_remotes, &keybit_remote_count);
  }
  if (HAL_GetTick() - last_scan < KEYBIT_SCAN_MS) return;
  last_scan = HAL_GetTick();

  int fresh = keybit_bus_assign(&hi2c3);
  int before = keybit_count;
  keybit_count = keybit_bus_scan(&hi2c3, keybits, KEYBIT_MAX);
  if (fresh > 0 || keybit_count > before) white_until = HAL_GetTick() + WHITE_BLINK_MS;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  MX_APPE_Config();   /* STM32_WPAN: IPCC/backup reset and HSE trim, before the clocks start */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C3_Init();
  MX_USB_PCD_Init();
  MX_QUADSPI_Init();
  MX_RF_Init();
  /* USER CODE BEGIN 2 */

  ch32_init(&keybit_swio, BB_SWIO_GPIO_Port, BB_SWIO_Pin);
  LED_Set(1, 1, 1); // white

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    MX_APPE_Process();  /* STM32_WPAN sequencer: BLE events, keymap, NUS */
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    Keybit_Dock_Poll();
    Keybit_Bus_Poll();
    LED_Update();

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_MEDIUMHIGH);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSE
                              |RCC_OSCILLATORTYPE_LSE|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 32;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the SYSCLKSource, HCLK, PCLK1 and PCLK2 clocks dividers
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK4|RCC_CLOCKTYPE_HCLK2
                              |RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.AHBCLK2Divider = RCC_SYSCLK_DIV2;
  RCC_ClkInitStruct.AHBCLK4Divider = RCC_SYSCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable MSI Auto calibration
  */
  HAL_RCCEx_EnableMSIPLLMode();
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SMPS|RCC_PERIPHCLK_RFWAKEUP;
  PeriphClkInitStruct.RFWakeUpClockSelection = RCC_RFWKPCLKSOURCE_HSE_DIV1024;
  PeriphClkInitStruct.SmpsClockSelection = RCC_SMPSCLKSOURCE_HSI;
  PeriphClkInitStruct.SmpsDivSelection = RCC_SMPSCLKDIV_RANGE1;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN Smps */

  /* USER CODE END Smps */
}

/**
  * @brief I2C3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C3_Init(void)
{

  /* USER CODE BEGIN I2C3_Init 0 */

  /* USER CODE END I2C3_Init 0 */

  /* USER CODE BEGIN I2C3_Init 1 */

  /* USER CODE END I2C3_Init 1 */
  hi2c3.Instance = I2C3;
  hi2c3.Init.Timing = 0xF0426363;  /* ~20 kHz: Keybits run their I2C target in software and miss bits at 100 kHz */
  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c3, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c3, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C3_Init 2 */

  /* USER CODE END I2C3_Init 2 */

}

/**
  * @brief QUADSPI Initialization Function
  * @param None
  * @retval None
  */
static void MX_QUADSPI_Init(void)
{

  /* USER CODE BEGIN QUADSPI_Init 0 */

  /* USER CODE END QUADSPI_Init 0 */

  /* USER CODE BEGIN QUADSPI_Init 1 */

  /* USER CODE END QUADSPI_Init 1 */
  /* QUADSPI parameter configuration*/
  hqspi.Instance = QUADSPI;
  hqspi.Init.ClockPrescaler = 255;
  hqspi.Init.FifoThreshold = 1;
  hqspi.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_NONE;
  hqspi.Init.FlashSize = 1;
  hqspi.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_1_CYCLE;
  hqspi.Init.ClockMode = QSPI_CLOCK_MODE_0;
  if (HAL_QSPI_Init(&hqspi) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN QUADSPI_Init 2 */

  /* USER CODE END QUADSPI_Init 2 */

}

/**
  * @brief RF Initialization Function
  * @param None
  * @retval None
  */
static void MX_RF_Init(void)
{

  /* USER CODE BEGIN RF_Init 0 */

  /* USER CODE END RF_Init 0 */

  /* USER CODE BEGIN RF_Init 1 */

  /* USER CODE END RF_Init 1 */
  /* USER CODE BEGIN RF_Init 2 */
  MX_APPE_Init();     /* STM32_WPAN: RTC timer server, transport layer, boots CPU2 */
  MX_USB_Device_Init(); /* USB CDC serial for the config protocol */
  /* USER CODE END RF_Init 2 */

}

/**
  * @brief USB Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB_PCD_Init(void)
{

  /* USER CODE BEGIN USB_Init 0 */

  /* USER CODE END USB_Init 0 */

  /* USER CODE BEGIN USB_Init 1 */

  /* USER CODE END USB_Init 1 */
  hpcd_USB_FS.Instance = USB;
  hpcd_USB_FS.Init.dev_endpoints = 8;
  hpcd_USB_FS.Init.speed = PCD_SPEED_FULL;
  hpcd_USB_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd_USB_FS.Init.Sof_enable = DISABLE;
  hpcd_USB_FS.Init.low_power_enable = DISABLE;
  hpcd_USB_FS.Init.lpm_enable = DISABLE;
  hpcd_USB_FS.Init.battery_charging_enable = DISABLE;
  if (HAL_PCD_Init(&hpcd_USB_FS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_Init 2 */

  /* USER CODE END USB_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(STATUS_B_GPIO_Port, STATUS_B_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, STATUS_G_Pin|STATUS_R_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BB_SWIO_GPIO_Port, BB_SWIO_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : STATUS_B_Pin */
  GPIO_InitStruct.Pin = STATUS_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(STATUS_B_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : STATUS_G_Pin STATUS_R_Pin */
  GPIO_InitStruct.Pin = STATUS_G_Pin|STATUS_R_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : BB_SWIO_Pin */
  GPIO_InitStruct.Pin = BB_SWIO_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BB_SWIO_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  /* Fast red blink so a failed init (e.g. a crystal not starting) is visible. */
  __disable_irq();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Pin = STATUS_B_Pin;
  HAL_GPIO_Init(STATUS_B_GPIO_Port, &GPIO_InitStruct);
  GPIO_InitStruct.Pin = STATUS_G_Pin | STATUS_R_Pin;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
  LED_Set(0, 0, 0);
  while (1)
  {
    HAL_GPIO_TogglePin(STATUS_R_GPIO_Port, STATUS_R_Pin);
    for (volatile uint32_t i = 0; i < 200000; i++)
    {
    }
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
