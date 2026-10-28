/* Host build stand-in: keybit_bus.h only needs the I2C handle type. */
#ifndef STM32WBXX_HAL_H
#define STM32WBXX_HAL_H
typedef struct { int unused; } I2C_HandleTypeDef;
#endif
