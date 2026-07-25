/*
 * ICM42688 + OLED 安全测试程序 — 地猛星 MSPM0G3507
 *
 * 连线表:
 *   OLED(4针): VDD→3.3V  GND→GND  SCK→PB2  SDA→PB3
 *   ICM42688:  3V3→3.3V  GND→GND  SCL→PA15  SDA→PA14
 *              CS→PA13(HIGH)  AD0→PA16(LOW)  INT1/FSYNC→不接
 *   串口:      PA28(TX)→USB-TTL RXD,  PA31(RX)→USB-TTL TXD, 115200
 *   LED:       PB8=状态灯  PB9=倾斜灯
 *
 * 安全边界:
 *   - 不执行软件复位；
 *   - 不启用看门狗；
 *   - 不写 Flash、NONMAIN、BCR 或 BSL；
 *   - 时钟只使用 empty.syscfg 中的默认 SYSOSC 配置。
 */
#include "ti_msp_dl_config.h"
#include "oled.h"
#include "icm42688.h"

#define GYRO_CALIBRATION_SAMPLES 200U
#define UART_BUSY_TIMEOUT        100000U

void delay_ms(uint32_t ms);

// ==================== 串口 ====================
static void UART_Putc(char c)
{
    uint32_t timeout = UART_BUSY_TIMEOUT;

    while (DL_UART_isBusy(PRINT_INST) && (timeout > 0U)) {
        timeout--;
    }

    if (timeout > 0U) {
        DL_UART_Main_transmitData(PRINT_INST, (uint8_t)c);
    }
}

static void UART_Puts(const char *s)
{
    while (*s != '\0') {
        UART_Putc(*s++);
    }
}

static void UART_PutHex(uint8_t value)
{
    uint8_t high = (value >> 4) & 0x0FU;
    uint8_t low  = value & 0x0FU;

    UART_Putc((char)(high < 10U ? ('0' + high) : ('A' + high - 10U)));
    UART_Putc((char)(low < 10U ? ('0' + low) : ('A' + low - 10U)));
}

static void UART_PutNum(int32_t value)
{
    char buffer[12];
    uint8_t index = 0U;
    uint32_t magnitude;

    if (value < 0) {
        UART_Putc('-');
        magnitude = (uint32_t)(-(value + 1)) + 1U;
    } else {
        magnitude = (uint32_t)value;
    }

    if (magnitude == 0U) {
        UART_Putc('0');
        return;
    }

    while ((magnitude > 0U) && (index < sizeof(buffer))) {
        buffer[index++] = (char)('0' + (magnitude % 10U));
        magnitude /= 10U;
    }

    while (index > 0U) {
        UART_Putc(buffer[--index]);
    }
}

// ==================== 延时 ====================
void delay_ms(uint32_t ms)
{
    for (volatile uint32_t i = 0U; i < (ms * 4000U); i++) {
        __NOP();
    }
}

// ==================== OLED 显示六轴 ====================
static void OLED_ShowSigned(u8 x, u8 y, int16_t value, u8 len, u8 size)
{
    int32_t displayValue = value;

    if (displayValue >= 0) {
        OLED_ShowChar(x, y, '+', size);
    } else {
        OLED_ShowChar(x, y, '-', size);
        displayValue = -displayValue;
    }

    OLED_ShowNum(x + size / 2U, y, (uint32_t)displayValue, len, size);
}

static int16_t ClampInt16(int32_t value)
{
    if (value > INT16_MAX) {
        return INT16_MAX;
    }
    if (value < INT16_MIN) {
        return INT16_MIN;
    }
    return (int16_t)value;
}

static void CalibrateGyro(
    int16_t *offsetX,
    int16_t *offsetY,
    int16_t *offsetZ)
{
    int32_t sumX = 0;
    int32_t sumY = 0;
    int32_t sumZ = 0;
    int16_t gx;
    int16_t gy;
    int16_t gz;

    UART_Puts("Keep ICM42688 still: calibrating gyro...\r\n");

    OLED_Clear();
    OLED_ShowString(0, 16, (u8 *)"KEEP ICM STILL", 16);
    OLED_ShowString(16, 32, (u8 *)"CALIBRATING", 16);
    OLED_Refresh();

    for (uint16_t i = 0U; i < 20U; i++) {
        ICM_ReadGyro(&gx, &gy, &gz);
        delay_ms(5U);
    }

    for (uint16_t i = 0U; i < GYRO_CALIBRATION_SAMPLES; i++) {
        ICM_ReadGyro(&gx, &gy, &gz);
        sumX += gx;
        sumY += gy;
        sumZ += gz;
        delay_ms(5U);
    }

    *offsetX = (int16_t)(sumX / (int32_t)GYRO_CALIBRATION_SAMPLES);
    *offsetY = (int16_t)(sumY / (int32_t)GYRO_CALIBRATION_SAMPLES);
    *offsetZ = (int16_t)(sumZ / (int32_t)GYRO_CALIBRATION_SAMPLES);

    UART_Puts("GYRO OFFSET: ");
    UART_PutNum(*offsetX);
    UART_Puts(" ");
    UART_PutNum(*offsetY);
    UART_Puts(" ");
    UART_PutNum(*offsetZ);
    UART_Puts("\r\nCalibration complete\r\n");
}

// ==================== 主程序 ====================
int main(void)
{
    SYSCFG_DL_init();

    /* 留出启动窗口，避免上电后立刻访问外设。 */
    delay_ms(500U);

    OLED_Init();
    OLED_ShowString(24, 0, (u8 *)"ICM42688", 16);
    OLED_Refresh();

    UART_Puts("\r\n=== ICM42688 + OLED ===\r\n");
    UART_Puts("FW: ICM_GYRO_SAFE_V4\r\n");

    if (ICM_Init() != 0U) {
        OLED_ShowString(0, 24, (u8 *)"ERR: No ICM", 16);
        OLED_Refresh();

        UART_Puts("FAIL: ICM42688 not found\r\n");
        UART_Puts("WHO_AM_I AD0=LOW : 0x");
        UART_PutHex(ICM_ReadWhoAmI(0U));
        UART_Puts("\r\n");
        UART_Puts("WHO_AM_I AD0=HIGH: 0x");
        UART_PutHex(ICM_ReadWhoAmI(1U));
        UART_Puts("\r\n");
        UART_Puts("GPIO levels SDA/SCL/CS/AD0: ");
        UART_PutNum(ICM_ReadSdaLevel());
        UART_Puts("/");
        UART_PutNum(ICM_ReadSclLevel());
        UART_Puts("/");
        UART_PutNum(ICM_ReadCsLevel());
        UART_Puts("/");
        UART_PutNum(ICM_ReadAd0Level());
        UART_Puts("\r\nExpected ID: 0x47\r\n");

        while (1) {
            DL_GPIO_togglePins(LED_PORT, LED_LED0_PIN);
            delay_ms(200U);
        }
    }

    OLED_ShowString(0, 24, (u8 *)"OK! ID:0x47", 16);
    OLED_Refresh();
    UART_Puts("ICM42688 OK\r\n");
    DL_GPIO_setPins(LED_PORT, LED_LED0_PIN);

    int16_t gyroOffsetX;
    int16_t gyroOffsetY;
    int16_t gyroOffsetZ;
    CalibrateGyro(&gyroOffsetX, &gyroOffsetY, &gyroOffsetZ);

    while (1) {
        int16_t ax;
        int16_t ay;
        int16_t az;
        int16_t gx;
        int16_t gy;
        int16_t gz;

        ICM_ReadAccel(&ax, &ay, &az);
        ICM_ReadGyro(&gx, &gy, &gz);

        gx = ClampInt16((int32_t)gx - gyroOffsetX);
        gy = ClampInt16((int32_t)gy - gyroOffsetY);
        gz = ClampInt16((int32_t)gz - gyroOffsetZ);

        UART_Puts("A:");
        UART_PutNum(ax);
        UART_Puts(" ");
        UART_PutNum(ay);
        UART_Puts(" ");
        UART_PutNum(az);
        UART_Puts("  G:");
        UART_PutNum(gx);
        UART_Puts(" ");
        UART_PutNum(gy);
        UART_Puts(" ");
        UART_PutNum(gz);
        UART_Puts("\r\n");

        if ((ax > 500) || (ax < -500) || (ay > 500) || (ay < -500)) {
            DL_GPIO_setPins(LED_PORT, LED_LED1_PIN);
        } else {
            DL_GPIO_clearPins(LED_PORT, LED_LED1_PIN);
        }

        OLED_Clear();
        OLED_ShowString(24, 0, (u8 *)"ICM42688", 16);
        OLED_ShowSigned(0, 16, ax, 5, 16);
        OLED_ShowSigned(64, 16, ay, 5, 16);
        OLED_ShowSigned(0, 32, az, 5, 16);
        OLED_ShowSigned(64, 32, gx, 5, 16);
        OLED_ShowSigned(0, 48, gy, 5, 16);
        OLED_ShowSigned(64, 48, gz, 5, 16);
        OLED_Refresh();

        delay_ms(100U);
    }
}
