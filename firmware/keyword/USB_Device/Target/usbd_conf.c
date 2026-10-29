/*
 * Glue between ST's USB device library and the HAL PCD driver, based on
 * ST's CDC_Standalone usbd_conf.c (STM32CubeWB 1.24.0).
 *
 * hpcd_USB_FS belongs to main.c: MX_USB_PCD_Init (CubeMX) already ran
 * HAL_PCD_Init and HAL_PCD_MspInit set up the pins, so USBD_LL_Init only
 * links the handles, lays out the packet memory and enables the IRQ.
 */
#include "main.h"
#include "usbd_def.h"
#include "usbd_core.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"

extern PCD_HandleTypeDef hpcd_USB_FS;

static USBD_StatusTypeDef USBD_Get_USB_Status(HAL_StatusTypeDef hal_status)
{
  switch (hal_status)
  {
    case HAL_OK:   return USBD_OK;
    case HAL_BUSY: return USBD_BUSY;
    default:       return USBD_FAIL;
  }
}

/* ---- PCD -> USB device library ---- */

void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd)
{
  USBD_LL_SetupStage((USBD_HandleTypeDef *)hpcd->pData, (uint8_t *)hpcd->Setup);
}

void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
  USBD_LL_DataOutStage((USBD_HandleTypeDef *)hpcd->pData, epnum, hpcd->OUT_ep[epnum].xfer_buff);
}

void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
  USBD_LL_DataInStage((USBD_HandleTypeDef *)hpcd->pData, epnum, hpcd->IN_ep[epnum].xfer_buff);
}

void HAL_PCD_SOFCallback(PCD_HandleTypeDef *hpcd)
{
  USBD_LL_SOF((USBD_HandleTypeDef *)hpcd->pData);
}

void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd)
{
  CDC_Detached();
  USBD_LL_SetSpeed((USBD_HandleTypeDef *)hpcd->pData, USBD_SPEED_FULL);
  USBD_LL_Reset((USBD_HandleTypeDef *)hpcd->pData);
}

/* VBUS isn't wired to the MCU (it only goes to the BQ25895), so a bus
 * suspend is how an unplug shows up. */
void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *hpcd)
{
  CDC_Detached();
  USBD_LL_Suspend((USBD_HandleTypeDef *)hpcd->pData);
}

void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *hpcd)
{
  USBD_LL_Resume((USBD_HandleTypeDef *)hpcd->pData);
}

void HAL_PCD_ISOOUTIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
  USBD_LL_IsoOUTIncomplete((USBD_HandleTypeDef *)hpcd->pData, epnum);
}

void HAL_PCD_ISOINIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
  USBD_LL_IsoINIncomplete((USBD_HandleTypeDef *)hpcd->pData, epnum);
}

void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *hpcd)
{
  USBD_LL_DevConnected((USBD_HandleTypeDef *)hpcd->pData);
}

void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *hpcd)
{
  CDC_Detached();
  USBD_LL_DevDisconnected((USBD_HandleTypeDef *)hpcd->pData);
}

/* ---- USB device library -> PCD ---- */

USBD_StatusTypeDef USBD_LL_Init(USBD_HandleTypeDef *pdev)
{
  hpcd_USB_FS.pData = pdev;
  pdev->pData = &hpcd_USB_FS;

  /* Packet memory: 8 byte BTABLE for 5 endpoints, then 64 bytes each */
  /* Buffers start after the table for endpoints 0-3 (4 x 8 bytes). */
  HAL_PCDEx_PMAConfig(&hpcd_USB_FS, 0x00, PCD_SNG_BUF, 0x20);
  HAL_PCDEx_PMAConfig(&hpcd_USB_FS, 0x80, PCD_SNG_BUF, 0x60);
  HAL_PCDEx_PMAConfig(&hpcd_USB_FS, CDC_IN_EP, PCD_SNG_BUF, 0xA0);
  HAL_PCDEx_PMAConfig(&hpcd_USB_FS, CDC_OUT_EP, PCD_SNG_BUF, 0xE0);
  HAL_PCDEx_PMAConfig(&hpcd_USB_FS, CDC_CMD_EP, PCD_SNG_BUF, 0x120);
  HAL_PCDEx_PMAConfig(&hpcd_USB_FS, 0x83, PCD_SNG_BUF, 0x130);   /* keyboard, usbd_kw.c */

  /* Below the radio's IPCC/RTC IRQs (priority 0). */
  HAL_NVIC_SetPriority(USB_LP_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(USB_LP_IRQn);
  return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_DeInit(USBD_HandleTypeDef *pdev)
{
  HAL_NVIC_DisableIRQ(USB_LP_IRQn);
  return USBD_Get_USB_Status(HAL_PCD_DeInit(pdev->pData));
}

USBD_StatusTypeDef USBD_LL_Start(USBD_HandleTypeDef *pdev)
{
  return USBD_Get_USB_Status(HAL_PCD_Start(pdev->pData));
}

USBD_StatusTypeDef USBD_LL_Stop(USBD_HandleTypeDef *pdev)
{
  return USBD_Get_USB_Status(HAL_PCD_Stop(pdev->pData));
}

USBD_StatusTypeDef USBD_LL_OpenEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr, uint8_t ep_type, uint16_t ep_mps)
{
  return USBD_Get_USB_Status(HAL_PCD_EP_Open(pdev->pData, ep_addr, ep_mps, ep_type));
}

USBD_StatusTypeDef USBD_LL_CloseEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
  return USBD_Get_USB_Status(HAL_PCD_EP_Close(pdev->pData, ep_addr));
}

USBD_StatusTypeDef USBD_LL_FlushEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
  return USBD_Get_USB_Status(HAL_PCD_EP_Flush(pdev->pData, ep_addr));
}

USBD_StatusTypeDef USBD_LL_StallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
  return USBD_Get_USB_Status(HAL_PCD_EP_SetStall(pdev->pData, ep_addr));
}

USBD_StatusTypeDef USBD_LL_ClearStallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
  return USBD_Get_USB_Status(HAL_PCD_EP_ClrStall(pdev->pData, ep_addr));
}

uint8_t USBD_LL_IsStallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
  PCD_HandleTypeDef *hpcd = (PCD_HandleTypeDef *)pdev->pData;
  if ((ep_addr & 0x80) == 0x80) return hpcd->IN_ep[ep_addr & 0x7F].is_stall;
  return hpcd->OUT_ep[ep_addr & 0x7F].is_stall;
}

USBD_StatusTypeDef USBD_LL_SetUSBAddress(USBD_HandleTypeDef *pdev, uint8_t dev_addr)
{
  return USBD_Get_USB_Status(HAL_PCD_SetAddress(pdev->pData, dev_addr));
}

USBD_StatusTypeDef USBD_LL_Transmit(USBD_HandleTypeDef *pdev, uint8_t ep_addr, uint8_t *pbuf, uint32_t size)
{
  return USBD_Get_USB_Status(HAL_PCD_EP_Transmit(pdev->pData, ep_addr, pbuf, size));
}

USBD_StatusTypeDef USBD_LL_PrepareReceive(USBD_HandleTypeDef *pdev, uint8_t ep_addr, uint8_t *pbuf, uint32_t size)
{
  return USBD_Get_USB_Status(HAL_PCD_EP_Receive(pdev->pData, ep_addr, pbuf, size));
}

uint32_t USBD_LL_GetRxDataSize(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
  return HAL_PCD_EP_GetRxCount((PCD_HandleTypeDef *)pdev->pData, ep_addr);
}

void USBD_LL_Delay(uint32_t Delay)
{
  HAL_Delay(Delay);
}

/* The CDC class asks for its handle once, so one static block does. */
void *USBD_static_malloc(uint32_t size)
{
  UNUSED(size);
  static uint32_t mem[(sizeof(USBD_CDC_HandleTypeDef) / 4) + 1];
  return mem;
}

void USBD_static_free(void *p)
{
  UNUSED(p);
}
