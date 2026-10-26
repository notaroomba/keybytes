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
 *   green  idle */
static uint8_t dock_present, dock_failed, bus_held;
static uint32_t white_until;

#define WHITE_BLINK_MS 1200
#define BUS_HELD_MS 100

static void LED_Update(void)
{
  uint32_t now = HAL_GetTick();
  if (dock_failed) LED_Set(1, 0, 0);
  else if (bus_held) LED_Set(((now / 250) & 1) == 0, 0, 0);
  else if ((int32_t)(white_until - now) > 0) {
    uint8_t on = ((now / 100) & 1) == 0;
    LED_Set(on, on, on);
  }
  else if (dock_present) LED_Set(0, 0, 1);
  else LED_Set(0, 1, 0);
}

/* Keybit programming dock: CN7/CN8/CN9 on the keyword's back are 3V3, SWIO
 * and GND, lined up with a Keybit's TP1/TP2/TP4. Poll for a Keybit there
 * and flash keybit_image onto it once each time one is placed. */
static void Keybit_Dock_Poll(void)
{
  static uint32_t last_poll;
  static uint8_t misses;

  if (HAL_GetTick() - last_poll < KEYBIT_POLL_MS) return;
  last_poll = HAL_GetTick();

  if (ch32_detect(&keybit_swio) != CH32_OK) {
    /* A few misses in a row before calling it gone, so a wobble while
     * seating it doesn't trigger a second flash. */
    if (dock_present && ++misses >= 3) {
      dock_present = 0;
      dock_failed = 0;
    }
    return;
  }
  misses = 0;
  if (dock_present) return;
  dock_present = 1;
  LED_Update();      /* blue while flashing */

  HAL_Delay(50);     /* let it finish powering up off CN7 */
  ch32_status_t r = ch32_halt(&keybit_swio, 1);
  if (r == CH32_OK && ch32_verify(&keybit_swio, CH32_FLASH_BASE, keybit_image, keybit_image_len) == CH32_OK) {
    ch32_reboot(&keybit_swio);  /* already up to date */
    return;
  }
  dock_failed = ch32_program(&keybit_swio, keybit_image, keybit_image_len) != CH32_OK;
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
  LED_Set(0, 1, 0); // greeen

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
  hi2c3.Init.Timing = 0x10B17DB5;
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
