#include "main.h"
#include "hw_if.h"

void NMI_Handler(void)
{
  while (1);
}

/* A crash in the bootloader shouldn't strand the keyword: reset, which
 * boots the app if it's valid or comes back here. */
void HardFault_Handler(void)
{
  NVIC_SystemReset();
}

void SysTick_Handler(void)
{
  HAL_IncTick();
}

void IPCC_C1_RX_IRQHandler(void)
{
  HW_IPCC_Rx_Handler();
}

void IPCC_C1_TX_IRQHandler(void)
{
  HW_IPCC_Tx_Handler();
}

void RTC_WKUP_IRQHandler(void)
{
  HW_TS_RTC_Wakeup_Handler();
}
