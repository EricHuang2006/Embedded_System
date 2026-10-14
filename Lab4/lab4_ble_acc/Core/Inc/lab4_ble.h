#ifndef LAB4_BLE_H
#define LAB4_BLE_H

#include <stdint.h>
#include "bluenrg_def.h"

/* UUIDs in the usual display order; BlueNRG commands use reversed bytes. */
#define LAB4_SERVICE_UUID "a4c00000-7a67-4b5a-9f5b-c3c893bdae40"
#define LAB4_ACCEL_UUID   "a4c00001-7a67-4b5a-9f5b-c3c893bdae40"
#define LAB4_RATE_UUID    "a4c00002-7a67-4b5a-9f5b-c3c893bdae40"
#define LAB4_MOTION_UUID  "a4c00003-7a67-4b5a-9f5b-c3c893bdae40"

#define LAB4_RATE_DEFAULT_HZ 2U
#define LAB4_RATE_MIN_HZ     1U
#define LAB4_RATE_MAX_HZ     20U

/* Initialize once after MX_BlueNRG_MS_Init(), before advertising starts. */
tBleStatus Lab4_BLE_Init(void);
void Lab4_BLE_Process(void);
uint16_t Lab4_BLE_GetSampleRateHz(void);
void Lab4_BLE_HandleWriteRequest(uint16_t connection, uint16_t attribute,
                                uint8_t length, uint8_t *data);
tBleStatus Lab4_BLE_UpdateAcceleration(const int16_t xyz[3]);
tBleStatus Lab4_BLE_NotifyMotion(uint8_t *count);

#endif
