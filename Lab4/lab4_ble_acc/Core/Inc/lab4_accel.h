#ifndef LAB4_ACCEL_H
#define LAB4_ACCEL_H

#include <stdint.h>
#include "stm32l4xx_hal.h"

/* Call after CubeMX has initialized I2C2. Values are signed milli-g (mg). */
HAL_StatusTypeDef Lab4_Accel_Init(void);
HAL_StatusTypeDef Lab4_Accel_Read(int16_t xyz[3]);
HAL_StatusTypeDef Lab4_Accel_EnableSignificantMotion(void);
HAL_StatusTypeDef Lab4_Accel_ReadMotionSource(uint8_t *detected);

#endif
