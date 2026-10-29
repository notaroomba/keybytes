/* Stand-in for the STM32 HAL so keybit_bus.c builds on the host. */
#ifndef STM32WBXX_HAL_SIM_H
#define STM32WBXX_HAL_SIM_H

#include <stdint.h>
#include <stddef.h>
#include "sim.h"

typedef enum { HAL_OK, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT } HAL_StatusTypeDef;
typedef enum { GPIO_PIN_RESET, GPIO_PIN_SET } GPIO_PinState;
typedef enum { HAL_I2C_STATE_RESET = 0, HAL_I2C_STATE_READY = 0x20 } HAL_I2C_StateTypeDef;
typedef struct { int unused; } I2C_HandleTypeDef;
typedef struct { int unused; } GPIO_TypeDef;
typedef struct { uint32_t Pin, Mode, Pull, Speed; } GPIO_InitTypeDef;
typedef struct { volatile uint32_t KR; } IWDG_TypeDef;

typedef struct { volatile uint32_t CYCCNT; } DWT_TypeDef;
DWT_TypeDef *sim_dwt(void);
#define DWT (sim_dwt())
uint32_t HAL_RCC_GetHCLKFreq(void);

extern GPIO_TypeDef sim_gpiob;
extern IWDG_TypeDef sim_iwdg;
#define GPIOB (&sim_gpiob)
#define IWDG (&sim_iwdg)
#define GPIO_PIN_13 (1u << 13)
#define GPIO_PIN_14 (1u << 14)
#define GPIO_MODE_OUTPUT_OD 0x11u
#define GPIO_NOPULL 0u
#define GPIO_SPEED_FREQ_LOW 0u
#define I2C_MEMADD_SIZE_8BIT 1u

uint32_t HAL_GetTick(void);
void HAL_Delay(uint32_t ms);
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin);
void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState s);
void HAL_GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *init);
HAL_StatusTypeDef HAL_I2C_Init(I2C_HandleTypeDef *h);
HAL_StatusTypeDef HAL_I2C_DeInit(I2C_HandleTypeDef *h);
HAL_I2C_StateTypeDef HAL_I2C_GetState(I2C_HandleTypeDef *h);
HAL_StatusTypeDef HAL_I2C_IsDeviceReady(I2C_HandleTypeDef *h, uint16_t addr, uint32_t trials, uint32_t timeout);
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *h, uint16_t addr, uint16_t reg, uint16_t regsize,
                                   uint8_t *buf, uint16_t len, uint32_t timeout);
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *h, uint16_t addr, uint16_t reg, uint16_t regsize,
                                    uint8_t *buf, uint16_t len, uint32_t timeout);

#endif
