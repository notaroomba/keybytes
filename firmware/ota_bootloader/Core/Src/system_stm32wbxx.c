/**
  ******************************************************************************
 * @file    system_stm32wbxx.c
 * @author  MCD Application Team
 * @brief   CMSIS Cortex Device Peripheral Access Layer System Source File
 *
 *   This file provides two functions and one global variable to be called from
 *   user application:
 *      - SystemInit(): This function is called at startup just after reset and
 *                      before branch to main program. This call is made inside
 *                      the "startup_stm32wbxx.s" file.
 *
 *      - SystemCoreClock variable: Contains the core clock (HCLK), it can be used
 *                                  by the user application to setup the SysTick
 *                                  timer or configure other parameters.
 *
 *      - SystemCoreClockUpdate(): Updates the variable SystemCoreClock and must
 *                                 be called whenever the core clock is changed
 *                                 during program execution.
 *
 *   After each device reset the MSI (4 MHz) is used as system clock source.
 *   Then SystemInit() function is called, in "startup_stm32wbxx.s" file, to
 *   configure the system clock before to branch to main program.
 *
 *   This file configures the system clock as follows:
 *=============================================================================
 *-----------------------------------------------------------------------------
 *        System Clock source                    | MSI
 *-----------------------------------------------------------------------------
 *        SYSCLK(Hz)                             | 4000000
 *-----------------------------------------------------------------------------
 *        HCLK(Hz)                               | 4000000
 *-----------------------------------------------------------------------------
 *        AHB Prescaler                          | 1
 *-----------------------------------------------------------------------------
 *        APB1 Prescaler                         | 1
 *-----------------------------------------------------------------------------
 *        APB2 Prescaler                         | 1
 *-----------------------------------------------------------------------------
 *        PLL_M                                  | 1
 *-----------------------------------------------------------------------------
 *        PLL_N                                  | 8
 *-----------------------------------------------------------------------------
 *        PLL_P                                  | 7
 *-----------------------------------------------------------------------------
 *        PLL_Q                                  | 2
 *-----------------------------------------------------------------------------
 *        PLL_R                                  | 2
 *-----------------------------------------------------------------------------
 *        PLLSAI1_P                              | NA
 *-----------------------------------------------------------------------------
 *        PLLSAI1_Q                              | NA
 *-----------------------------------------------------------------------------
 *        PLLSAI1_R                              | NA
 *-----------------------------------------------------------------------------
 *        Require 48MHz for USB OTG FS,          | Disabled
 *        SDIO and RNG clock                     |
 *-----------------------------------------------------------------------------
 *=============================================================================
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2019-2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */


/** @addtogroup CMSIS
 * @{
 */

/** @addtogroup stm32WBxx_system
 * @{
 */

/** @addtogroup stm32WBxx_System_Private_Includes
 * @{
 */

#include "app_common.h"
#include "otp.h"

#ifdef OTA_SBSFU
#include "ota_sbsfu.h"
#endif /* OTA_SBSFU */

#if !defined  (HSE_VALUE)
#define HSE_VALUE    ((uint32_t)32000000) /*!< Value of the External oscillator in Hz */
#endif /* HSE_VALUE */

#if !defined  (MSI_VALUE)
#define MSI_VALUE    ((uint32_t)4000000) /*!< Value of the Internal oscillator in Hz*/
#endif /* MSI_VALUE */

#if !defined  (HSI_VALUE)
#define HSI_VALUE    ((uint32_t)16000000) /*!< Value of the Internal oscillator in Hz*/
#endif /* HSI_VALUE */

#if !defined  (LSI_VALUE) 
#define LSI_VALUE  ((uint32_t)32000)       /*!< Value of LSI in Hz*/
#endif /* LSI_VALUE */ 

#if !defined  (LSE_VALUE)
#define LSE_VALUE    ((uint32_t)32768)    /*!< Value of LSE in Hz*/
#endif /* LSE_VALUE */

/**
 * @}
 */

/** @addtogroup STM32WBxx_System_Private_TypesDefinitions
 * @{
 */

typedef void (*fct_t)(void);
/**
 * @}
 */

/** @addtogroup STM32WBxx_System_Private_Defines
 * @{
 */

/*!< Uncomment the following line if you need to relocate your vector Table in
     Internal SRAM. */
/* #define VECT_TAB_SRAM */
/*!< Vector Table base offset field. This value must be a multiple of 0x200. */
/* #define VECT_TAB_OFFSET  0x0U*/
#ifdef OTA_SBSFU
#if defined (__CC_ARM) || defined (__ARMCC_VERSION)
extern void * __Vectors;
#define VECT_TAB_OFFSET ((uint32_t) & __Vectors)
#elif defined(__ICCARM__)
extern uint32_t __vector_table;
#define VECT_TAB_OFFSET ((uint32_t)& __vector_table)
#elif defined(__GNUC__)
extern void * g_pfnVectors;
#define VECT_TAB_OFFSET ((uint32_t)& g_pfnVectors)
#endif
#endif
/**
 * @}
 */

/** @addtogroup STM32WBxx_System_Private_Macros
 * @{
 */

/**
 * @}
 */

/** @addtogroup STM32WBxx_System_Private_Variables
 * @{
 */
/* The SystemCoreClock variable is updated in three ways:
      1) by calling CMSIS function SystemCoreClockUpdate()
      2) by calling HAL API function HAL_RCC_GetHCLKFreq()
      3) each time HAL_RCC_ClockConfig() is called to configure the system clock frequency
         Note: If you use this function to configure the system clock; then there
               is no need to call the 2 first functions listed above, since SystemCoreClock
               variable is updated automatically.
 */
uint32_t SystemCoreClock  = 4000000; /*CPU1: M4 on MSI clock after startup (4MHz)*/

const uint32_t AHBPrescTable[16] =   {1, 3, 5, 1, 1, 6, 10, 32, 2, 4, 8, 16, 64, 128, 256, 512};
/* index=[0,...15]*/
const uint32_t APBPrescTable[8] =  {0, 0, 0, 0, 1, 2, 3, 4};

const uint32_t MSIRangeTable[16UL] = {100000UL, 200000UL, 400000UL, 800000UL, 1000000UL, 2000000UL, \
    4000000UL, 8000000UL, 16000000UL, 24000000UL, 32000000UL, 48000000UL, 0UL, 0UL, 0UL, 0UL}; /* 0UL values are incorrect cases */

#if defined(STM32WB55xx) || defined(STM32WB5Mxx) || defined(STM32WB35xx) || defined (STM32WB15xx)
  const uint32_t SmpsPrescalerTable[4UL][6UL]={{1UL,3UL,2UL,2UL,1UL,2UL}, \
                                        {2UL,6UL,4UL,3UL,2UL,4UL}, \
                                        {4UL,12UL,8UL,6UL,4UL,8UL}, \
                                        {4UL,12UL,8UL,6UL,4UL,8UL}};
#endif

/**
 * @}
 */

 /** @addtogroup STM32WBxx_System_Private_FunctionPrototypes
  * @{
  */

  /**
   * @}
   */

   /** @addtogroup STM32WBxx_System_Private_Functions
    * @{
    */
#ifndef OTA_SBSFU
/****************************/
static void JumpFwApp( void );
static void BootModeCheck( void );
static void JumpSelectionOnPowerUp( void );

/**
 * Return 0 if FW App not valid
 * Return 1 if Fw App valid
 */
static uint8_t  CheckFwAppValidity( void );


static uint8_t CheckFwAppValidity( void )
{
  uint8_t status;
  uint32_t magic_keyword_address;
  uint32_t last_user_flash_address;

  magic_keyword_address = *(uint32_t*)(FLASH_BASE + (CFG_APP_START_SECTOR_INDEX * 0x1000 + 0x140));
  last_user_flash_address = (((READ_BIT(FLASH->SFR, FLASH_SFR_SFSA) >> FLASH_SFR_SFSA_Pos) << 12) + FLASH_BASE) - 4;
  if( (magic_keyword_address < FLASH_BASE) || (magic_keyword_address > last_user_flash_address) )
  {
    /**
     * The address is not valid
     */
    status = 0;
  }
  else
  {
    if( (*(uint32_t*)magic_keyword_address) != 0x94448A29  )
    {
      /**
       * A firmware update procedure did not complete
       */
      status = 0;
    }
    else
    {
      /**
       * The firmware application is available
       */
      status = 1;
    }
  }

  return status;
}

/**
 * Jump to existing FW App in flash
 * It never returns
 */
static void JumpFwApp( void )
{
  fct_t app_reset_handler;

  SCB->VTOR = FLASH_BASE + (CFG_APP_START_SECTOR_INDEX * 0x1000);
  __set_MSP(*(uint32_t*)(FLASH_BASE + (CFG_APP_START_SECTOR_INDEX * 0x1000)));
  app_reset_handler = (fct_t)(*(uint32_t*)(FLASH_BASE + (CFG_APP_START_SECTOR_INDEX * 0x1000) + 4));
  app_reset_handler();

  /**
   * app_reset_handler() never returns.
   * However, if for any reason a PUSH instruction is added at the entry of  JumpFwApp(),
   * we need to make sure the POP instruction is not there before app_reset_handler() is called
   * The way to ensure this is to add a dummy code after app_reset_handler() is called
   * This prevents app_reset_handler() to be the last code in the function.
   */
  __WFI();


  return;
}

/**
 * Check the Boot mode request
 * Depending on the result, the CPU may either jump to an existing application in the user flash
 * or keep on running the code to start the OTA loader
 */
/**
 * Keyword: start ST's ROM USB DFU bootloader in system memory, without
 * option bytes. Only called right out of a software reset, before any
 * clock, peripheral or CPU2 has been touched, so there is nothing to
 * de-initialise first. Interrupts are still off (SystemInit runs before
 * the app enables any).
 */
static void JumpSystemDfu( void )
{
  fct_t rom_reset_handler;

  LL_SYSCFG_SetRemapMemory(LL_SYSCFG_REMAP_SYSTEMFLASH);
  SCB->VTOR = SYSTEM_MEMORY_BASE;
  __set_MSP(*(uint32_t*)SYSTEM_MEMORY_BASE);
  rom_reset_handler = (fct_t)(*(uint32_t*)(SYSTEM_MEMORY_BASE + 4));
  rom_reset_handler();

  __WFI();   /* never reached, see JumpFwApp() */
}

static void BootModeCheck( void )
{
  /**
   * Keyword: the app (DFU command) or this loader (no BLE stack) asked for
   * the ROM USB DFU bootloader. The magic is cleared first, so a reset out
   * of DFU boots normally again.
   */
  if(LL_RCC_IsActiveFlag_SFTRST( ) && (CFG_SYSDFU_REQ_WORD == CFG_SYSDFU_REQ_MAGIC))
  {
    CFG_SYSDFU_REQ_WORD = 0;
    CFG_OTA_REBOOT_VAL_MSG = CFG_REBOOT_ON_FW_APP;
    JumpSystemDfu();
  }

  if(LL_RCC_IsActiveFlag_SFTRST( ) || LL_RCC_IsActiveFlag_OBLRST( ))
  {
    /**
     * The SRAM1 content is kept on Software Reset.
     * In the Ble_Ota application, the first address of the SRAM1 indicates which kind of action has been requested
     */

    /**
     * Check Boot Mode from SRAM1
     */
    if((CFG_OTA_REBOOT_VAL_MSG == CFG_REBOOT_ON_FW_APP) && (CheckFwAppValidity( ) != 0))
    {
      /**
       * The user has requested to start on the firmware application and it has been checked
       * a valid application is ready
       * Jump now on the application
       */
      JumpFwApp();
    }
    else if((CFG_OTA_REBOOT_VAL_MSG == CFG_REBOOT_ON_FW_APP) && (CheckFwAppValidity( ) == 0))
    {
      /**
       * The user has requested to start on the firmware application but there is no valid application
       * Erase all sectors specified by byte1 and byte1 in SRAM1 to download a new App.
       */
      CFG_OTA_REBOOT_VAL_MSG = CFG_REBOOT_ON_BLE_OTA_APP;     /* Request to reboot on BLE_Ota application */
      CFG_OTA_START_SECTOR_IDX_VAL_MSG = CFG_APP_START_SECTOR_INDEX;
      CFG_OTA_NBR_OF_SECTOR_VAL_MSG = 0xFF;
    }
    else if(CFG_OTA_REBOOT_VAL_MSG == CFG_REBOOT_ON_BLE_OTA_APP)
    {
      /**
       * It has been requested to reboot on BLE_Ota application to download data
       */
    }
    else if(CFG_OTA_REBOOT_VAL_MSG == CFG_REBOOT_ON_CPU2_UPGRADE)
    {
      /**
       * It has been requested to reboot on BLE_Ota application to keep running the firmware upgrade process
       *
       */
    }
    else
    {
      /**
       * There should be no use case to be there because the device already starts from power up
       * and the SRAM1 is then filled with the value define by the user
       * However, it could be that a reset occurs just after a power up and in that case, the Ble_Ota
       * will be running but the sectors to download a new App may not be erased
       */
      JumpSelectionOnPowerUp( );
    }
  }
  else
  {
    /**
     * On Power up, the content of SRAM1 is random
     * The only thing that could be done is to jump on either the firmware application
     * or the Ble_Ota application
     */
    JumpSelectionOnPowerUp( );
  }

  /**
   * Return to the startup file and run the Ble_Ota application
   */
  return;
}

static void JumpSelectionOnPowerUp( void )
{
  /**
   * Check if there is a FW App
   */
  if(CheckFwAppValidity( ) != 0)
  {
    /**
     * The SRAM1 is random
     * Initialize SRAM1 to indicate we requested to reboot of firmware application
     */
    CFG_OTA_REBOOT_VAL_MSG = CFG_REBOOT_ON_FW_APP;

    /**
     * A valid application is available
     * Jump now on the application
     */
    JumpFwApp();
  }
  else
  {
    /**
     * The SRAM1 is random
     * Initialize SRAM1 to indicate we requested to reboot of BLE_Ota application
     */
    CFG_OTA_REBOOT_VAL_MSG = CFG_REBOOT_ON_BLE_OTA_APP;

    /**
     * There is no valid application available
     * Erase all sectors specified by byte1 and byte1 in SRAM1 to download a new App.
     */
    CFG_OTA_START_SECTOR_IDX_VAL_MSG = CFG_APP_START_SECTOR_INDEX;
    CFG_OTA_NBR_OF_SECTOR_VAL_MSG = 0xFF;
  }
  return;
}
#endif /* OTA_SBSFU */

/**
 * @brief  Setup the microcontroller system.
 * @param  None
 * @retval None
 */
void SystemInit(void)
{
  OTP_ID0_t * p_otp;

#ifdef OTA_SBSFU
  if (STANDALONE_LOADER_STATE == STANDALONE_LOADER_DWL_REQ)
  {
    /* Standard SBSFU start-up at next reset */
    STANDALONE_LOADER_STATE = STANDALONE_LOADER_NO_REQ;
  }      
#else  
  BootModeCheck();
#endif /* OTA_SBSFU */

  /* FPU settings ------------------------------------------------------------*/
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
  SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));  /* set CP10 and CP11 Full Access */
#endif

/**
 * When out of the SBSFU, the system configuration is
 *            System Clock source            = PLL (MSI)
 *            SYSCLK(Hz)                     = 64000000
 *            HCLK(Hz)                       = 64000000
 *            HCLK1 Prescaler                = 1
 *            HCKL2 Prescaler                = 2
 *            HCKLS Prescaler                = 1
 *            APB1 Prescaler                 = 1
 *            APB2 Prescaler                 = 1
 *            MSI Frequency(Hz)              = 4000000
 *            PLL_M                          = 1
 *            PLL_N                          = 32
 *            PLL_P                          = 5
 *            PLL_Q                          = 4
 *            PLL_R                          = 2
 *            Flash Latency(WS)              = 3
 *
 * The new system configuration shall be applied in a different way when SBSFU is used versus
 * the case when the device is out of reset
 */

  /**
   * Read HSE_Tuning from OTP
   */
  p_otp = (OTP_ID0_t *) OTP_Read(0);
  if (p_otp)
  {
    LL_RCC_HSE_SetCapacitorTuning(p_otp->hse_tuning);
  }

  LL_RCC_HSE_Enable();

#ifndef OTA_SBSFU
  /**
   * Set FLASH latency to 1WS
   */
  LL_FLASH_SetLatency( LL_FLASH_LATENCY_1 );
  while( LL_FLASH_GetLatency() != LL_FLASH_LATENCY_1 );
#endif

  /**
   * Switch to HSE
   *
   */
  while(!LL_RCC_HSE_IsReady());
  LL_RCC_SetSysClkSource( LL_RCC_SYS_CLKSOURCE_HSE );
  while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSE);

#ifdef OTA_SBSFU
  /**
   * Switch OFF the PLL to save power
   */
  LL_RCC_PLL_Disable();

  /**
   * Set FLASH latency to 1WS
   */
  LL_FLASH_SetLatency( LL_FLASH_LATENCY_1 );
  while( LL_FLASH_GetLatency() != LL_FLASH_LATENCY_1 );

  /**
   * Set AHB2 prescaler so that CPU is clocked by 32 MHz
   */
  LL_C2_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
#endif

  /**
   * Switch OFF MSI
   */
  LL_RCC_MSI_Disable();

  /* Configure the Vector Table location add offset address ------------------*/
#if defined(VECT_TAB_SRAM)
  /* program in SRAM1 */
  SCB->VTOR = RAM1_BASE | VECT_TAB_OFFSET;  /* Vector Table Relocation in Internal SRAM1 for CPU1 */
#elif defined(VECT_TAB_OFFSET)
  SCB->VTOR = VECT_TAB_OFFSET;              /* Vector Table Relocation in Internal FLASH */
#endif

}

/**
 * @brief  Update SystemCoreClock variable according to Clock Register Values.
 *         The SystemCoreClock variable contains the core clock (HCLK), it can
 *         be used by the user application to setup the SysTick timer or configure
 *         other parameters.
 *
 * @note   Each time the core clock (HCLK) changes, this function must be called
 *         to update SystemCoreClock variable value. Otherwise, any configuration
 *         based on this variable will be incorrect.
 *
 * @note   - The system frequency computed by this function is not the real
 *           frequency in the chip. It is calculated based on the predefined
 *           constant and the selected clock source:
 *
 *           - If SYSCLK source is MSI, SystemCoreClock will contain the MSI_VALUE(*)
 *
 *           - If SYSCLK source is HSI, SystemCoreClock will contain the HSI_VALUE(**)
 *
 *           - If SYSCLK source is HSE, SystemCoreClock will contain the HSE_VALUE(***)
 *
 *           - If SYSCLK source is PLL, SystemCoreClock will contain the HSE_VALUE(***)
 *             or HSI_VALUE(*) or MSI_VALUE(*) multiplied/divided by the PLL factors.
 *
 *         (*) MSI_VALUE is a constant defined in stm32wbxx_hal.h file (default value
 *             4 MHz) but the real value may vary depending on the variations
 *             in voltage and temperature.
 *
 *         (**) HSI_VALUE is a constant defined in stm32wbxx_hal_conf.h file (default value
 *              16 MHz) but the real value may vary depending on the variations
 *              in voltage and temperature.
 *
 *         (***) HSE_VALUE is a constant defined in stm32wbxx_hal_conf.h file (default value
 *              32 MHz), user has to ensure that HSE_VALUE is same as the real
 *              frequency of the crystal used. Otherwise, this function may
 *              have wrong result.
 *
 *         - The result of this function could be not correct when using fractional
 *           value for HSE crystal.
 *
 * @param  None
 * @retval None
 */
void SystemCoreClockUpdate(void)
{
  uint32_t tmp = 0, msirange = 0, pllvco = 0, pllr = 2, pllsource = 0, pllm = 2;

  /* Get MSI Range frequency--------------------------------------------------*/

  /*MSI frequency range in Hz*/
  msirange = MSIRangeTable[(RCC->CR & RCC_CR_MSIRANGE) >> RCC_CR_MSIRANGE_Pos];

  /*SystemCoreClock=HAL_RCC_GetSysClockFreq();*/
  /* Get SYSCLK source -------------------------------------------------------*/
  switch (RCC->CFGR & RCC_CFGR_SWS)
  {
    case 0x00:   /* MSI used as system clock source */
      SystemCoreClock = msirange;
      break;

    case 0x04:  /* HSI used as system clock source */
      /* HSI used as system clock source */
      SystemCoreClock = HSI_VALUE;
      break;

    case 0x08:  /* HSE used as system clock source */
      SystemCoreClock = HSE_VALUE;
      break;

    case 0x0C: /* PLL used as system clock  source */
      /* PLL_VCO = (HSE_VALUE or HSI_VALUE or MSI_VALUE/ PLLM) * PLLN
         SYSCLK = PLL_VCO / PLLR
       */
      pllsource = (RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC);
      pllm = ((RCC->PLLCFGR & RCC_PLLCFGR_PLLM) >> RCC_PLLCFGR_PLLM_Pos) + 1 ;

      switch (pllsource)
      {
        case 0x02:  /* HSI used as PLL clock source */
          pllvco = (HSI_VALUE / pllm);
          break;

        case 0x03:  /* HSE used as PLL clock source */
          pllvco = (HSE_VALUE / pllm);
          break;

        default:    /* MSI used as PLL clock source */
          pllvco = (msirange / pllm);
          break;
      }

      pllvco = pllvco * ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> RCC_PLLCFGR_PLLN_Pos);
      pllr = (((RCC->PLLCFGR & RCC_PLLCFGR_PLLR) >> RCC_PLLCFGR_PLLR_Pos) + 1);

      SystemCoreClock = pllvco/pllr;
      break;

        default:
          SystemCoreClock = msirange;
          break;
  }

  /* Compute HCLK clock frequency --------------------------------------------*/
  /* Get HCLK1 prescaler */
  tmp = AHBPrescTable[((RCC->CFGR & RCC_CFGR_HPRE) >> RCC_CFGR_HPRE_Pos)];
  /* HCLK clock frequency */
  SystemCoreClock = SystemCoreClock / tmp;

}


/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */
