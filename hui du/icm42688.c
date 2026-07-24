#include "icm42688.h"

// 引脚宏由 SysConfig 生成:
//   ICM_CS_PORT=GPIOA,  ICM_CS_CS_PIN=DL_GPIO_PIN_13
//   ICM_SCL_PORT=GPIOA, ICM_SCL_SCL_PIN=DL_GPIO_PIN_15
//   ICM_SDA_PORT=GPIOA, ICM_SDA_SDA_PIN=DL_GPIO_PIN_14
//   ICM_AD0_PORT=GPIOA, ICM_AD0_AD0_PIN=DL_GPIO_PIN_16

// 直接使用 PA13/PA14/PA15/PA16，避免 SysConfig 生成的 *_PIN 宏在不同版本中
// 出现“引脚编号”和“位掩码”差异，导致 set/read 操作读写错位。
#define SCL_PORT  GPIOA
#define SCL_PIN   DL_GPIO_PIN_15
#define SDA_PORT  GPIOA
#define SDA_PIN   DL_GPIO_PIN_14
#define CS_PORT   GPIOA
#define CS_PIN    DL_GPIO_PIN_13
#define AD0_PORT  GPIOA
#define AD0_PIN   DL_GPIO_PIN_16
#define SCL_IOMUX ICM_SCL_SCL_IOMUX
#define SDA_IOMUX ICM_SDA_SDA_IOMUX
#define CS_IOMUX  ICM_CS_CS_IOMUX
#define AD0_IOMUX ICM_AD0_AD0_IOMUX

extern void delay_ms(uint32_t ms);
static void ICM_PreparePins(uint8_t ad0High)
static uint8_t ReadRegAt(uint8_t addr, uint8_t reg)
static void i2c_delay(void) { for (volatile int i = 0; i < 50; i++); }

// SDA 方向: 直接写 DOE (不影响 IOMUX, DL_GPIO 无 setDirection API)
static void SDA_Out(void)
{
    DL_GPIO_initDigitalOutput(SDA_IOMUX);
    SDA_PORT->DOE31_0 |= SDA_PIN;
}

static void SDA_In(void)
{
    DL_GPIO_initDigitalInputFeatures(SDA_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    SDA_PORT->DOE31_0 &= ~SDA_PIN;
}

#define SCL_H()  DL_GPIO_setPins(SCL_PORT, SCL_PIN)
#define SCL_L()  DL_GPIO_clearPins(SCL_PORT, SCL_PIN)
#define SDA_H()  DL_GPIO_setPins(SDA_PORT, SDA_PIN)
#define SDA_L()  DL_GPIO_clearPins(SDA_PORT, SDA_PIN)
#define SDA_R()  (DL_GPIO_readPins(SDA_PORT, SDA_PIN) ? 1 : 0)

static void I2C_Start(void)
{
    SDA_Out();
    SDA_H(); i2c_delay();
    SCL_H(); i2c_delay();
    SDA_L(); i2c_delay();
    SCL_L(); i2c_delay();
}

static void I2C_Stop(void)
{
    SDA_Out();
    SDA_L(); i2c_delay();
    SCL_H(); i2c_delay();
    SDA_H(); i2c_delay();
}

static uint8_t I2C_Write(uint8_t data)
{
    SDA_Out();
    for (uint8_t i = 0; i < 8; i++) {
        if (data & 0x80) SDA_H(); else SDA_L();
        data <<= 1;
        i2c_delay(); SCL_H(); i2c_delay(); SCL_L(); i2c_delay();
    }
    SDA_In(); i2c_delay();
    SCL_H(); i2c_delay();
    uint8_t ack = SDA_R();
    SCL_L(); i2c_delay();
    SDA_Out();
    return ack;
}

static uint8_t I2C_Read(uint8_t ack)
{
    uint8_t data = 0;
    SDA_In();
    for (uint8_t i = 0; i < 8; i++) {
        data <<= 1;
        SCL_H(); i2c_delay();
        if (SDA_R()) data |= 1;
        SCL_L(); i2c_delay();
    }
    SDA_Out();
    if (ack) SDA_L(); else SDA_H();
    i2c_delay(); SCL_H(); i2c_delay(); SCL_L(); i2c_delay();
    SDA_In();
    return data;
}

// ==================== I2C 读写寄存器 ====================
static void WriteReg(uint8_t reg, uint8_t val)
{
    I2C_Start();
    I2C_Write((ICM_ADDR << 1) | 0);
    I2C_Write(reg);
    I2C_Write(val);
    I2C_Stop();
}

static uint8_t ReadReg(uint8_t reg)
{
    I2C_Start();
    I2C_Write((ICM_ADDR << 1) | 0);
    I2C_Write(reg);
    I2C_Start();
    I2C_Write((ICM_ADDR << 1) | 1);
    uint8_t v = I2C_Read(0);
    I2C_Stop();
    return v;
}

static void ReadMulti(uint8_t reg, uint8_t *buf, uint8_t len)
{
    I2C_Start();
    I2C_Write((ICM_ADDR << 1) | 0);
    I2C_Write(reg);
    I2C_Start();
    I2C_Write((ICM_ADDR << 1) | 1);
    for (uint8_t i = 0; i < len; i++)
        buf[i] = I2C_Read(i < (len - 1));
    I2C_Stop();
}

// ==================== API ====================
static void ICM_PreparePins(uint8_t ad0High)
{
    DL_GPIO_initDigitalOutput(CS_IOMUX);
    DL_GPIO_initDigitalOutput(AD0_IOMUX);
    DL_GPIO_initDigitalOutput(SCL_IOMUX);
    DL_GPIO_initDigitalOutput(SDA_IOMUX);
    CS_PORT->DOE31_0  |= CS_PIN;
    AD0_PORT->DOE31_0 |= AD0_PIN;
    SCL_PORT->DOE31_0 |= SCL_PIN;
    SDA_PORT->DOE31_0 |= SDA_PIN;

    DL_GPIO_setPins(CS_PORT, CS_PIN);
    if (ad0High) {
        DL_GPIO_setPins(AD0_PORT, AD0_PIN);
    } else {
        DL_GPIO_clearPins(AD0_PORT, AD0_PIN);
    }
    DL_GPIO_setPins(SCL_PORT, SCL_PIN);
    DL_GPIO_setPins(SDA_PORT, SDA_PIN);
}
uint8_t ICM_ReadSdaLevel(void)
{
    SDA_In();
    delay_ms(1);
    return SDA_R();
}

uint8_t ICM_ReadSclLevel(void)
{
    return DL_GPIO_readPins(SCL_PORT, SCL_PIN) ? 1 : 0;
}

uint8_t ICM_ReadCsLevel(void)
{
    return DL_GPIO_readPins(CS_PORT, CS_PIN) ? 1 : 0;
}

uint8_t ICM_ReadAd0Level(void)
{
    return DL_GPIO_readPins(AD0_PORT, AD0_PIN) ? 1 : 0;
}
uint8_t ICM_Init(void)
{
    // SysConfig 已将 CS/SCL/SDA/AD0 初始化为输出
    DL_GPIO_setPins(CS_PORT, CS_PIN);        // CS=HIGH  → I2C
    DL_GPIO_clearPins(AD0_PORT, AD0_PIN);    // AD0=LOW  → 0x68
    DL_GPIO_setPins(SCL_PORT, SCL_PIN);      // 总线空闲
    DL_GPIO_setPins(SDA_PORT, SDA_PIN);

    // 软复位
    WriteReg(ICM_BANK_SEL, 0x00);
    WriteReg(ICM_DEVICE_CONFIG, 0x01);
    delay_ms(50);

    // ID 检测
    WriteReg(ICM_BANK_SEL, 0x00);
    if (ReadReg(ICM_WHO_AM_I) != 0x47) return 1;

    // 使能 (LN 模式)
    WriteReg(ICM_BANK_SEL, 0x00);
    WriteReg(ICM_PWR_MGMT0, 0x0F);
    delay_ms(10);

    // ±2g / 1kHz
    WriteReg(ICM_BANK_SEL, 0x00);
    WriteReg(ICM_ACCEL_CONFIG0, (AFS_2G << 5) | ODR_1K);

    // ±500dps / 1kHz
    WriteReg(ICM_BANK_SEL, 0x00);
    WriteReg(ICM_GYRO_CONFIG0, (GFS_500 << 5) | ODR_1K);

    return 0;
}

void ICM_ReadAccel(int16_t *ax, int16_t *ay, int16_t *az)
{
    uint8_t buf[6];
    WriteReg(ICM_BANK_SEL, 0x00);
    ReadMulti(ICM_ACCEL_DATA_X1, buf, 6);
    *ax = (int16_t)((buf[0] << 8) | buf[1]);
    *ay = (int16_t)((buf[2] << 8) | buf[3]);
    *az = (int16_t)((buf[4] << 8) | buf[5]);
}

void ICM_ReadGyro(int16_t *gx, int16_t *gy, int16_t *gz)
{
    uint8_t buf[6];
    WriteReg(ICM_BANK_SEL, 0x00);
    ReadMulti(ICM_GYRO_DATA_X1, buf, 6);
    *gx = (int16_t)((buf[0] << 8) | buf[1]);
    *gy = (int16_t)((buf[2] << 8) | buf[3]);
    *gz = (int16_t)((buf[4] << 8) | buf[5]);
}
