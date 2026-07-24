#ifndef __ICM42688_H
#define __ICM42688_H

#include "ti_msp_dl_config.h"
#include <stdint.h>

#define ICM_ADDR 0x68

// 寄存器 (Bank 0)
enum {
    ICM_WHO_AM_I         = 0x75,
    ICM_BANK_SEL         = 0x76,
    ICM_DEVICE_CONFIG    = 0x11,
    ICM_PWR_MGMT0        = 0x4E,
    ICM_ACCEL_CONFIG0    = 0x50,
    ICM_GYRO_CONFIG0     = 0x4F,
    ICM_ACCEL_DATA_X1    = 0x1F,
    ICM_GYRO_DATA_X1     = 0x25,
    ICM_GYRO_ACCEL_CONFIG0 = 0x52,
};

// 量程
enum {
    AFS_2G  = 3,
    AFS_4G  = 2,
    AFS_8G  = 1,
    AFS_16G = 0,
    GFS_250 = 3,
    GFS_500 = 2,
    GFS_1000 = 1,
    GFS_2000 = 0,
    ODR_1K  = 6,
};

uint8_t ICM_Init(void);
void    ICM_ReadAccel(int16_t *ax, int16_t *ay, int16_t *az);
void    ICM_ReadGyro(int16_t *gx, int16_t *gy, int16_t *gz);

#endif
