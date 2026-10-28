/**
  ******************************************************************************
  * @file    opus_interface_stm.h
  * @author  MCD Application Team
  * @brief   This file contains definitions for the opus interface.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __OPUS_INTERFACE_STM_H
#define __OPUS_INTERFACE_STM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "opus.h"
#include "opus_private.h"
#include "ble_gatt_aci.h"
  
/** 
  * @brief Return states values.
  */
typedef enum 
{
  OPUS_IF_SUCCESS = 0x00,                        /*!< Success.*/
  OPUS_IF_ERROR = 0x01,                          /*!< Error.*/
  OPUS_IF_INVALID_PARAM = 0x02,                  /*!< Invalid Opus parameters.*/
  OPUS_IF_ENC_NOT_CONFIG = 0x03,                 /*!< BlueVoiceOPUS encoder not configured.*/
  OPUS_IF_DEC_NOT_CONFIG = 0x04,                 /*!< BlueVoiceOPUS decoder not configured.*/
} OPUS_IF_Status;


/**
 * @brief Opus encoder configuration parameters.
 */
typedef struct
{
  float ms_frame;                                       /*!< audio frame size [ms] (2.5, 5, 10, 20, 40, 60).*/
  
  uint32_t sample_freq;                                 /*!< Audio sampling frequency [Hz] (8000, 12000, 16000, 24000, 48000).*/
   
  uint8_t channels;                                     /*!< Number of audio input channels. */
    
  int application;                                      /*!< Specifies the application type (OPUS_APPLICATION_VOIP, OPUS_APPLICATION_AUDIO, OPUS_APPLICATION_RESTRICTED_LOWDELAY) see opus_define.h. */
 
  uint32_t bitrate;                                     /*!< Specifies the chosen encoding bitrate [bps] (from 6000 to 510000). */
  
  uint8_t complexity;                                   /*!< Specifies the chosen encoding complexity (from 0 to 10). */
    
} OPUS_IF_ENC_ConfigTypeDef;


/**
 * @brief Opus decoder configuration parameters.
 */
typedef struct
{
  float ms_frame;                                       /*!< audio frame size [ms] (2.5, 5, 10, 20, 40, 60). */
   
  uint32_t sample_freq;                                 /*!< Audio sampling frequency [Hz] (8000, 12000, 16000, 24000, 48000). */
   
  uint8_t channels;                                     /*!< Number of audio input channels */

  uint32_t bitrate;                                     /*!< Specifies the chosen encoding bitrate [bps] (from 6000 to 510000). */
   
} OPUS_IF_DEC_ConfigTypeDef;

    
/**
 * @brief  Encoder initialization.
 * @param  Opus encoder configuration.
 * @param  opus_err: @ref opus_errorcodes
 * @retval OPUS_IF_Status: Value indicating success or error (in case of error check opus_err).
 */
OPUS_IF_Status OPUS_IF_ENC_Init(OPUS_IF_ENC_ConfigTypeDef *ENC_configOpus, int *opus_err);

/**
 * @brief  Encoder deinit function.
 * @param  None.
 * @retval None.
 */
void OPUS_IF_ENC_Deinit(void);

/**
  * @brief  This function returns if the the Opus Encoder is configured.
  * @param  None.
  * @retval uint8_t: 1 if the Encoder is configured 0 otherwise.
  */
uint8_t OPUS_IF_ENC_IsConfigured(void);

/**
 * @brief  Decoder initialization.
 * @param  Opus decoder configuration.
 * @param  opus_err: @ref opus_errorcodes
 * @retval OPUS_IF_Status: Value indicating success or error (in case of error check opus_err).
 */
OPUS_IF_Status OPUS_IF_DEC_Init(OPUS_IF_DEC_ConfigTypeDef *DEC_configOpus, int *opus_err);

/**
 * @brief  Decoder deinit function.
 * @param  None.
 * @retval None.
 */
void OPUS_IF_DEC_Deinit(void);

/**
  * @brief  This function returns if the the Opus Decoder is configured.
  * @param  None.
  * @retval uint8_t: 1 if the Decoder is configured 0 otherwise.
  */
uint8_t OPUS_IF_DEC_IsConfigured(void);

/**
 * @brief  Set bitrate to be used for encoding
 * @param  bitrate: Indicate the bitrate in bit per second.
 * @param  opus_err: @ref opus_errorcodes.
 * @retval OPUS_IF_Status: Value indicating success or error (in case of error check opus_err).
 */
OPUS_IF_Status OPUS_IF_ENC_Set_Bitrate(OPUS_IF_ENC_ConfigTypeDef *ENC_configOpus, int bitrate, int *opus_err);

/**
 * @brief  Set constant bitrate option for the encoder.
 * @param  None.
 * @retval OPUS_IF_Status: Value indicating success or error.
 */
OPUS_IF_Status OPUS_IF_ENC_Set_CBR(void);

/**
 * @brief  Set variable bitrate option for the encoder.
 * @param  None.
 * @retval OPUS_IF_Status: Value indicating success or error.
 */
OPUS_IF_Status OPUS_IF_ENC_Set_VBR(void); 

/**
 * @brief  Set complexity to be used for encoding
 * @param  complexity: value from o to 10.
 * @param  opus_err: @ref opus_errorcodes.
 * @retval OPUS_IF_Status: Value indicating success or error (in case of error check opus_err).
 */
OPUS_IF_Status OPUS_IF_ENC_Set_Complexity(OPUS_IF_ENC_ConfigTypeDef *ENC_configOpus, int complexity, int *opus_err);

/**
 * @brief  Force the encoder to use only SILK
 * @param  None.
 * @retval OPUS_IF_Status: Value indicating success or error.
 */
OPUS_IF_Status OPUS_IF_ENC_Force_SILKmode(void);

/**
 * @brief  Force the encoder to use only CELT.
 * @param  None.
 * @retval OPUS_IF_Status: Value indicating success or error.
 */
OPUS_IF_Status OPUS_IF_ENC_Force_CELTmode(void);

/**
 * @brief  Encoding functions.
 * @param  buf_in: pointer to the PCM buffer to be encoded.
 * @param  buf_out: pointer to the Encoded buffer.
 * @retval Number of bytes in case of success, 0 vice-versa.
 */
int OPUS_IF_ENC_Encode(uint8_t * buf_in, uint8_t * buf_out);

/**
 * @brief  Decoding functions.
 * @param  buf_in: pointer to the Encoded buffer to be decoded.
 * @param  len: length of the buffer in.
 * @param  buf_out: pointer to the Decoded buffer.
 * @retval Number of decoded samples or @ref opus_errorcodes.
 */
int OPUS_IF_DEC_Decode(uint8_t * buf_in, uint32_t len, uint8_t * buf_out);


#ifdef __cplusplus
}
#endif

#endif /* __OPUS_INTERFACE_STM_H */


