#include "main.h"
#include "app_conf.h"
#include "usb_device.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"
#include "usbd_kw.h"

USBD_HandleTypeDef hUsbDeviceFS;

void USB_Clock_HSI48(void)
{
  /* CPU2 may switch CLK48 for its RNG unless CPU1 holds this semaphore,
   * so take it for as long as USB is on (AN5289). */
  __HAL_RCC_HSEM_CLK_ENABLE();
  while (LL_HSEM_1StepLock(HSEM, CFG_HW_CLK48_CONFIG_SEMID));

  LL_RCC_HSI48_Enable();
  while (!LL_RCC_HSI48_IsReady());
  LL_RCC_SetUSBClockSource(LL_RCC_USB_CLKSOURCE_HSI48);
  /* CubeMX's MSP just started PLLSAI1 for USB; nothing else uses it. */
  __HAL_RCC_PLLSAI1_DISABLE();

  RCC_CRSInitTypeDef crs = {0};
  __HAL_RCC_CRS_CLK_ENABLE();
  crs.Prescaler = RCC_CRS_SYNC_DIV1;
  crs.Source = RCC_CRS_SYNC_SOURCE_USB;
  crs.Polarity = RCC_CRS_SYNC_POLARITY_RISING;
  crs.ReloadValue = __HAL_RCC_CRS_RELOADVALUE_CALCULATE(48000000, 1000);
  crs.ErrorLimitValue = RCC_CRS_ERRORLIMIT_DEFAULT;
  crs.HSI48CalibrationValue = RCC_CRS_HSI48CALIBRATION_DEFAULT;
  HAL_RCCEx_CRSConfig(&crs);
}

void MX_USB_Device_Init(void)
{
  CDC_PortInit();
  if (USBD_Init(&hUsbDeviceFS, &CDC_Desc, DEVICE_FS) != USBD_OK
      || USBD_RegisterClass(&hUsbDeviceFS, &USBD_KW) != USBD_OK   /* CDC plus a keyboard */
      || USBD_CDC_RegisterInterface(&hUsbDeviceFS, &USBD_Interface_fops_FS) != USBD_OK
      || USBD_Start(&hUsbDeviceFS) != USBD_OK)
  {
    Error_Handler();
  }
}

/* USB_LP isn't enabled in keyword.ioc, so the handler lives here rather
 * than in the generated stm32wbxx_it.c. */
void USB_LP_IRQHandler(void)
{
  extern PCD_HandleTypeDef hpcd_USB_FS;
  HAL_PCD_IRQHandler(&hpcd_USB_FS);
}
