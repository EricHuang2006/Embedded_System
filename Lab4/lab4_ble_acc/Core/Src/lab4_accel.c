#include "lab4_accel.h"
#include "lsm6dsl.h"
#include <string.h>

extern I2C_HandleTypeDef hi2c2;
static HAL_StatusTypeDef sensor_io_status;

static void record_status(HAL_StatusTypeDef status)
{
    if (status != HAL_OK) {
        sensor_io_status = status;
    }
}

HAL_StatusTypeDef Lab4_Accel_Init(void)
{
    sensor_io_status = HAL_OK;
    uint8_t id = LSM6DSL_AccReadID();
    if (sensor_io_status != HAL_OK) {
        return sensor_io_status;
    }
    if (id != LSM6DSL_ACC_GYRO_WHO_AM_I) {
        return HAL_ERROR;
    }

    /* 52 Hz, +/-2 g; hold complete samples and increment register addresses. */
    uint16_t settings = LSM6DSL_ODR_52Hz | LSM6DSL_ACC_FULLSCALE_2G;
    settings |= (uint16_t)(LSM6DSL_BDU_BLOCK_UPDATE |
                           LSM6DSL_ACC_GYRO_IF_INC_ENABLED) << 8;
    LSM6DSL_AccInit(settings);
    return sensor_io_status;
}

HAL_StatusTypeDef Lab4_Accel_Read(int16_t xyz[3])
{
    sensor_io_status = HAL_OK;
    LSM6DSL_AccReadXYZ(xyz);
    return sensor_io_status;
}

HAL_StatusTypeDef Lab4_Accel_EnableSignificantMotion(void)
{
    const uint8_t addr = LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW;
    sensor_io_status = HAL_OK;

    /* AN5040: the significant-motion threshold lives in embedded bank A.
       Three recognized steps make the classroom demonstration practical. */
    SENSOR_IO_Write(addr, LSM6DSL_ACC_GYRO_FUNC_CFG_ACCESS, 0x80U);
    if (sensor_io_status == HAL_OK) {
        SENSOR_IO_Write(addr, LSM6DSL_ACC_GYRO_SM_STEP_THS, 3U);
    }
    /* Always return to the normal register bank, even after an I2C error. */
    SENSOR_IO_Write(addr, LSM6DSL_ACC_GYRO_FUNC_CFG_ACCESS, 0U);
    if (sensor_io_status != HAL_OK) {
        return sensor_io_status;
    }

    uint8_t tap_cfg = SENSOR_IO_Read(addr, LSM6DSL_ACC_GYRO_TAP_CFG1);
    SENSOR_IO_Write(addr, LSM6DSL_ACC_GYRO_TAP_CFG1,
                    (uint8_t)(tap_cfg | 0x01U)); /* LIR: latch the interrupt. */
    uint8_t ctrl10 = SENSOR_IO_Read(addr, LSM6DSL_ACC_GYRO_CTRL10_C);
    SENSOR_IO_Write(addr, LSM6DSL_ACC_GYRO_CTRL10_C,
                    (uint8_t)(ctrl10 | 0x05U)); /* FUNC_EN + SIGN_MOTION_EN. */
    uint8_t int1 = SENSOR_IO_Read(addr, LSM6DSL_ACC_GYRO_INT1_CTRL);
    SENSOR_IO_Write(addr, LSM6DSL_ACC_GYRO_INT1_CTRL,
                    (uint8_t)(int1 | 0x40U)); /* Route significant motion to INT1. */
    return sensor_io_status;
}

HAL_StatusTypeDef Lab4_Accel_ReadMotionSource(uint8_t *detected)
{
    sensor_io_status = HAL_OK;
    /* Reading FUNC_SRC1 clears the latched interrupt and rearms detection. */
    uint8_t source = SENSOR_IO_Read(LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW,
                                    LSM6DSL_ACC_GYRO_FUNC_SRC);
    *detected = (source & 0x40U) != 0U; /* SIGN_MOTION_IA. */
    return sensor_io_status;
}

/* Board I/O functions required by the ST component driver. */
void SENSOR_IO_Init(void)
{
    /* MX_I2C2_Init() has already initialized hi2c2 in main(). */
}

void SENSOR_IO_Write(uint8_t addr, uint8_t reg, uint8_t value)
{
    record_status(HAL_I2C_Mem_Write(&hi2c2, addr, reg,
                                   I2C_MEMADD_SIZE_8BIT, &value, 1, 100));
}

uint8_t SENSOR_IO_Read(uint8_t addr, uint8_t reg)
{
    uint8_t value = 0;
    record_status(HAL_I2C_Mem_Read(&hi2c2, addr, reg,
                                  I2C_MEMADD_SIZE_8BIT, &value, 1, 100));
    return value;
}

uint16_t SENSOR_IO_ReadMultiple(uint8_t addr, uint8_t reg,
                               uint8_t *buffer, uint16_t length)
{
    memset(buffer, 0, length);
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c2, addr, reg,
                                              I2C_MEMADD_SIZE_8BIT,
                                              buffer, length, 100);
    record_status(status);
    return status == HAL_OK ? length : 0;
}

void SENSOR_IO_WriteMultiple(uint8_t addr, uint8_t reg,
                            uint8_t *buffer, uint16_t length)
{
    record_status(HAL_I2C_Mem_Write(&hi2c2, addr, reg,
                                   I2C_MEMADD_SIZE_8BIT, buffer, length, 100));
}
