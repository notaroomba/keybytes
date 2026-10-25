
/**
  ******************************************************************************
  * @file    svc_ctl.h
  * @author  MCD Application Team
  * @brief   Header for ble_controller.c module
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2018-2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */


/**
 * The BLE Controller supports the application to handle services and clients.
 * It provides an API to initialize the BLE core Device and a handler mechanism to rout the GATT/GAP events to the
 * application. When the ble_controller is used (recommended), the application shall register a callback for each
 * Service and each Client implemented. This is already done with the Services and Clients provided in that delivery.
 *  + A GATT event is relevant to only one Service and/or one Client. When a GATT event is received, it is notified to
 *    the registered handlers to the BLE controller. When no registered handler acknowledges positively the GATT event,
 *    it is reported to the application.
 *  + A GAP event is not relevant to either a Service or a Client. It is sent to the application
 *  + In case the application does not want to take benefit from the ble_controller, it could bypass it. In that case,
 *  the application shall:
 *    - call  SVCCTL_Init() to initialize the BLE core device (or implement on its own what is inside that function
 *    - implement TLHCI_UserEvtRx() which is the notification from the HCI layer to report all events (GATT/GAP).
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SVCCTL_H
#define __SVCCTL_H

#ifdef __cplusplus
extern "C" {
#endif

  /* Includes ------------------------------------------------------------------*/
  /* Exported types ------------------------------------------------------------*/
  typedef enum
  {
    SVCCTL_EvtNotAck,
    SVCCTL_EvtAckFlowEnable,
    SVCCTL_EvtAckFlowDisable,
  } SVCCTL_EvtAckStatus_t;

  typedef enum
  {
    SVCCTL_UserEvtFlowDisable,
    SVCCTL_UserEvtFlowEnable,
  } SVCCTL_UserEvtFlowStatus_t;

  typedef SVCCTL_EvtAckStatus_t (*SVC_CTL_p_EvtHandler_t)(void *p_evt);

  /* Exported constants --------------------------------------------------------*/
  /* External variables --------------------------------------------------------*/
  /* Exported macros -----------------------------------------------------------*/

  /* Exported functions ------------------------------------------------------- */
  /**
   * @brief  It initializes the BLE core Driver and sends some commands to initialize the BLE core device
   *         It shall be called before any BLE operation
   *
   * @param  None
   * @retval None
   */
  void SVCCTL_Init( void );

  /**
   * @brief  This API registers a handler to be called when a GATT user event is received from the BLE core device. When
   *         a Service is created, it shall register a callback to be notified when a GATT event is received from the
   *         BLE core device. When a GATT event is received, it shall be checked in the handler if the GATT events belongs
   *         to the Service or not. The handler shall return the correct status depending on the result. As soon as one
   *         Service handler registered acknowledges positively the GATT event, the ble_controller stops calling the
   *         registered Service handlers.
   *         This handler is called in the TL_BLE_HCI_UserEvtProc() context
   *
   * @param  pfBLE_SVC_Service_Event_Handler: This is the Service handler that the ble_controller calls to report a GATT
   *         event received. If the GATT event belongs to that Service, the callback shall return positively with
   *         SVCCTL_EvtAckFlowEnable.
   * @retval None
   */
  void SVCCTL_RegisterSvcHandler( SVC_CTL_p_EvtHandler_t pfBLE_SVC_Service_Event_Handler );

  /**
   * @brief  This API registers a handler to be called when a GATT user event is received from the BLE core device. When
   *         a Client is created, it shall register a callback to be notified when a GATT event is received from the
   *         BLE core device. When a GATT event is received, it shall be checked in the handler if the GATT events belongs
   *         to the Client or not. The handler shall return the correct status depending on the result. As soon as one
   *         Client handler registered acknowledges positively the GATT event, the ble_controller stops calling the
   *         registered Client handlers.
   *         This handler is called in the TL_BLE_HCI_UserEvtProc() context
   *
   * @param  pfBLE_SVC_Client_Event_Handler: This is the Client handler that the ble_controller calls to report a GATT
   *         event received. If the GATT event belongs to that Client, the callback shall return positively with
   *         SVCCTL_EvtAckFlowEnable.
   * @retval None
   */
  void SVCCTL_RegisterCltHandler( SVC_CTL_p_EvtHandler_t pfBLE_SVC_Client_Event_Handler );

  /**
   * @brief  This API is used to resume the User Event Flow that has been stopped in return of SVCCTL_UserEvtRx()
   *
   * @param  None
   * @retval None
   */
  void SVCCTL_ResumeUserEventFlow( void );


  /**
   * @brief This callback is triggered when either
   *          + a GAP event is received from the BLE core device.
   *          + a GATT event that has not been positively acknowledged by the registered handler is received from the
   *            BLE core device.
   *        The event is returned in a HCI packet. The full HCI packet is stored in a single buffer and is available when
   *        this callback is triggered. However, an ACI event may be longer than a HCI packet and could be fragmented over
   *        several HCI packets. The HCI layer only handles HCI packets so when an ACI packet is split over several HCI
   *        packets, this callback is triggered for each HCI fragment. It is the responsibility of the application to
   *        reassemble the ACI event.
   *        This callback is triggered in the TL_BLE_HCI_UserEvtProc() context
   *
   * @param  pckt: The user event received from the BLE core device
   * @retval None
   */
  SVCCTL_UserEvtFlowStatus_t SVCCTL_App_Notification( void *pckt );

  /**
   * @brief
   *
   *
   * @param  pckt: The user event received from the BLE core device
   * @retval SVCCTL_UserEvtFlowStatus_t: SVCCTL_UserEvtFlowEnable when the packet has been processed
   *         SVCCTL_UserEvtFlowDisable otherwise (the packet is kept in the queue)
   */
  SVCCTL_UserEvtFlowStatus_t SVCCTL_UserEvtRx( void *pckt );

  /**
   * @brief This API may be used by the application when the Service Controller is used to add a custom service
   *
   *
   * @param  None
   * @retval None
   */
  void SVCCTL_InitCustomSvc( void );

  /**
   * @brief This API may be overloaded by the application to select a limited list of ble services to initialize.
   *        It is called by SVCCTL_Init()
   *        By default, SVCCTL_SvcInit() is implemented to initialize all BLE services which are included in the
   *        application at build time
   *        If it is required to initialize only limited part of the BLE service available in the application,
   *        this API may be used to call the initialization API of the subset of needed services at run time.
   *
   * @param  None
   * @retval None
   */
  void SVCCTL_SvcInit( void );

#ifdef __cplusplus
}
#endif

#endif /*__SVCCTL_H */


