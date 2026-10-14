#include "lab4_ble.h"
#include "bluenrg_gatt_aci.h"
#include "hci.h"
#include "sensor.h"
#include <stdio.h>

extern volatile uint8_t set_connectable;

static uint16_t service_handle;
static uint16_t accel_handle;
static uint16_t rate_handle;
static uint16_t motion_handle;
static uint8_t motion_count;
/* Written by TASK_BLE and read by TASK_ACC. Aligned 16-bit accesses are atomic on this MCU. */
static volatile uint16_t sample_rate_hz = LAB4_RATE_DEFAULT_HZ;
static uint8_t service_ready;

/* a4c00000-7a67-4b5a-9f5b-c3c893bdae40, least significant byte first. */
static const uint8_t service_uuid[16] = {
    0x40, 0xae, 0xbd, 0x93, 0xc8, 0xc3, 0x5b, 0x9f,
    0x5a, 0x4b, 0x67, 0x7a, 0x00, 0x00, 0xc0, 0xa4
};

/* a4c00001-7a67-4b5a-9f5b-c3c893bdae40. */
static const uint8_t accel_uuid[16] = {
    0x40, 0xae, 0xbd, 0x93, 0xc8, 0xc3, 0x5b, 0x9f,
    0x5a, 0x4b, 0x67, 0x7a, 0x01, 0x00, 0xc0, 0xa4
};

/* a4c00002-7a67-4b5a-9f5b-c3c893bdae40. */
static const uint8_t rate_uuid[16] = {
    0x40, 0xae, 0xbd, 0x93, 0xc8, 0xc3, 0x5b, 0x9f,
    0x5a, 0x4b, 0x67, 0x7a, 0x02, 0x00, 0xc0, 0xa4
};

/* a4c00003-7a67-4b5a-9f5b-c3c893bdae40. */
static const uint8_t motion_uuid[16] = {
    0x40, 0xae, 0xbd, 0x93, 0xc8, 0xc3, 0x5b, 0x9f,
    0x5a, 0x4b, 0x67, 0x7a, 0x03, 0x00, 0xc0, 0xa4
};

tBleStatus Lab4_BLE_Init(void)
{
    /* Nine records: service, XYZ declaration/value/CCCD, rate
       declaration/value, and motion declaration/value/CCCD. */
    tBleStatus status = aci_gatt_add_serv(UUID_TYPE_128, service_uuid,
                                        PRIMARY_SERVICE, 9, &service_handle);
    if (status != BLE_STATUS_SUCCESS) {
        return status;
    }

    status = aci_gatt_add_char(service_handle, UUID_TYPE_128, accel_uuid,
                               6, CHAR_PROP_READ | CHAR_PROP_NOTIFY,
                               ATTR_PERMISSION_NONE, GATT_DONT_NOTIFY_EVENTS,
                               16, 0, &accel_handle);
    if (status != BLE_STATUS_SUCCESS) {
        return status;
    }

    /* Reads return the latest stored sample without an application callback.
       The controller creates the CCCD and manages notification subscriptions. */
    const uint8_t initial_value[6] = {0};
    status = aci_gatt_update_char_value(service_handle, accel_handle,
                                        0, sizeof(initial_value), initial_value);
    if (status != BLE_STATUS_SUCCESS) {
        return status;
    }

    status = aci_gatt_add_char(service_handle, UUID_TYPE_128, rate_uuid,
                               2, CHAR_PROP_READ | CHAR_PROP_WRITE,
                               ATTR_PERMISSION_NONE,
                               GATT_NOTIFY_WRITE_REQ_AND_WAIT_FOR_APPL_RESP,
                               16, 0, &rate_handle);
    if (status != BLE_STATUS_SUCCESS) {
        return status;
    }

    const uint8_t initial_rate[2] = {
        (uint8_t)sample_rate_hz, (uint8_t)(sample_rate_hz >> 8)
    };
    status = aci_gatt_update_char_value(service_handle, rate_handle,
                                        0, sizeof(initial_rate), initial_rate);
    if (status != BLE_STATUS_SUCCESS) {
        return status;
    }

    status = aci_gatt_add_char(service_handle, UUID_TYPE_128, motion_uuid,
                               1, CHAR_PROP_READ | CHAR_PROP_NOTIFY,
                               ATTR_PERMISSION_NONE, GATT_DONT_NOTIFY_EVENTS,
                               16, 0, &motion_handle);
    if (status != BLE_STATUS_SUCCESS) {
        return status;
    }
    motion_count = 0U;
    status = aci_gatt_update_char_value(service_handle, motion_handle,
                                        0, 1, &motion_count);
    service_ready = (status == BLE_STATUS_SUCCESS);
    return status;
}

void Lab4_BLE_Process(void)
{
    if (set_connectable) {
        Set_DeviceConnectable();
        set_connectable = 0;
    }
    hci_user_evt_proc();
}

uint16_t Lab4_BLE_GetSampleRateHz(void)
{
    return sample_rate_hz;
}

void Lab4_BLE_HandleWriteRequest(uint16_t connection, uint16_t attribute,
                                uint8_t length, uint8_t *data)
{
    /* The value attribute is immediately after the characteristic declaration. */
    if (!service_ready || attribute != rate_handle + 1U) {
        return;
    }

    uint8_t error = 0;
    uint16_t requested_hz = 0;
    if (length != 2U) {
        error = 0x0D; /* ATT: invalid attribute value length. */
    } else {
        requested_hz = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
        if (requested_hz < LAB4_RATE_MIN_HZ || requested_hz > LAB4_RATE_MAX_HZ) {
            error = 0x80; /* ATT application error: rate outside supported range. */
        }
    }

    /* Accepting lets BlueNRG store these bytes and reply to the client.
       Rejecting leaves the previous value and sampling rate unchanged. */
    tBleStatus status = aci_gatt_write_response(connection, attribute,
                                               error != 0U, error, length, data);
    if (status != BLE_STATUS_SUCCESS) {
        printf("BLE write response failed: 0x%02X\r\n", status);
    } else if (error == 0U) {
        sample_rate_hz = requested_hz;
        printf("Sampling rate set to %u Hz (period %lu ms).\r\n",
               (unsigned int)sample_rate_hz,
               (unsigned long)(1000U / sample_rate_hz));
    } else {
        printf("Sampling rate rejected; use two bytes for 1-20 Hz.\r\n");
    }
}

tBleStatus Lab4_BLE_UpdateAcceleration(const int16_t xyz[3])
{
    if (!service_ready) {
        return BLE_STATUS_ERROR;
    }

    uint8_t payload[6];
    for (unsigned int axis = 0; axis < 3; ++axis) {
        /* Cast to unsigned before shifting, preserving negative values'
           two's-complement representation without signed right shifts. */
        uint16_t value = (uint16_t)xyz[axis];
        payload[2 * axis] = (uint8_t)value;
        payload[2 * axis + 1] = (uint8_t)(value >> 8);
    }

    /* Store the sample; notify a connected client if its CCCD is enabled.
       Return a full-buffer error to the caller instead of silently hiding it. */
    return aci_gatt_update_char_value(service_handle, accel_handle,
                                      0, sizeof(payload), payload);
}

tBleStatus Lab4_BLE_NotifyMotion(uint8_t *count)
{
    if (!service_ready) {
        return BLE_STATUS_ERROR;
    }

    uint8_t next_count = (uint8_t)(motion_count + 1U);
    tBleStatus status = aci_gatt_update_char_value(service_handle, motion_handle,
                                                    0, 1, &next_count);
    if (status == BLE_STATUS_SUCCESS) {
        motion_count = next_count;
        *count = motion_count;
    }
    return status;
}
