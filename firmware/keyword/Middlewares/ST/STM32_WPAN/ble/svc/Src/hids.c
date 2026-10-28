/**
 ******************************************************************************
 * @file    hids.c
 * @author  MCD Application Team
 * @brief   Human Interface Device Service
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



/* Includes ------------------------------------------------------------------*/
#include "common_blesvc.h"


/* Private typedef -----------------------------------------------------------*/
typedef struct
{
  uint16_t HidSvcHdle;                      /**< Service handle        */
#if (BLE_CFG_HIDS_PROTOCOL_MODE_CHAR != 0)
  uint16_t HidProtocolModeCharHdle;         /**< Characteristic handle */
#endif
#if (BLE_CFG_HIDS_REPORT_CHAR != 0)
  uint16_t HidReportCharHdle[BLE_CFG_HIDS_INPUT_REPORT_NB +
                             BLE_CFG_HIDS_OUTPUT_REPORT_NB +
                             BLE_CFG_HIDS_FEATURE_REPORT_NB]; /**< Characteristic handle */
  uint16_t HidReportReferenceDescHdle[BLE_CFG_HIDS_INPUT_REPORT_NB +
                                      BLE_CFG_HIDS_OUTPUT_REPORT_NB +
                                      BLE_CFG_HIDS_FEATURE_REPORT_NB];      /**< Descriptor handle */
#endif                                      
  uint16_t HidReportMapCharHdle;            /**< Characteristic handle */
#if (BLE_CFG_HIDS_EXTERNAL_REPORT_REFERENCE != 0)
  uint16_t HidExternalReferenceDescHdle;    /**< Descriptor handle */
#endif
#if (BLE_CFG_HIDS_KEYBOARD_DEVICE != 0)          
  uint16_t HidKeyboardReportInputCharHdle;  /**< Characteristic handle */
  uint16_t HidKeyboardReportOutputCharHdle; /**< Characteristic handle */
#endif
#if (BLE_CFG_HIDS_MOUSE_DEVICE != 0) 
  uint16_t HidMouseReportInputCharHdle;      /**< Characteristic handle */
#endif
  uint16_t HidInformationCharHdle;          /**< Characteristic handle */
  uint16_t HidControlPointCharHdle;         /**< Characteristic handle */
}HIDS_Context_t;


/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Store Value into a buffer in Little Endian Format */
#define STORE_LE_16(buf, val)    ( ((buf)[0] =  (uint8_t) (val)    ) , \
                                   ((buf)[1] =  (uint8_t) (val>>8) ) )
#define STORE_LE_32(buf, val)    ( ((buf)[0] =  (uint8_t) (val)     ) , \
                                   ((buf)[1] =  (uint8_t) (val>>8)  ) , \
                                   ((buf)[2] =  (uint8_t) (val>>16) ) , \
                                   ((buf)[3] =  (uint8_t) (val>>24) ) ) 


/* Private variables ---------------------------------------------------------*/

/**
 * START of Section BLE_DRIVER_CONTEXT
 */

PLACE_IN_SECTION("BLE_DRIVER_CONTEXT") static HIDS_Context_t HIDS_Context[BLE_CFG_HIDS_NUMBER];

/**
 * END of Section BLE_DRIVER_CONTEXT
 */


/* Private function prototypes -----------------------------------------------*/
/* Functions Definition ------------------------------------------------------*/
/* Private functions ----------------------------------------------------------*/

/**
 * @brief  HID Handle Control Point write
 * @retval None
 */
static void HIDS_Handle_Control_Point_Write(uint8_t service_instance, uint8_t *attVal)
{
  tBleStatus hciCmdResult;
  uint16_t length;
  uint8_t data;
  
  hciCmdResult = aci_gatt_read_handle_value((HIDS_Context[service_instance].HidControlPointCharHdle) + 1,
                                            0,
                                            1,
                                            &length,
                                            &length,
                                            &data);
  if (hciCmdResult == BLE_STATUS_SUCCESS)
  {
    BLE_DBG_HIDS_MSG("Command %d of length %d\n", data, length);
  }
  else
  {
    BLE_DBG_HIDS_MSG ("FAILED to read handle value, Error: 0x%02X !!\n", 
                         hciCmdResult);
  }
}


#if((BLE_CFG_HIDS_MOUSE_DEVICE != 0) && (BLE_CFG_HIDS_MOUSE_INPUT_WRITE != 0))
/**
 * @brief  HID Handle Mouse Input write
 * @retval None
 */
static void HIDS_Handle_Mouse_Input_Write(uint8_t service_instance, uint8_t *attVal)
{
  tBleStatus hciCmdResult;
  uint16_t length;
  uint8_t data[BLE_CFG_HIDS_BOOT_MOUSE_INPUT_REPORT_MAX_LEN];
  HIDS_App_Notification_evt_t Notification;
  
  hciCmdResult = aci_gatt_read_handle_value((HIDS_Context[service_instance].HidMouseReportInputCharHdle) + 1,
                                            0,
                                            BLE_CFG_HIDS_BOOT_MOUSE_INPUT_REPORT_MAX_LEN,
                                            &length,
                                            &length,
                                            data);
  if (hciCmdResult == BLE_STATUS_SUCCESS)
  {
    uint8_t i;
    
    for(i = 0; i < length; i++)
      BLE_DBG_HIDS_MSG("Data[%d] 0x%X \n", i, data[i]);
  }
  else
  {
    BLE_DBG_HIDS_MSG ("FAILED to read handle value, Error: 0x%02X !!\n", 
                         hciCmdResult);
  }
  
  /**
   * Notify the application of notification setting
   */
  Notification.HIDS_Evt_Opcode = HIDS_MOUSE_INPUT_REPORT;
  Notification.Instance = service_instance;
  Notification.Index = 0;
  Notification.pReport = data;
  Notification.ReportLength = length;

  HIDS_Notification(&Notification);
}
#endif

#if((BLE_CFG_HIDS_KEYBOARD_DEVICE != 0) && (BLE_CFG_HIDS_KEYBOARD_INPUT_WRITE != 0))
/**
 * @brief  HID Handle Keyboard Output write
 * @retval None
 */
static void HIDS_Handle_Keyboard_Output_Write(uint8_t service_instance, uint8_t *attVal)
{
  tBleStatus hciCmdResult;
  uint16_t length;
  uint8_t data[BLE_CFG_HIDS_BOOT_KEYBOARD_OUTPUT_REPORT_MAX_LEN];
  HIDS_App_Notification_evt_t Notification;
  
  hciCmdResult = aci_gatt_read_handle_value((HIDS_Context[service_instance].HidKeyboardReportOutputCharHdle) + 1, 
                                            0,
                                            BLE_CFG_HIDS_BOOT_KEYBOARD_OUTPUT_REPORT_MAX_LEN,
                                            &length,
                                            &length,
                                            data);
  if (hciCmdResult == BLE_STATUS_SUCCESS)
  {
    uint8_t i;
    
    for(i = 0; i < length; i++)
      BLE_DBG_HIDS_MSG("Data[%d] 0x%X \n", i, data[i]);
  }
  else
  {
    BLE_DBG_HIDS_MSG ("FAILED to read handle value, Error: 0x%02X !!\n", 
                         hciCmdResult);
  }
  
  /**
   * Notify the application of notification setting
   */
  Notification.HIDS_Evt_Opcode = HIDS_KEYBOARD_OUTPUT_REPORT;
  Notification.Instance = service_instance;
  Notification.pReport = data;
  Notification.ReportLength = length;

  HIDS_Notification(&Notification);
}
#endif


#if((BLE_CFG_HIDS_KEYBOARD_DEVICE != 0) && (BLE_CFG_HIDS_KEYBOARD_INPUT_WRITE != 0))
/**
 * @brief  HID Handle Keyboard Input write
 * @retval None
 */
static void HIDS_Handle_Keyboard_Input_Write(uint8_t service_instance, uint8_t *attVal)
{
  tBleStatus hciCmdResult;
  uint8_t data[BLE_CFG_HIDS_BOOT_KEYBOARD_INPUT_REPORT_MAX_LEN];
  uint16_t length;
  HIDS_App_Notification_evt_t Notification;
  
  hciCmdResult = aci_gatt_read_handle_value((HIDS_Context[service_instance].HidKeyboardReportInputCharHdle) + 1,
                                            0,
                                            BLE_CFG_HIDS_BOOT_KEYBOARD_INPUT_REPORT_MAX_LEN,
                                            &length,
                                            &length,
                                            data);
  if (hciCmdResult == BLE_STATUS_SUCCESS)
  {
    uint8_t i;
    
    for(i = 0; i < length; i++)
      BLE_DBG_HIDS_MSG("Data[%d] 0x%X \n", i, data[i]);
  }
  else
  {
    BLE_DBG_HIDS_MSG ("FAILED to read handle value, Error: 0x%02X !!\n", 
                         hciCmdResult);
  }
  
  /**
   * Notify the application of notification setting
   */
  Notification.HIDS_Evt_Opcode = HIDS_KEYBOARD_INPUT_REPORT;
  Notification.Instance = service_instance;
  Notification.pReport = data;
  Notification.ReportLength = length;

  HIDS_Notification(&Notification);
}
#endif


#if(BLE_CFG_HIDS_REPORT_CHAR != 0)
/**
 * @brief  HID Handle Report
 * @retval None
 */
static void HIDS_Handle_Report(uint8_t service_instance, uint8_t report_index, uint8_t *attVal)
{
  tBleStatus hciCmdResult;
  uint8_t data[BLE_CFG_HIDS_REPORT_MAX_LEN];
  uint16_t length;
  HIDS_App_Notification_evt_t Notification;

  /**
   * Category ID update
   */
  hciCmdResult = aci_gatt_read_handle_value((HIDS_Context[service_instance].HidReportCharHdle[report_index]) + 1,
                                            0,
                                            BLE_CFG_HIDS_REPORT_MAX_LEN,
                                            &length,
                                            &length,
                                            data);
  if (hciCmdResult == BLE_STATUS_SUCCESS)
  {
    uint8_t i;
    
    for(i = 0; i < length; i++)
      BLE_DBG_HIDS_MSG("Report %d Data[%d] 0x%X \n", report_index, i, data[i]);
  }
  else
  {
    BLE_DBG_HIDS_MSG ("FAILED to read handle value of 0x%x, Error: 0x%02X !!\n", 
                      (HIDS_Context[service_instance].HidReportCharHdle[report_index]) + 1,
                      hciCmdResult);
  }

  /**
   * Notify the application of notification setting
   */
  Notification.HIDS_Evt_Opcode = HIDS_OUTPUT_REPORT;
  Notification.Instance = service_instance;
  Notification.Index = report_index;
  Notification.pReport = data;
  Notification.ReportLength = length;

  HIDS_Notification(&Notification);
}
#endif


#if(BLE_CFG_HIDS_PROTOCOL_MODE_CHAR != 0)
/**
 * @brief  HID Handle Protocol Mode
 * @retval None
 */
static void HIDS_Handle_Protocol_Mode(uint8_t service_instance, uint8_t *attVal)
{
  tBleStatus hciCmdResult;
  uint16_t length;
  uint8_t data;
  
  hciCmdResult = aci_gatt_read_handle_value((HIDS_Context[service_instance].HidProtocolModeCharHdle) + 1, 
                                            0,
                                            1,
                                            &length,
                                            &length,
                                            &data);
  if (hciCmdResult == BLE_STATUS_SUCCESS)
  {
    BLE_DBG_HIDS_MSG("Mode %d of length %d\n", data, length);
  }
  else
  {
    BLE_DBG_HIDS_MSG ("FAILED to read handle value, Error: 0x%02X !!\n", 
                         hciCmdResult);
  }
}
#endif


/**
 * @brief  Event handler
 * @param  Event: Address of the buffer holding the Event
 * @retval Ack: Return whether the Event has been managed or not
 */
static SVCCTL_EvtAckStatus_t HIDS_Event_Handler(void *Event)
{
  SVCCTL_EvtAckStatus_t return_value;
  hci_event_pckt *event_pckt;
  evt_blecore_aci *blecore_evt;
  aci_gatt_attribute_modified_event_rp0    * attribute_modified;
  uint8_t service_instance;
#if (BLE_CFG_HIDS_REPORT_CHAR != 0)
  uint8_t i;
#endif

  return_value = SVCCTL_EvtNotAck;
  event_pckt = (hci_event_pckt *)(((hci_uart_pckt*)Event)->data);

  switch(event_pckt->evt)
  {
    case HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE:
    {
      blecore_evt = (evt_blecore_aci*)event_pckt->data;
      switch(blecore_evt->ecode)
      {        
#if ((BLE_CFG_HIDS_REPORT_CHAR != 0) || (BLE_CFG_HIDS_KEYBOARD_DEVICE != 0) || (BLE_CFG_HIDS_MOUSE_DEVICE != 0))
        case ACI_GATT_WRITE_PERMIT_REQ_VSEVT_CODE:
        {
          aci_gatt_write_permit_req_event_rp0 * write_perm_req;
          
          write_perm_req = (aci_gatt_write_permit_req_event_rp0*)blecore_evt->data;
          
#if ((BLE_CFG_HIDS_REPORT_CHAR != 0) || (BLE_CFG_HIDS_KEYBOARD_DEVICE != 0) || ((BLE_CFG_HIDS_MOUSE_DEVICE != 0) && (BLE_CFG_HIDS_MOUSE_INPUT_WRITE != 0)))
          for(service_instance = 0; service_instance < BLE_CFG_HIDS_NUMBER ; service_instance++)
          {
#if (BLE_CFG_HIDS_REPORT_CHAR != 0)
            for(i = 0; i < BLE_CFG_HIDS_INPUT_REPORT_NB + BLE_CFG_HIDS_OUTPUT_REPORT_NB + BLE_CFG_HIDS_FEATURE_REPORT_NB; i++)
            {
              if(write_perm_req->Attribute_Handle == (HIDS_Context[service_instance].HidReportCharHdle[i]) + 1) 
              {
                return_value = SVCCTL_EvtAckFlowEnable;

                aci_gatt_write_resp(write_perm_req->Connection_Handle,
                                    write_perm_req->Attribute_Handle,
                                    0x00, /* write_status = 0 (no error))*/
                                    0x00, /* err_code */
                                    write_perm_req->Data_Length,
                                    (uint8_t *)&(write_perm_req->Data[0]));
                BLE_DBG_HIDS_MSG("ACI_GATT_WRITE_PERMIT_REQ_VSEVT_CODE, HidReportCharHdle[%d]: 0x%02X, Len: 0x%02X !!\n", 
                                    i, write_perm_req->Attribute_Handle, write_perm_req->Data_Length); 
                HIDS_Handle_Report(service_instance, i, write_perm_req->Data);
              }
            }
#endif
#if (BLE_CFG_HIDS_KEYBOARD_DEVICE != 0)          
#if (BLE_CFG_HIDS_KEYBOARD_INPUT_WRITE != 0)
            if(write_perm_req->Attribute_Handle == (HIDS_Context[service_instance].HidKeyboardReportInputCharHdle) + 1)
            {
              return_value = SVCCTL_EvtAckFlowEnable;

              aci_gatt_write_resp(write_perm_req->Connection_Handle,
                                  write_perm_req->Attribute_Handle,
                                  0x00, /* write_status = 0 (no error)) */
                                  (uint8_t)0, /* err_code  */
                                  write_perm_req->Data_Length,
                                  (uint8_t *)&(write_perm_req->Data[0]));
              BLE_DBG_HIDS_MSG("ACI_GATT_WRITE_PERMIT_REQ_VSEVT_CODE, HidKeyboardReportInputCharHdle: 0x%02X, Len: 0x%02X !!\n", 
                                  write_perm_req->Attribute_Handle, write_perm_req->Data_Length); 
              HIDS_Handle_Keyboard_Input_Write(service_instance, write_perm_req->Data);
            }
#endif
            if(write_perm_req->Attribute_Handle == (HIDS_Context[service_instance].HidKeyboardReportOutputCharHdle) + 1)
            {
              return_value = SVCCTL_EvtAckFlowEnable;

              aci_gatt_write_resp(write_perm_req->Connection_Handle,
                                  write_perm_req->Attribute_Handle,
                                  0x00, /* write_status = 0 (no error)) */
                                  (uint8_t)0, /* err_code  */
                                  write_perm_req->Data_Length,
                                  (uint8_t *)&(write_perm_req->Data[0]));
              BLE_DBG_HIDS_MSG("ACI_GATT_WRITE_PERMIT_REQ_VSEVT_CODE, HidKeyboardReportOutputCharHdle: 0x%02X, Len: 0x%02X !!\n", 
                                  write_perm_req->Attribute_Handle, write_perm_req->Data_Length); 
              HIDS_Handle_Keyboard_Output_Write(service_instance, write_perm_req->Data);
            }
#endif
#if ((BLE_CFG_HIDS_MOUSE_DEVICE != 0) && (BLE_CFG_HIDS_MOUSE_INPUT_WRITE != 0))
            if(write_perm_req->Attribute_Handle == (HIDS_Context[service_instance].HidMouseReportInputCharHdle) + 1)
            {
              return_value = SVCCTL_EvtAckFlowEnable;

              aci_gatt_write_resp(write_perm_req->Connection_Handle,
                                  write_perm_req->Attribute_Handle,
                                  0x00, /* write_status = 0 (no error)) */
                                  (uint8_t)0, /* err_code */
                                  write_perm_req->Data_Length,
                                  (uint8_t *)&(write_perm_req->Data[0]));
              BLE_DBG_HIDS_MSG("ACI_GATT_WRITE_PERMIT_REQ_VSEVT_CODE, HidMouseReportInputCharHdle: 0x%02X, Len: 0x%02X !!\n", 
                                  write_perm_req->Attribute_Handle, write_perm_req->Data_Length); 
              HIDS_Handle_Mouse_Input_Write(service_instance, write_perm_req->Data);
            }
#endif
          }
#endif
        }
        break;
#endif
        case ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE:
        {
          attribute_modified = (aci_gatt_attribute_modified_event_rp0*)blecore_evt->data;
          for(service_instance = 0; service_instance < BLE_CFG_HIDS_NUMBER ; service_instance++)
          {
#if (BLE_CFG_HIDS_PROTOCOL_MODE_CHAR != 0)
            /* Report characteristic write */
            if(attribute_modified->Attr_Handle == (HIDS_Context[service_instance].HidProtocolModeCharHdle) + 1)
            {
              return_value = SVCCTL_EvtAckFlowEnable;
              BLE_DBG_HIDS_MSG("ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE, HidProtocolModeCharHdle: 0x%02X, Len: 0x%02X !!\n", 
                                  attribute_modified->Attr_Handle, attribute_modified->Attr_Data_Length); 
              HIDS_Handle_Protocol_Mode(service_instance, attribute_modified->Attr_Data);
            }
#endif
#if (BLE_CFG_HIDS_REPORT_CHAR != 0)
            for(i = 0; i < BLE_CFG_HIDS_INPUT_REPORT_NB + BLE_CFG_HIDS_OUTPUT_REPORT_NB + BLE_CFG_HIDS_FEATURE_REPORT_NB ; i++)
            {
              /* Report characteristic write */
              if(attribute_modified->Attr_Handle == (HIDS_Context[service_instance].HidReportCharHdle[i]) + 1)
              {
                return_value = SVCCTL_EvtAckFlowEnable;
                if(i < BLE_CFG_HIDS_INPUT_REPORT_NB)
                {
                  aci_gatt_write_resp(attribute_modified->Connection_Handle,
                                      attribute_modified->Attr_Handle,
                                      0x00, /* write_status = 0 (no error))*/
                                      0x00, /* err_code */
                                      attribute_modified->Attr_Data_Length,
                                      (uint8_t *)&(attribute_modified->Attr_Data[0]));
                }
                BLE_DBG_HIDS_MSG("ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE, HidReportCharHdle[%d]: 0x%02X, Len: 0x%02X !!\n", 
                                 i, attribute_modified->Attr_Handle, attribute_modified->Attr_Data_Length); 
                HIDS_Handle_Report(service_instance, i, attribute_modified->Attr_Data);
              }
              /* Report Client Characteristic Configuration descriptor write */
              if(attribute_modified->Attr_Handle == (HIDS_Context[service_instance].HidReportCharHdle[i]) + 2)
              {
                HIDS_App_Notification_evt_t Notification;
                
                return_value = SVCCTL_EvtAckFlowEnable;
                /**
                 * Notify the application of notification setting
                 */
                Notification.Instance = service_instance;
                Notification.Index = i;
                if(attribute_modified->Attr_Data[0] & COMSVC_Notification)
                {
                  BLE_DBG_HIDS_MSG("ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE HIDS_REPORT_NOTIFICATION_ENABLED\n");
                  Notification.HIDS_Evt_Opcode = HIDS_REPORT_NOTIFICATION_ENABLED;
                  HIDS_Notification(&Notification);
                }
                else
                {
                  BLE_DBG_HIDS_MSG("ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE HIDS_REPORT_NOTIFICATION_DISABLED\n");
                  Notification.HIDS_Evt_Opcode = HIDS_REPORT_NOTIFICATION_DISABLED;
                  HIDS_Notification(&Notification);
                }
                aci_gatt_store_db(); /* Store CCCD value in GATT database: CCCD state restored if reset */
              }
            } /* End for(i = 0; i < HIDS_INPUT_REPORT_NB + HIDS_OUTPUT_REPORT_NB + BLE_CFG_HIDS_FEATURE_REPORT_NB ; i++) */
#endif
#if (BLE_CFG_HIDS_KEYBOARD_DEVICE != 0)
#if (BLE_CFG_HIDS_KEYBOARD_INPUT_WRITE != 0)
            /* Keyboard Report Input characteristic write */
            if(attribute_modified->Attr_Handle == (HIDS_Context[service_instance].HidKeyboardReportInputCharHdle) + 1)
            {
              return_value = SVCCTL_EvtAckFlowEnable;
              aci_gatt_write_resp(attribute_modified->Connection_Handle,
                                  attribute_modified->Attr_Handle,
                                  0x00, /* write_status = 0 (no error))*/
                                  0x00, /* err_code */
                                  attribute_modified->Attr_Data_Length,
                                  (uint8_t *)&(attribute_modified->Attr_Data[0]));
              BLE_DBG_HIDS_MSG("ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE, HidKeyboardReportInputCharHdle: 0x%02X, Len: 0x%02X !!\n", 
                                  attribute_modified->Attr_Handle, attribute_modified->Attr_Data_Length); 
              HIDS_Handle_Keyboard_Input_Write(service_instance, attribute_modified->Attr_Data);
            }
#endif
            /*  Keyboard Report Input Client Characteristic Configuration descriptor write */
            if(attribute_modified->Attr_Handle == (HIDS_Context[service_instance].HidKeyboardReportInputCharHdle) + 2)
            {
              HIDS_App_Notification_evt_t Notification;
              
              return_value = SVCCTL_EvtAckFlowEnable;
              /**
               * Notify the application of write setting
               */
              Notification.Instance = service_instance;
              Notification.Index = 0;
              if(attribute_modified->Attr_Data[0] & COMSVC_Notification)
              {
                BLE_DBG_HIDS_MSG("ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE HIDS_KEYB_INPUT_NOTIFY_ENABLED\n");
                Notification.HIDS_Evt_Opcode = HIDS_KEYB_INPUT_NOTIFY_ENABLED;
                HIDS_Notification(&Notification);
              }
              else
              {
                BLE_DBG_HIDS_MSG("ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE HIDS_KEYB_INPUT_NOTIFY_DISABLED\n");
                Notification.HIDS_Evt_Opcode = HIDS_KEYB_INPUT_NOTIFY_DISABLED;
                HIDS_Notification(&Notification);
              }
              aci_gatt_store_db(); /* Store CCCD value in GATT database: CCCD state restored if reset */
            }
            /* Keyboard Report Output characteristic write */
            if(attribute_modified->Attr_Handle == (HIDS_Context[service_instance].HidKeyboardReportOutputCharHdle) + 1)
            {
              return_value = SVCCTL_EvtAckFlowEnable;
              BLE_DBG_HIDS_MSG("ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE, HidKeyboardReportOutputCharHdle: 0x%02X, Len: 0x%02X !!\n", 
                                  attribute_modified->Attr_Handle, attribute_modified->Attr_Data_Length); 
              HIDS_Handle_Keyboard_Output_Write(service_instance, attribute_modified->Attr_Data);
            }
#endif
#if (BLE_CFG_HIDS_MOUSE_DEVICE != 0)
#if (BLE_CFG_HIDS_MOUSE_INPUT_WRITE != 0)
            /* Mouse Report Input characteristic write */
            if(attribute_modified->Attr_Handle == (HIDS_Context[service_instance].HidMouseReportInputCharHdle) + 1)
            {
              return_value = SVCCTL_EvtAckFlowEnable;
              aci_gatt_write_resp(attribute_modified->Connection_Handle,
                                  attribute_modified->Attr_Handle,
                                  0x00, /* write_status = 0 (no error))*/
                                  0x00, /* err_code */
                                  attribute_modified->Attr_Data_Length,
                                  (uint8_t *)&(attribute_modified->Attr_Data[0]));
              BLE_DBG_HIDS_MSG("ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE, HidMouseReportInputCharHdle: 0x%02X, Len: 0x%02X !!\n", 
                                  attribute_modified->Attr_Handle, attribute_modified->Attr_Data_Length); 
              HIDS_Handle_Mouse_Input_Write(service_instance, attribute_modified->Attr_Data);
            }
#endif
            /*  Mouse Report Input Client Characteristic Configuration descriptor write */
            if(attribute_modified->Attr_Handle == (HIDS_Context[service_instance].HidMouseReportInputCharHdle) + 2)
            {
              HIDS_App_Notification_evt_t Notification;

              return_value = SVCCTL_EvtAckFlowEnable;
              /**
               * Notify the application of write setting
               */
              Notification.Instance = service_instance;
              Notification.Index = 0;
              if(attribute_modified->Attr_Data[0] & COMSVC_Notification)
              {
                BLE_DBG_HIDS_MSG("ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE HIDS_MOUSE_INPUT_NOTIFY_ENABLED\n");
                Notification.HIDS_Evt_Opcode = HIDS_MOUSE_INPUT_NOTIFY_ENABLED;
                HIDS_Notification(&Notification);
              }
              else
              {
                BLE_DBG_HIDS_MSG("ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE HIDS_MOUSE_INPUT_NOTIFY_DISABLED\n");
                Notification.HIDS_Evt_Opcode = HIDS_MOUSE_INPUT_NOTIFY_DISABLED;
                HIDS_Notification(&Notification);
              }
              aci_gatt_store_db(); /* Store CCCD value in GATT database: CCCD state restored if reset */
            }
#endif
            /*  Control Point Characteristic write */
            if(attribute_modified->Attr_Handle == (HIDS_Context[service_instance].HidControlPointCharHdle) + 1)
            {
              return_value = SVCCTL_EvtAckFlowEnable;
              BLE_DBG_HIDS_MSG("ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE, HidControlPointCharHdle: 0x%02X, Len: 0x%02X !!\n", 
                                  attribute_modified->Attr_Handle, attribute_modified->Attr_Data_Length); 
              HIDS_Handle_Control_Point_Write(0/*service_instance*/, attribute_modified->Attr_Data);
            }
          } /* End for(service_instance = 0; service_instance < BLE_CFG_HIDS_NUMBER ; service_instance++) */
        }
        break;

        default:
          break;
      }
    }
    break; /* HCI_HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE_SPECIFIC */

    default:
      break;
  }

  return(return_value);
}/* end HIDS_Event_Handler */


/* Public functions ----------------------------------------------------------*/

/**
 * @brief  Service initialization
 * @param  None
 * @retval None
 */
void HIDS_Init(void)
{
  uint16_t uuid;
  uint8_t service_instance;
#if (BLE_CFG_HIDS_REPORT_CHAR != 0)
  uint8_t i;
#endif
  tBleStatus hciCmdResult;
#if (BLE_CFG_HIDS_PROTOCOL_MODE_CHAR != 0)
  uint8_t protocol_mode;
#endif

  /**
   *	Register the event handler to the BLE controller
   */
  SVCCTL_RegisterSvcHandler(HIDS_Event_Handler);

  for(service_instance = 0; service_instance < BLE_CFG_HIDS_NUMBER; service_instance++)
  {
    /**
     *  Add Human Interface Device Service
     *
     * Max_Attribute_Records = 2*no_of_char + 1
     * service_max_attribute_record = 1 for HID service +
     *                                2 for protocol mode characteristic +
     *                                2 report characteristic +
     *                                1 for client char configuration descriptor +
     *                                1 for report reference descriptor +
     *                                2 for report map characteristic +
     *                                2 boot keyboard input report characteristic +
     *                                1 for client char configuration descriptor +
     *                                2 boot keyboard output report characteristic +
     *                                2 boot mouse input report characteristic +
     *                                1 for client char configuration descriptor +
     *                                2 for HID information characteristic +
     *                                2 for HID control point characteristic
     */
    uuid = HUMAN_INTERFACE_DEVICE_SERVICE_UUID;
    hciCmdResult = aci_gatt_add_service(UUID_TYPE_16,
                                        (Service_UUID_t *) &uuid,
                                        PRIMARY_SERVICE,
#if (BLE_CFG_HIDS_PROTOCOL_MODE_CHAR != 0)
                                        2+
#endif
#if (BLE_CFG_HIDS_REPORT_CHAR != 0)
                                        (4*BLE_CFG_HIDS_INPUT_REPORT_NB)+
                                        (3*BLE_CFG_HIDS_OUTPUT_REPORT_NB)+
                                        (3*BLE_CFG_HIDS_FEATURE_REPORT_NB)+
#endif
#if (BLE_CFG_HIDS_EXTERNAL_REPORT_REFERENCE != 0)
                                         1+
#endif
#if (BLE_CFG_HIDS_KEYBOARD_DEVICE != 0)
                                         5+
#endif
#if (BLE_CFG_HIDS_MOUSE_DEVICE != 0)
                                         3+
#endif
                                         1 + 2 + 2 + 2,      /* Service + Report Map + HID Information + HID Control Point */
                                         &(HIDS_Context[service_instance].HidSvcHdle));
    if (hciCmdResult == BLE_STATUS_SUCCESS)
    {
      BLE_DBG_HIDS_MSG ("Human Interface Device Service (HIDS) is added Successfully 0x%04X\n", 
                        HIDS_Context[service_instance].HidSvcHdle);
    }
    else
    {
      BLE_DBG_HIDS_MSG ("FAILED to add Human Interface Device Service (HIDS), Error: 0x%02X !!\n", 
                        hciCmdResult);
    }


#if (BLE_CFG_HIDS_PROTOCOL_MODE_CHAR != 0)
    /**
     *  Add Protocol Mode Characteristic
     */
    uuid = PROTOCOL_MODE_CHAR_UUID;
    hciCmdResult = aci_gatt_add_char(HIDS_Context[service_instance].HidSvcHdle,
                                     UUID_TYPE_16,
                                     (Char_UUID_t *) &uuid ,
                                     1,
                                     CHAR_PROP_READ | CHAR_PROP_WRITE_WITHOUT_RESP,
                                     ATTR_PERMISSION_NONE,
                                     GATT_NOTIFY_ATTRIBUTE_WRITE, /* gattEvtMask */
                                     10, /* encryKeySize */
                                     CHAR_VALUE_LEN_CONSTANT,
                                     &(HIDS_Context[service_instance].HidProtocolModeCharHdle));
    if (hciCmdResult == BLE_STATUS_SUCCESS)
    {
      BLE_DBG_HIDS_MSG ("Protocol Mode Characteristic is added Successfully %04X\n", 
                        HIDS_Context[service_instance].HidProtocolModeCharHdle);
    }
    else
    {
      BLE_DBG_HIDS_MSG ("FAILED to add Protocol Mode Characteristic, Error: %02X !!\n", 
                        hciCmdResult);
    }
    protocol_mode = BLE_CFG_HIDS_PROTOCOL_MODE;
    hciCmdResult = aci_gatt_update_char_value(HIDS_Context[service_instance].HidSvcHdle, 
                                              HIDS_Context[service_instance].HidProtocolModeCharHdle, 
                                              (uint8_t)0,
                                              (uint8_t)1, 
                                              (uint8_t *)&protocol_mode);
    
    if (hciCmdResult == BLE_STATUS_SUCCESS)
    {
      BLE_DBG_HIDS_MSG ("Protocol mode charact update is Successfully %04X\n", 
                        HIDS_Context[service_instance].HidProtocolModeCharHdle);
    }
    else 
    {
      BLE_DBG_HIDS_MSG ("FAILED to update Protocol mode charact, Error: %02X !!\n", 
                        hciCmdResult);
    }   
#endif

#if (BLE_CFG_HIDS_REPORT_CHAR != 0)
    /**
     *  Add Report Characteristic
     */
    for(i = 0; 
        i < (BLE_CFG_HIDS_INPUT_REPORT_NB + BLE_CFG_HIDS_OUTPUT_REPORT_NB + BLE_CFG_HIDS_FEATURE_REPORT_NB); 
        i++)
    {      
      if(i < BLE_CFG_HIDS_INPUT_REPORT_NB)
      {        
        uint8_t buf[2] = {0,0};
        
        uuid = REPORT_CHAR_UUID;
        hciCmdResult = aci_gatt_add_char(HIDS_Context[service_instance].HidSvcHdle,
                                         UUID_TYPE_16,
                                         (Char_UUID_t *) &uuid ,
                                         BLE_CFG_HIDS_REPORT_MAX_LEN, /* bytes */
                                         CHAR_PROP_READ | CHAR_PROP_NOTIFY
#if (BLE_CFG_HIDS_INPUT_WRITE != 0)
                                         | CHAR_PROP_WRITE,
#else
                                         ,
#endif
                                         ATTR_PERMISSION_NONE,
#if (BLE_CFG_HIDS_INPUT_WRITE != 0)
                                         GATT_NOTIFY_WRITE_REQ_AND_WAIT_FOR_APPL_RESP,
#else
                                         GATT_DONT_NOTIFY_EVENTS, /* gattEvtMask */
#endif
                                         10, /* encryKeySize */
                                         CHAR_VALUE_LEN_VARIABLE,
                                         &(HIDS_Context[service_instance].HidReportCharHdle[i]));
        if (hciCmdResult == BLE_STATUS_SUCCESS)
        {
          BLE_DBG_HIDS_MSG ("Input Report Characteristic is added Successfully 0x%04X\n", 
                            HIDS_Context[service_instance].HidReportCharHdle[i]);
        }
        else
        {
          BLE_DBG_HIDS_MSG ("FAILED to add Input Report Characteristic, Error: 0x%02X !!\n", 
                            hciCmdResult);
        }
        
        uuid = REPORT_REFERENCE_DESCRIPTOR_UUID;
        buf[0] = i + 1; /* keyword: report IDs start at 1, ID 0 is reserved when a map has several reports */
        buf[1] = 1; /* Input Report */
        /* add the valid descriptor */
        hciCmdResult = aci_gatt_add_char_desc(HIDS_Context[service_instance].HidSvcHdle, 
                                              HIDS_Context[service_instance].HidReportCharHdle[i], 
                                              UUID_TYPE_16,
                                              (Char_Desc_Uuid_t *)&uuid, 
                                              BLE_CFG_HIDS_REPORT_REFERENCE_LEN,
                                              BLE_CFG_HIDS_REPORT_REFERENCE_LEN,
                                              (void *)&buf,
                                              ATTR_PERMISSION_NONE,
                                              ATTR_ACCESS_READ_ONLY,
                                              GATT_DONT_NOTIFY_EVENTS,
                                              MIN_ENCRY_KEY_SIZE, 
                                              CHAR_VALUE_LEN_CONSTANT,
                                              &HIDS_Context[service_instance].HidReportReferenceDescHdle[i]);
        if (hciCmdResult == BLE_STATUS_SUCCESS)
        {
          BLE_DBG_HIDS_MSG ("Input Report Reference Descriptor is added Successfully 0x%04X\n", 
                            HIDS_Context[service_instance].HidReportReferenceDescHdle[i]);
        }
        else
        {
          BLE_DBG_HIDS_MSG ("FAILED to add Input Report Reference Descriptor, Error: 0x%02X !!\n", 
                            hciCmdResult);
        }
      }
      else if((i - BLE_CFG_HIDS_INPUT_REPORT_NB) < BLE_CFG_HIDS_OUTPUT_REPORT_NB)
      {
        uint8_t buf[2] = {0,0};
        
        uuid = REPORT_CHAR_UUID;
        hciCmdResult = aci_gatt_add_char(HIDS_Context[service_instance].HidSvcHdle,
                                         UUID_TYPE_16,
                                         (Char_UUID_t *) &uuid ,
                                         BLE_CFG_HIDS_REPORT_MAX_LEN, /* bytes */
                                         CHAR_PROP_READ | CHAR_PROP_WRITE | CHAR_PROP_WRITE_WITHOUT_RESP,
                                         ATTR_PERMISSION_NONE,
                                         GATT_NOTIFY_WRITE_REQ_AND_WAIT_FOR_APPL_RESP, /* gattEvtMask */
                                         10, /* encryKeySize */
                                         CHAR_VALUE_LEN_VARIABLE,
                                         &(HIDS_Context[service_instance].HidReportCharHdle[i]));
        if (hciCmdResult == BLE_STATUS_SUCCESS)
        {
          BLE_DBG_HIDS_MSG ("Output Report Characteristic is added Successfully 0x%04X\n", 
                            HIDS_Context[service_instance].HidReportCharHdle[i]);
        }
        else
        {
          BLE_DBG_HIDS_MSG ("FAILED to add Output Report Characteristic, Error: 0x%02X !!\n", 
                            hciCmdResult);
        }

        uuid = REPORT_REFERENCE_DESCRIPTOR_UUID;
        buf[0] = i - BLE_CFG_HIDS_INPUT_REPORT_NB;
        buf[1] = 2; /* Output Report */
        /* add the valid descriptor */
        hciCmdResult = aci_gatt_add_char_desc(HIDS_Context[service_instance].HidSvcHdle, 
                                              HIDS_Context[service_instance].HidReportCharHdle[i], 
                                              UUID_TYPE_16,
                                              (Char_Desc_Uuid_t *)&uuid, 
                                              BLE_CFG_HIDS_REPORT_REFERENCE_LEN,
                                              BLE_CFG_HIDS_REPORT_REFERENCE_LEN,
                                              (void *)&buf,
                                              ATTR_PERMISSION_NONE,
                                              ATTR_ACCESS_READ_ONLY,
                                              GATT_DONT_NOTIFY_EVENTS,
                                              MIN_ENCRY_KEY_SIZE, 
                                              CHAR_VALUE_LEN_CONSTANT,
                                              &HIDS_Context[service_instance].HidReportReferenceDescHdle[i]);
        if (hciCmdResult == BLE_STATUS_SUCCESS)
        {
          BLE_DBG_HIDS_MSG ("Output Report Reference Descriptor is added Successfully 0x%04X\n", 
                            HIDS_Context[service_instance].HidReportReferenceDescHdle[i]);
        }
        else
        {
          BLE_DBG_HIDS_MSG ("FAILED to add Output Report Reference Descriptor, Error: 0x%02X !!\n", 
                            hciCmdResult);
        }
      }
      else if((i - BLE_CFG_HIDS_INPUT_REPORT_NB - BLE_CFG_HIDS_OUTPUT_REPORT_NB) < BLE_CFG_HIDS_FEATURE_REPORT_NB)
      {
        uint8_t buf[2] = {0,0};
        
        uuid = REPORT_CHAR_UUID;
        hciCmdResult = aci_gatt_add_char(HIDS_Context[service_instance].HidSvcHdle,
                                         UUID_TYPE_16,
                                         (Char_UUID_t *) &uuid ,
                                         BLE_CFG_HIDS_REPORT_MAX_LEN, /* bytes */
                                         CHAR_PROP_READ | CHAR_PROP_WRITE,
                                         ATTR_PERMISSION_NONE,
                                         GATT_NOTIFY_WRITE_REQ_AND_WAIT_FOR_APPL_RESP, /* gattEvtMask */
                                         10, /* encryKeySize */
                                         CHAR_VALUE_LEN_VARIABLE,
                                         &(HIDS_Context[service_instance].HidReportCharHdle[i]));
        if (hciCmdResult == BLE_STATUS_SUCCESS)
        {
          BLE_DBG_HIDS_MSG ("Feature Report Characteristic is added Successfully 0x%04X\n", 
                            HIDS_Context[service_instance].HidReportReferenceDescHdle[i]);
        }
        else
        {
          BLE_DBG_HIDS_MSG ("FAILED to add Feature Report Characteristic, Error: 0x%02X !!\n", 
                            hciCmdResult);
        }

        uuid = REPORT_REFERENCE_DESCRIPTOR_UUID;
        buf[0] = i - BLE_CFG_HIDS_INPUT_REPORT_NB - BLE_CFG_HIDS_OUTPUT_REPORT_NB;
        buf[1] = 3; /* Feature Report */
        /* add the valid descriptor */
        hciCmdResult = aci_gatt_add_char_desc(HIDS_Context[service_instance].HidSvcHdle, 
                                              HIDS_Context[service_instance].HidReportCharHdle[i], 
                                              UUID_TYPE_16,
                                              (Char_Desc_Uuid_t*)&uuid, 
                                              BLE_CFG_HIDS_REPORT_REFERENCE_LEN,
                                              BLE_CFG_HIDS_REPORT_REFERENCE_LEN,
                                              (void *)&buf,
                                              ATTR_PERMISSION_NONE,
                                              ATTR_ACCESS_READ_ONLY,
                                              GATT_DONT_NOTIFY_EVENTS,
                                              MIN_ENCRY_KEY_SIZE, 
                                              CHAR_VALUE_LEN_CONSTANT,
                                              &HIDS_Context[service_instance].HidReportReferenceDescHdle[i]);
        if (hciCmdResult == BLE_STATUS_SUCCESS)
        {
          BLE_DBG_HIDS_MSG ("Feature Report Reference Descriptor is added Successfully 0x%04X\n", 
                            HIDS_Context[service_instance].HidReportReferenceDescHdle[i]);
        }
        else
        {
          BLE_DBG_HIDS_MSG ("FAILED to add Feature Report Reference Descriptor, Error: 0x%02X !!\n", 
                            hciCmdResult);
        }
      }
    }
#endif

    /**
     *  Add Report Map Characteristic
     */
    uuid = REPORT_MAP_CHAR_UUID;
    hciCmdResult = aci_gatt_add_char(HIDS_Context[service_instance].HidSvcHdle,
                                     UUID_TYPE_16,
                                     (Char_UUID_t *) &uuid ,
                                     BLE_CFG_HIDS_REPORT_MAP_MAX_LEN,
                                     CHAR_PROP_READ,
                                     ATTR_PERMISSION_ENCRY_READ,
                                     GATT_DONT_NOTIFY_EVENTS, /* gattEvtMask */
                                     10, /* encryKeySize */
                                     CHAR_VALUE_LEN_VARIABLE,
                                     &(HIDS_Context[service_instance].HidReportMapCharHdle));
    if (hciCmdResult == BLE_STATUS_SUCCESS)
    {
      BLE_DBG_HIDS_MSG ("Report Map Characteristic is added Successfully 0x%04X\n", 
                        HIDS_Context[service_instance].HidReportMapCharHdle);
    }
    else
    {
      BLE_DBG_HIDS_MSG ("FAILED to add Report Map Characteristic, Error: 0x%02X !!\n", 
                        hciCmdResult);
    }

#if (BLE_CFG_HIDS_EXTERNAL_REPORT_REFERENCE != 0)
    {
      uint8_t buf[2] = {0,0};
      
      uuid = EXTERNAL_REPORT_REFERENCE_DESCRIPTOR_UUID;
      STORE_LE_16(buf, 0);    /* Characteristic UUID  */
      /* add the valid descriptor */
      hciCmdResult = aci_gatt_add_char_desc(HIDS_Context[service_instance].HidSvcHdle, 
                                            HIDS_Context[service_instance].HidReportMapCharHdle, 
                                            UUID_TYPE_16,
                                            (Char_Desc_Uuid_t *)&uuid, 
                                            BLE_CFG_HIDS_EXTERNAL_REPORT_REFERENCE_LEN,
                                            BLE_CFG_HIDS_EXTERNAL_REPORT_REFERENCE_LEN,
                                            (void *)&buf,
                                            ATTR_PERMISSION_NONE,
                                            ATTR_ACCESS_READ_ONLY,
                                            GATT_DONT_NOTIFY_EVENTS,
                                            MIN_ENCRY_KEY_SIZE, 
                                            CHAR_VALUE_LEN_CONSTANT,
                                            &HIDS_Context[service_instance].HidExternalReferenceDescHdle);
      if (hciCmdResult == BLE_STATUS_SUCCESS)
      {
        BLE_DBG_HIDS_MSG ("External Report Reference Descriptor is added Successfully 0x%04X\n", 
                          HIDS_Context[service_instance].HidExternalReferenceDescHdle);
      }
      else
      {
        BLE_DBG_HIDS_MSG ("FAILED to add External Report Reference Descriptor, Error: 0x%02X !!\n", 
                          hciCmdResult);
      }
    }
#endif

#if (BLE_CFG_HIDS_KEYBOARD_DEVICE != 0)
    /**
     *  Add Boot Keyboard Input Report Characteristic
     */
    uuid = BOOT_KEYBOARD_INPUT_REPORT_CHAR_UUID;
    hciCmdResult = aci_gatt_add_char(HIDS_Context[service_instance].HidSvcHdle,
                                     UUID_TYPE_16,
                                     (Char_UUID_t *) &uuid ,
                                     BLE_CFG_HIDS_BOOT_KEYBOARD_INPUT_REPORT_MAX_LEN,
                                     CHAR_PROP_READ | CHAR_PROP_NOTIFY
#if (BLE_CFG_HIDS_KEYBOARD_INPUT_WRITE != 0)
                                     | CHAR_PROP_WRITE,
#else
                                     ,
#endif
                                     ATTR_PERMISSION_NONE,
                                     GATT_NOTIFY_WRITE_REQ_AND_WAIT_FOR_APPL_RESP, /* gattEvtMask */
                                     10, /* encryKeySize */
                                     CHAR_VALUE_LEN_VARIABLE,
                                     &(HIDS_Context[service_instance].HidKeyboardReportInputCharHdle));
    if (hciCmdResult == BLE_STATUS_SUCCESS)
    {
      BLE_DBG_HIDS_MSG ("Boot Keyboard Input Report Characteristic is added Successfully 0x%04X\n", 
                        HIDS_Context[service_instance].HidKeyboardReportInputCharHdle);
    }
    else
    {
      BLE_DBG_HIDS_MSG ("FAILED to add Boot Keyboard Input Report Characteristic, Error: 0x%02X !!\n", 
                        hciCmdResult);
    }

    /**
     *  Add Boot Keyboard Output Report Characteristic
     */
    uuid = BOOT_KEYBOARD_OUTPUT_REPORT_CHAR_UUID;
    hciCmdResult = aci_gatt_add_char(HIDS_Context[service_instance].HidSvcHdle,
                                     UUID_TYPE_16,
                                     (Char_UUID_t *) &uuid ,
                                     BLE_CFG_HIDS_BOOT_KEYBOARD_OUTPUT_REPORT_MAX_LEN,
                                     CHAR_PROP_READ | CHAR_PROP_WRITE | CHAR_PROP_WRITE_WITHOUT_RESP,
                                     ATTR_PERMISSION_NONE,
                                     GATT_NOTIFY_WRITE_REQ_AND_WAIT_FOR_APPL_RESP, /* gattEvtMask */
                                     10, /* encryKeySize */
                                     CHAR_VALUE_LEN_VARIABLE,
                                     &(HIDS_Context[service_instance].HidKeyboardReportOutputCharHdle));
    if (hciCmdResult == BLE_STATUS_SUCCESS)
    {
      BLE_DBG_HIDS_MSG ("Boot Keyboard Output Report Characteristic is added Successfully 0x%04X\n", 
                        HIDS_Context[service_instance].HidKeyboardReportOutputCharHdle);
    }
    else
    {
      BLE_DBG_HIDS_MSG ("FAILED to add Boot Keyboard Output Report Characteristic, Error: 0x%02X !!\n", 
                        hciCmdResult);
    }
#endif
#if (BLE_CFG_HIDS_MOUSE_DEVICE != 0) 
    /**
     *  Add Boot Mouse Input Report Characteristic
     */
    uuid = BOOT_MOUSE_INPUT_REPORT_CHAR_UUID;
    hciCmdResult = aci_gatt_add_char(HIDS_Context[service_instance].HidSvcHdle,
                                     UUID_TYPE_16,
                                     (Char_UUID_t *) &uuid ,
                                     BLE_CFG_HIDS_BOOT_MOUSE_INPUT_REPORT_MAX_LEN,
                                     CHAR_PROP_READ | CHAR_PROP_NOTIFY
#if (BLE_CFG_HIDS_MOUSE_INPUT_WRITE != 0)
                                     | CHAR_PROP_WRITE,
#else
                                     ,
#endif
                                     ATTR_PERMISSION_NONE,
#if (BLE_CFG_HIDS_MOUSE_INPUT_WRITE != 0)
                                     GATT_NOTIFY_WRITE_REQ_AND_WAIT_FOR_APPL_RESP,
#else
                                     GATT_DONT_NOTIFY_EVENTS, /* gattEvtMask */
#endif
                                     10, /* encryKeySize */
                                     CHAR_VALUE_LEN_VARIABLE,
                                     &(HIDS_Context[service_instance].HidMouseReportInputCharHdle));
    if (hciCmdResult == BLE_STATUS_SUCCESS)
    {
      BLE_DBG_HIDS_MSG ("Boot Mouse Input Report Characteristic is added Successfully 0x%04X\n", 
                        HIDS_Context[service_instance].HidMouseReportInputCharHdle);
    }
    else
    {
      BLE_DBG_HIDS_MSG ("FAILED to add Boot Mouse Input Report Characteristic, Error: 0x%02X !!\n", 
                        hciCmdResult);
    }
#endif

    uuid = HID_INFORMATION_CHAR_UUID;
    hciCmdResult = aci_gatt_add_char(HIDS_Context[service_instance].HidSvcHdle,
                                     UUID_TYPE_16,
                                     (Char_UUID_t *) &uuid ,
                                     BLE_CFG_HIDS_INFORMATION_LEN, /* bytes */
                                     CHAR_PROP_READ,
                                     ATTR_PERMISSION_NONE,
                                     GATT_DONT_NOTIFY_EVENTS, /* gattEvtMask */
                                     10, /* encryKeySize */
                                     CHAR_VALUE_LEN_CONSTANT,
                                     &(HIDS_Context[service_instance].HidInformationCharHdle));
    if (hciCmdResult == BLE_STATUS_SUCCESS)
    {
      BLE_DBG_HIDS_MSG ("HID Information Characteristic is added Successfully 0x%04X\n", 
                        HIDS_Context[service_instance].HidInformationCharHdle);
    }
    else
    {
      BLE_DBG_HIDS_MSG ("FAILED to add HID Information Characteristic, Error: 0x%02X !!\n", 
                        hciCmdResult);
    }

    uuid = HID_CONTROL_POINT_CHAR_UUID;
    hciCmdResult = aci_gatt_add_char(HIDS_Context[service_instance].HidSvcHdle,
                                     UUID_TYPE_16,
                                     (Char_UUID_t *) &uuid ,
                                     BLE_CFG_HIDS_CONTROL_POINT_LEN, /* bytes */
                                     CHAR_PROP_WRITE_WITHOUT_RESP,
                                     ATTR_PERMISSION_NONE,
                                     GATT_NOTIFY_ATTRIBUTE_WRITE, /* gattEvtMask */
                                     10, /* encryKeySize */
                                     CHAR_VALUE_LEN_CONSTANT,
                                     &(HIDS_Context[service_instance].HidControlPointCharHdle));
    if (hciCmdResult == BLE_STATUS_SUCCESS)
    {
      BLE_DBG_HIDS_MSG ("HID Control Point Characteristic is added Successfully 0x%04X\n", 
                        HIDS_Context[service_instance].HidControlPointCharHdle);
    }
    else
    {
      BLE_DBG_HIDS_MSG ("FAILED to add HID Control Point Characteristic, Error: 0x%02X !!\n", 
                        hciCmdResult);
    }

  }

  return;
}

/**
 * @brief  Characteristic update
 * @param  UUID: UUID of the characteristic
 * @param  service_instance: Instance of the service to which the characteristic belongs
 * @retval BodySensorLocationValue: The new value to be written
 */
tBleStatus HIDS_Update_Char(uint16_t UUID, 
                            uint8_t service_instance, 
                            uint8_t Report_Index, 
                            uint8_t report_size,
                            uint8_t *pPayload)
{
  tBleStatus return_value = BLE_STATUS_FAILED;
  switch(UUID)
  {
#if (BLE_CFG_HIDS_REPORT_CHAR != 0)
    case REPORT_CHAR_UUID:
      {
        return_value = aci_gatt_update_char_value(HIDS_Context[service_instance].HidSvcHdle,
                                                  HIDS_Context[service_instance].HidReportCharHdle[Report_Index],
                                                  0, /* charValOffset */
                                                  report_size,
                                                  pPayload);
      }
      break;
#endif
    case REPORT_MAP_CHAR_UUID:
      {
        return_value = aci_gatt_update_char_value(HIDS_Context[service_instance].HidSvcHdle,
                                                  HIDS_Context[service_instance].HidReportMapCharHdle,
                                                  0, /* charValOffset */
                                                  report_size,
                                                  pPayload);
      }
      break;
#if (BLE_CFG_HIDS_KEYBOARD_DEVICE != 0)          
    case BOOT_KEYBOARD_INPUT_REPORT_CHAR_UUID:
      {
        return_value = aci_gatt_update_char_value(HIDS_Context[service_instance].HidSvcHdle,
                                                  HIDS_Context[service_instance].HidKeyboardReportInputCharHdle,
                                                  0, /* charValOffset */
                                                  report_size,
                                                  pPayload);
      }
      break;
#endif
#if (BLE_CFG_HIDS_MOUSE_DEVICE != 0)          
    case BOOT_MOUSE_INPUT_REPORT_CHAR_UUID:
      {
        return_value = aci_gatt_update_char_value(HIDS_Context[service_instance].HidSvcHdle,
                                                  HIDS_Context[service_instance].HidMouseReportInputCharHdle,
                                                  0, /* charValOffset */
                                                  report_size,
                                                  pPayload);
      }
      break;
#endif

    case HID_INFORMATION_CHAR_UUID:
      {
        return_value = aci_gatt_update_char_value(HIDS_Context[service_instance].HidSvcHdle,
                                                  HIDS_Context[service_instance].HidInformationCharHdle,
                                                  0, /* charValOffset */
                                                  report_size,
                                                  pPayload);
      }
      break;
    default:
      break;
  }

  return return_value;
}/* end HIDS_Update_Char() */



