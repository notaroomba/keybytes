/**
 ******************************************************************************
 * @file    uuid.h.h
 * @author  MCD Application Team
 * @brief   Header containing the UUIDs of all the services and caharcteristics
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



#ifndef _UUID_H_
#define _UUID_H_

/* Descriptor UUIDs */
#define CHAR_EXTENDED_PROPERTIES_DESCRIPTOR_UUID                       (0x2900)
#define CHAR_USER_DESCRIPTION_DESCRIPTOR_UUID                          (0x2901)
#define CLIENT_CHAR_CONFIG_DESCRIPTOR_UUID                             (0x2902)
#define SERVER_CHAR_CONFIG_DESCRIPTOR_UUID                             (0x2903)
#define CHAR_PRESENTATION_FORMAT_DESCRIPTOR_UUID                       (0x2904)
#define CHAR_AGGREGATE_FORMAT_DESCRIPTOR_UUID                          (0x2905)
#define VALID_RANGE_DESCRIPTOR_UUID                                    (0x2906)
#define EXTERNAL_REPORT_REFERENCE_DESCRIPTOR_UUID                      (0x2907)
#define REPORT_REFERENCE_DESCRIPTOR_UUID                               (0x2908)
#define NUMBER_OF_DIGITALS_DESCRIPTOR_UUID                             (0x2909)
#define VALUE_TRIGGER_SETTING_DESCRIPTOR_UUID                          (0x290A) 
#define ES_CONFIGURATION_DESCRIPTOR_UUID                               (0x290B)
#define ES_MEASUREMENT_DESCRIPTOR_UUID                                 (0x290C)
#define ES_TRIGGER_SETTING_DESCRIPTOR_UUID                             (0x290D)
#define TIME_TRIGGER_SETTING_DESCRIPTOR_UUID                           (0x290E) 

/* UUIDs of Generic Attribute service */
#define GENERIC_ATTRIBUTE_SERVICE_UUID                                 (0x1801)
#define SERVICE_CHANGED_CHARACTERISTIC_UUID                            (0x2A05)

/* UUIDs of immediate alert service */
#define IMMEDIATE_ALERT_SERVICE_UUID                                   (0x1802)
#define ALERT_LEVEL_CHARACTERISTIC_UUID                                (0x2A06)

/* UUIDs for Link Loss service */
#define LINK_LOSS_SERVICE_UUID                                         (0x1803)
#define LINK_LOSS_ALERT_LEVEL_CHARACTERISTIC_UUID                      (0x2A06)

/* UUIDs for TX Power service */
#define TX_POWER_SERVICE_UUID                                          (0x1804)
#define TX_POWER_LEVEL_CHARACTERISTIC_UUID                             (0x2A07)

/* UUIDs for Time service */
#define CURRENT_TIME_SERVICE_UUID                                      (0x1805)
#define CURRENT_TIME_CHAR_UUID                                         (0x2A2B)
#define LOCAL_TIME_INFORMATION_CHAR_UUID                               (0x2A0F)
#define REFERENCE_TIME_INFORMATION_CHAR_UUID                           (0x2A14)

/* UUIDs for Reference Time Update service */
#define REFERENCE_UPDATE_TIME_SERVICE_UUID                             (0x1806)
#define TIME_UPDATE_CONTROL_POINT_CHAR_UUID                            (0x2A16)
#define TIME_UPDATE_STATE_CHAR_UUID                                    (0x2A17)

/* UUIDs for Next DST Change service */
#define NEXT_DST_CHANGE_SERVICE_UUID                                   (0x1807)
#define TIME_WITH_DST_CHAR_UUID                                        (0x2A11)

/* UUIDs for glucose profile */
#define GLUCOSE_SERVICE_UUID                                           (0x1808)
#define GLUCOSE_MEASUREMENT_CHAR_UUID                                  (0x2A18)
#define GLUCOSE_MEASUREMENT_CONTEXT_CHAR_UUID                          (0x2A34)
#define GLUCOSE_FEATURE_CHAR_UUID                                      (0x2A51)
/* Record Access Control Point (RACP) */
#define GLUCOSE_RACP_CHAR_UUID                                         (0x2A52)

/* UUIDs for health thermometer profile */
#define HEALTH_THERMOMETER_SERVICE_UUID                                (0x1809)
#define TEMPERATURE_MEASUREMENT_CHAR_UUID                              (0x2A1C)
#define TEMPERATURE_TYPE_CHAR_UUID                                     (0x2A1D)
#define INTERMEDIATE_TEMPERATURE_CHAR_UUID                             (0x2A1E)
#define MEASUREMENT_INTERVAL_CHAR_UUID                                 (0x2A21)

/* UUIDs for Device Information Service */
#define DEVICE_INFORMATION_SERVICE_UUID                                (0x180A)
#define SYSTEM_ID_UUID                                                 (0x2A23)
#define MODEL_NUMBER_UUID                                              (0x2A24)
#define SERIAL_NUMBER_UUID                                             (0x2A25)
#define FIRMWARE_REVISION_UUID                                         (0x2A26)
#define HARDWARE_REVISION_UUID                                         (0x2A27)
#define SOFTWARE_REVISION_UUID                                         (0x2A28)
#define MANUFACTURER_NAME_UUID                                         (0x2A29)
#define IEEE_CERTIFICATION_UUID                                        (0x2A2A)
#define PNP_ID_UUID                                                    (0x2A50)

/* UUIDs for Heart Rate Service */
#define HEART_RATE_SERVICE_UUID                                        (0x180D)
#define CLIENT_CHARACTERISTIC_CONFIG_DESCRIPTOR_UUID                   (0x2902)
#define HEART_RATE_MEASURMENT_UUID                                     (0x2A37)
#define SENSOR_LOCATION_UUID                                           (0x2A38)
#define CONTROL_POINT_UUID                                             (0x2A39)

/* UUIDs for Phone Alert status profile */
#define PHONE_ALERT_SERVICE_UUID                                       (0x180E)
#define PHONE_ALERT_STATUS_CHARAC_UUID                                 (0x2A3F)
#define RINGER_CNTRL_POINT_CHARAC_UUID                                 (0x2A40)
#define RINGER_SETTING_CHARAC_UUID                                     (0x2A41)

/* UUIDs for battery service */ 
#define BATTERY_SERVICE_UUID                                           (0x180F)
#define BATTERY_LEVEL_CHAR_UUID                                        (0x2A19)

/* UUIDs for Blood Pressure profile */
#define BLOOD_PRESSURE_SERVICE_UUID                                    (0x1810)
#define BLOOD_PRESSURE_MEASUREMENT_CHAR_UUID                           (0x2A35)
#define INTERMEDIATE_CUFF_PRESSURE_CHAR_UUID                           (0x2A36)
#define BLOOD_PRESSURE_FEATURE_CHAR_UUID                               (0x2A49)

/* UUIDs for alert notification profile */
#define ALERT_NOTIFICATION_SERVICE_UUID                                (0x1811)
#define SUPPORTED_NEW_ALERT_CATEGORY_CHAR_UUID                         (0x2A47)
#define NEW_ALERT_CHAR_UUID                                            (0x2A46)
#define SUPPORTED_UNREAD_ALERT_CATEGORY_CHAR_UUID                      (0x2A48)
#define UNREAD_ALERT_STATUS_CHAR_UUID                                  (0x2A45)
#define ALERT_NOTIFICATION_CONTROL_POINT_CHAR_UUID                     (0x2A44)

/* UUIDs for human interface device */
#define HUMAN_INTERFACE_DEVICE_SERVICE_UUID                            (0x1812)
#define PROTOCOL_MODE_CHAR_UUID                                        (0x2A4E)
#define REPORT_CHAR_UUID                                               (0x2A4D)
#define REPORT_MAP_CHAR_UUID                                           (0x2A4B)
#define BOOT_KEYBOARD_INPUT_REPORT_CHAR_UUID                           (0x2A22)
#define BOOT_KEYBOARD_OUTPUT_REPORT_CHAR_UUID                          (0x2A32)
#define BOOT_MOUSE_INPUT_REPORT_CHAR_UUID                              (0x2A33)
#define HID_INFORMATION_CHAR_UUID                                      (0x2A4A)
#define HID_CONTROL_POINT_CHAR_UUID                                    (0x2A4C)

/* UUIDs for scan parameter service */
#define SCAN_PARAMETER_SERVICE_UUID                                    (0x1813)
#define SCAN_INTERVAL_WINDOW_CHAR_UUID                                 (0x2A4F)
#define SCAN_REFRESH_CHAR_UUID                                         (0x2A31)

/* UUIDs for running speed and cadence service */
#define RUNNING_SPEED_CADENCE_SERVICE_UUID                             (0x1814)
#define RUNNING_SPEED_CADENCE_MEASUREMENT_CHAR_UUID                    (0x2A53)
#define RUNNING_SPEED_CADENCE_FEATURE_CHAR_UUID                        (0x2A54)

/* UUIDs for automation IO service */
#define AUTOMATION_IO_SERVICE_UUID                                     (0x1815)
#define AUTOMATION_IO_DIGITAL_CHAR_UUID                                (0x2A56)
#define AUTOMATION_IO_ANALOG_CHAR_UUID                                 (0x2A58)
#define AUTOMATION_IO_AGGREGATE_CHAR_UUID                              (0x2A5A)

/* UUIDs for cycling speed and cadence service */
#define CYCLING_SPEED_CADENCE_SERVICE_UUID                             (0x1816)
#define CYCLING_SPEED_CADENCE_MEASUREMENT_CHAR_UUID                    (0x2A5B)
#define CYCLING_SPEED_CADENCE_FEATURE_CHAR_UUID                        (0x2A5C)

/* UUIDs for cycling power service */
#define CYCLING_POWER_SERVICE_UUID                                     (0x1818)
#define CYCLING_POWER_MEASUREMENT_CHAR_UUID                            (0x2A63)
#define CYCLING_POWER_FEATURE_CHAR_UUID                                (0x2A65)
#define CYCLING_POWER_SENSOR_LOCATION_CHAR_UUID                        (0x2A5D)

/* UUIDs for location and navigation device */
#define LOCATION_NAVIGATION_SERVICE_UUID                               (0x1819)
#define LN_FEATURE_UUID                                                (0x2A6A)
#define LOCATION_SPEED_UUID                                            (0x2A67)
#define POSITION_QUALITY_UUID                                          (0x2A69)
#define LN_CONTROL_POINT_UUID                                          (0x2A6B)
#define NAVIGATION_UUID                                                (0x2A68)

/* UUIDs for environmental sensing profile */
#define ENVIRONMENTAL_SENSING_SERVICE_UUID                             (0x181A)
#define DESCRIPTOR_VALUE_CHANGED_UUID                                  (0x2A7D)
#define APPARENT_WIND_DIRECTION_UUID                                   (0x2A73)
#define APPARENT_WIND_SPEED_UUID                                       (0x2A72)
#define DEW_POINT_UUID                                                 (0x2A7B)
#define ELEVATION_UUID                                                 (0x2A6C)
#define GUST_FACTOR_UUID                                               (0x2A74)
#define HEAT_INDEX_UUID                                                (0x2A7A)
#define HUMIDITY_UUID                                                  (0x2A6F)
#define IRRADIANCE_UUID                                                (0x2A77)
#define POLLEN_CONCENTRATION_UUID                                      (0x2A75)
#define RAINFALL_UUID                                                  (0x2A78)
#define PRESSURE_UUID                                                  (0x2A6D)
#define TEMPERATURE_UUID                                               (0x2A6E)
#define TRUE_WIND_DIRECTION_UUID                                       (0x2A71)
#define TRUE_WIND_SPEED_UUID                                           (0x2A70)
#define UV_INDEX_UUID                                                  (0x2A76)
#define WIND_CHILL_UUID                                                (0x2A79)
#define BAROMETRIC_PRESSURE_TREND_UUID                                 (0x2AA3)
#define MAGNETIC_DECLINATION_UUID                                      (0x2A2C)
#define MAGNETIC_FLUX_DENSITY_2D_UUID                                  (0x2AA0)
#define MAGNETIC_FLUX_DENSITY_3D_UUID                                  (0x2AA1)
   
/* UUIDs for body composition service */
#define BODY_COMPOSITION_SERVICE_UUID                                  (0x181B)
#define BODY_COMPOSITION_MEASUREMENT_CHAR_UUID                         (0x2A9C)
#define BODY_COMPOSITION_FEATURE_CHARAC                                (0x2A9B)      
   
/* UUIDs for user data service */
#define USER_DATA_SERVICE_UUID                                         (0x181C)
#define AERO_HR_LOWER_LIMIT_CHAR_UUID                                  (0x2A7E)
#define AEROBIC_THRESHOLD_CHAR_UUID                                    (0x2A7F)
#define AGE_CHAR_UUID                                                  (0x2A80)
#define ANAERO_HR_LOWER_LIMIT_CHAR_UUID                                (0x2A81)
#define ANAERO_HR_UPPER_LIMIT_CHAR_UUID                                (0x2A82)
#define ANAEROBIC_THRESHOLD_CHAR_UUID                                  (0x2A83)
#define AERO_HR_UPPER_LIMIT_CHAR_UUID                                  (0x2A84)
#define BIRTH_DATE_CHAR_UUID                                           (0x2A85)
#define DATE_THRESHOLD_ASSESSMENT_CHAR_UUID                            (0x2A86)
#define EMAIL_ADDRESS_CHAR_UUID                                        (0x2A87)
#define FAT_BURN_HR_LOWER_LIMIT_CHAR_UUID                              (0x2A88)
#define FAT_BURN_HR_UPPER_LIMIT_CHAR_UUID                              (0x2A89)
#define FIRST_NAME_CHAR_UUID                                           (0x2A8A)
#define FIVE_ZONE_HR_LIMIT_CHAR_UUID                                   (0x2A8B)
#define GENDER_CHAR_UUID                                               (0x2A8C)
#define HEART_RATE_MAX_CHAR_UUID                                       (0x2A8D)
#define HEIGHT_CHAR_UUID                                               (0x2A8E)
#define HIP_CIRC_CHAR_UUID                                             (0x2A8F)
#define LAST_NAME_CHAR_UUID                                            (0x2A90)
#define MAX_RECO_HEART_RATE_CHAR_UUID                                  (0x2A91)
#define RESTING_HEART_RATE_CHAR_UUID                                   (0x2A92)
#define SPORT_TYPE_CHAR_UUID                                           (0x2A93)
#define THREE_ZONE_HR_LIMIT_CHAR_UUID                                  (0x2A94)
#define TWO_ZONE_HR_LIMIT_CHAR_UUID                                    (0x2A95)
#define VO2_MAX_CHAR_UUID                                              (0x2A96)
#define WAIST_CIRC_CHAR_UUID                                           (0x2A97)
#define WEIGHT_CHAR_UUID                                               (0x2A98)
#define DATABASE_CHANGE_INCREMENT_CHAR_UUID                            (0x2A99)
#define USER_INDEX_CHAR_UUID                                           (0x2A9A)
#define USER_CONTROL_POINT_CHAR_UUID                                   (0x2A9F)
#define LANGUAGE_CHAR_UUID                                             (0x2AA2)
   
/* UUIDs for weight scale profile */
#define WEIGHT_SCALE_SERVICE_UUID                                      (0x181D)
#define WEIGHT_SCALE_MEASUREMENT_CHAR_UUID                             (0x2A9D)
#define WEIGHT_SCALE_FEATURE_CHAR_UUID                                 (0x2A9E)  
   
/* UUIDs for weight scale profile */
#define BOND_MANAGEMENT_SERVICE_UUID                                   (0x181E)
#define BM_CONTROL_POINT_CHAR_UUID                                     (0x2AA4)
#define BM_FEATURE_CHAR_UUID                                           (0x2AA5)  
   
/* UUIDs for Internet Support Service */
#define INTERNET_SUPPORT_SERVICE_UUID                                  (0x1820)
   
/* UUIDs for Indoor Positioning Service */
#define INDOOR_POSITIONING_SERVICE_UUID                                (0x1821)
#define IP_CONFIGURATION_CHAR_UUID                                     (0x2AAD)
#define IP_LATITUDE_CHAR_UUID                                          (0x2AAE)  
#define IP_LONGITUDE_CHAR_UUID                                         (0x2AAF)  
   
/* UUIDs for HTTP proxy Service */
#define HTTP_PROXY_SERVICE_UUID                                        (0x1823)
#define HTTP_URI_CHAR_UUID                                             (0x2AB6)
#define HTTP_HEADERS_CHAR_UUID                                         (0x2AB7)
#define HTTP_STATUS_CODE_CHAR_UUID                                     (0x2AB8)
#define HTTP_ENTITY_BODY_CHAR_UUID                                     (0x2AB9)
#define HTTP_CONTROL_POINT_CHAR_UUID                                   (0x2ABA)
#define HTTP_SECURITY_CHAR_UUID                                        (0x2ABB)
   
/* UUIDs for Object Transfer Service */
#define OBJECT_TRANSFER_SERVICE_UUID                                   (0x1825)
#define OTS_FEATURE_CHAR_UUID                                          (0x2ABD)
#define OBJECT_NAME_CHAR_UUID                                          (0x2ABE)
#define OBJECT_TYPE_CHAR_UUID                                          (0x2ABF)
#define OBJECT_SIZE_CHAR_UUID                                          (0x2AC0)
#define OBJECT_PROPERTIES_CHAR_UUID                                    (0x2AC4)
#define OBJECT_ACTION_CONTROL_POINT_CHAR_UUID                          (0x2AC5)
#define OBJECT_LIST_CONTROL_POINT_CHAR_UUID                            (0x2AC6)
   
/* UUIDs for Zigbee Direct Service */
#define ZIGBEE_DIRECT_COMM_SERVICE_UUID                                (0xFFF7)

/* Custom Services*/
/* UUIDs for data transfer service */
#define DT_SERVICE_UUID                                               (0xFE80)
#define DT_TX_CHAR_UUID                                               (0xFE81)
#define DT_RX_CHAR_UUID                                               (0xFE82)
#define DT_THROUGHPUT_CHAR_UUID                                       (0xFE83)

/* UUIDs for custom battery service */ 
#define CUSTOM_BATTERY_SERVICE_UUID                                    (0xF2F0)
#define CUSTOM_BATTERY_LEVEL_CHAR_UUID                                 (0xF2F1)

/* Custom Services*/
/* UUIDs for data transfer service */
#define LED_BUTTON_SERVICE_UUID                                       (0x1A30)
#define LED_CHAR_UUID                                                 (0x2B50)
#define BUTTON_CHAR_UUID                                              (0x2B51)    
/*UUIDs for End Device Management Service*/
#define END_DEVICE_MGT_SERVICE_UUID                                   (0x1A40)
#define END_DEVICE_STATUS_CHAR_UUID                                   (0x2B60)

#define P2P_SERVICE_UUID                                              (0xFE40)
#define P2P_WRITE_CHAR_UUID                                           (0xFE41)
#define P2P_NOTIFY_CHAR_UUID                                          (0xFE42)

#define HOME_SERVICE_UUID                                              (0xFE90)
#define HOME_WRITE_CHAR_UUID                                           (0xFE91)
#define HOME_NOTIFY_CHAR_UUID                                          (0xFE92)

#define CAM_SERVICE_UUID                                              (0xFEA0)
#define CAM_WRITE_CHAR_UUID                                           (0xFEA1)
#define CAM_NOTIFY_CHAR_UUID                                          (0xFEA2)

/* UUIDs for Cable Replacement Service */
#define CRS_SERVICE_UUID                                              (0xFE60)
#define CRS_TX_CHAR_UUID                                              (0xFE61)
#define CRS_RX_CHAR_UUID                                              (0xFE62)

/* UUIDs for Apple Notification Center Service */
#define ANCS_SERVICE_UUID                                             (0xF431) 
#define ANCS_NOTIFICATION_SOURCE_CHAR_UUID                            (0x120D) 
#define ANCS_CONTROL_POINT_CHAR_UUID                                  (0xD8F3) 
#define ANCS_DATA_SOURCE_CHAR_UUID                                    (0xC6E9) 

/* UUIDs for Apple Media Service start from iOS 8*/
#define AMS_SERVICE_UUID                                              (0x502B) 
#define AMS_REMOTE_COMMAND_CHAR_UUID                                  (0x81D8) 
#define AMS_ENTITY_UPDATE_CHAR_UUID                                   (0xABCE) 
#define AMS_ENTITY_ATTRIBUTE_CHAR_UUID                                (0xF38C) 
#endif  /* _UUID_H_ */
