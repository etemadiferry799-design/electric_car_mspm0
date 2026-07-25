/*
 * ICM42688 + OLED 测试程序 — 地猛星 MSPM0G3507
 *
 * 连线表:
 *   OLED(4针): VDD→3.3V  GND→GND  SCK→PB2  SDA→PB3
 *   ICM42688:  3V3→3.3V  GND→GND  SCL→PA15  SDA→PA14
 *              CS→PA13(HIGH)  AD0→PA16(LOW)  INT1/FSYNC→不接
 *   串口:      PA28(TX)→USB-TTL RXD,  PA31(RX)→USB-TTL TXD, 115200
 *   灰度模块:  AD0→PA0  AD1→PA1  AD2→PA2  OUT→PA27(ADC)
 *   LED:       PB8=状态灯  PB9=倾斜灯
 */
#include "ti_msp_dl_config.h"
#include "oled.h"
#include "icm42688.h"
#include "gray.h"

void delay_ms(uint32_t ms);

// ==================== 串口 ====================
static void UART_Putc(char c)
{
    while (DL_UART_isBusy(PRINT_INST));
    DL_UART_Main_transmitData(PRINT_INST, (uint8_t)c);
}
static void UART_Puts(const char *s) { while (*s) UART_Putc(*s++); }
static void UART_PutHex(uint8_t v)
{
    char h = (v >> 4) & 0xF; UART_Putc(h < 10 ? '0' + h : 'A' + h - 10);
    char l = v & 0xF;        UART_Putc(l < 10 ? '0' + l : 'A' + l - 10);
}
static void UART_PutNum(int32_t n)
{
    if (n < 0) { UART_Putc('-'); n = -n; }
    if (n == 0) { UART_Putc('0'); return; }
    char buf[8]; uint8_t i = 0;
    while (n > 0 && i < 6) { buf[i++] = '0' + (n % 10); n /= 10; }
    while (i > 0) UART_Putc(buf[--i]);
}

#define GYRO_CALIBRATION_SAMPLES 200

static volatile uint32_t g_grayAdcTimeoutCount = 0U;

static int16_t ClampInt16(int32_t value)
{
    if (value > INT16_MAX) return INT16_MAX;
    if (value < INT16_MIN) return INT16_MIN;
    return (int16_t)value;
}

static void CalibrateGyro(int16_t *offsetX, int16_t *offsetY, int16_t *offsetZ)
{
    int32_t sumX = 0, sumY = 0, sumZ = 0;
    int16_t gx, gy, gz;

    UART_Puts("Keep ICM42688 still: calibrating gyro...\r\n");
    OLED_Clear();
    OLED_ShowString(0, 16, (u8 *)"KEEP ICM STILL", 16);
    OLED_ShowString(16, 32, (u8 *)"CALIBRATING", 16);
    OLED_Refresh();

    /* 丢弃刚启动时的瞬态数据。 */
    for (uint16_t i = 0; i < 20; i++) {
        ICM_ReadGyro(&gx, &gy, &gz);
        delay_ms(5);
    }

    for (uint16_t i = 0; i < GYRO_CALIBRATION_SAMPLES; i++) {
        ICM_ReadGyro(&gx, &gy, &gz);
        sumX += gx;
        sumY += gy;
        sumZ += gz;
        delay_ms(5);
    }

    *offsetX = (int16_t)(sumX / GYRO_CALIBRATION_SAMPLES);
    *offsetY = (int16_t)(sumY / GYRO_CALIBRATION_SAMPLES);
    *offsetZ = (int16_t)(sumZ / GYRO_CALIBRATION_SAMPLES);

    UART_Puts("GYRO OFFSET: "); UART_PutNum(*offsetX);
    UART_Puts(" "); UART_PutNum(*offsetY);
    UART_Puts(" "); UART_PutNum(*offsetZ);
    UART_Puts("\r\nCalibration complete\r\n");
}

static uint16_t ReadGrayAdc(void)
{
    uint32_t timeout = 100000U;

    /*
     * Follow TI's single-conversion example sequence: SysConfig enables ENC
     * initially; start, wait, read, and then re-enable ENC for the next read.
     */
    DL_ADC12_clearInterruptStatus(
        GRAY_ADC_INST, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);
    DL_ADC12_startConversion(GRAY_ADC_INST);

    while ((DL_ADC12_getRawInterruptStatus(
                GRAY_ADC_INST, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED) == 0U) &&
           (timeout > 0U)) {
        timeout--;
    }

    if (timeout == 0U) {
        g_grayAdcTimeoutCount++;
        DL_ADC12_enableConversions(GRAY_ADC_INST);
        return 0U;
    }

    uint16_t result = DL_ADC12_getMemResult(
        GRAY_ADC_INST, GRAY_ADC_ADCMEM_GRAY_ADC_MEM);
    DL_ADC12_enableConversions(GRAY_ADC_INST);
    return result;
}

static void UART_PrintGray(const uint16_t values[GRAY_CHANNEL_COUNT])
{
    UART_Puts("GRAY:");
    for (uint8_t i = 0U; i < GRAY_CHANNEL_COUNT; i++) {
        UART_Puts(" ");
        UART_PutNum(values[i]);
    }
    UART_Puts("\r\n");
}

// ==================== 延时 ====================
void delay_ms(uint32_t ms)
{
    for (volatile uint32_t i = 0; i < ms * 4000; i++);
}

// ==================== OLED 显示六轴 ====================
static void OLED_ShowSigned(u8 x, u8 y, int16_t val, u8 len, u8 size)
{
    if (val >= 0) {
        OLED_ShowChar(x, y, '+', size);
        OLED_ShowNum(x + size / 2, y, val, len, size);
    } else {
        OLED_ShowChar(x, y, '-', size);
        OLED_ShowNum(x + size / 2, y, -val, len, size);
    }
}

// ==================== 主程序 ====================
int main(void)
{
    SYSCFG_DL_init();
    OLED_Init();

    OLED_ShowString(24, 0, (u8 *)"ICM42688", 16);
    OLED_Refresh();
    UART_Puts("\r\n=== ICM42688 + OLED ===\r\n");
    UART_Puts("FW: SENSOR_BRINGUP_SAFE_V9\r\n");

    uint8_t r = ICM_Init();
    if (r) {
        OLED_ShowString(0, 24, (u8 *)"ERR: No ICM", 16);
        OLED_Refresh();
        UART_Puts("FAIL: ICM42688 not found\r\n");
        UART_Puts("WHO_AM_I AD0=LOW : 0x");
        UART_PutHex(ICM_ReadWhoAmI(0));
        UART_Puts("\r\n");
        UART_Puts("WHO_AM_I AD0=HIGH: 0x");
        UART_PutHex(ICM_ReadWhoAmI(1));
        UART_Puts("\r\n");
        UART_Puts("GPIO levels SDA/SCL/CS/AD0: ");
        UART_PutNum(ICM_ReadSdaLevel());
        UART_Puts("/");
        UART_PutNum(ICM_ReadSclLevel());
        UART_Puts("/");
        UART_PutNum(ICM_ReadCsLevel());
        UART_Puts("/");
        UART_PutNum(ICM_ReadAd0Level());
        UART_Puts("\r\n");
        UART_Puts("Expected ID: 0x47. If SDA voltage is 3.3V but GPIO SDA level is 0, change SDA to a free GPIO or fix pin config.\r\n");
        while (1) {
            DL_GPIO_togglePins(LED_PORT, LED_LED0_PIN);
            delay_ms(200);
        }
    }

    OLED_ShowString(0, 24, (u8 *)"OK! ID:0x47", 16);
    OLED_Refresh();
    UART_Puts("ICM42688 OK\r\n");
    DL_GPIO_setPins(LED_PORT, LED_LED0_PIN);

    int16_t gyroOffsetX, gyroOffsetY, gyroOffsetZ;
    CalibrateGyro(&gyroOffsetX, &gyroOffsetY, &gyroOffsetZ);
    /* PA27 analog pinmux and ADC MEM0 are configured only by SysConfig. */
    Gray_Init(ReadGrayAdc);
    UART_Puts("GRAY ADC ready: PA27, 8 channels\r\n");
    UART_Puts("ADC peripheral power: ");
    UART_PutNum(DL_ADC12_isPowerEnabled(GRAY_ADC_INST) ? 1 : 0);
    UART_Puts("\r\n");

    while (1) {
        int16_t ax, ay, az, gx, gy, gz;
        uint16_t grayValues[GRAY_CHANNEL_COUNT];
        uint32_t timeoutCountBeforeRead = g_grayAdcTimeoutCount;
        ICM_ReadAccel(&ax, &ay, &az);
        ICM_ReadGyro(&gx, &gy, &gz);
        gx = ClampInt16((int32_t)gx - gyroOffsetX);
        gy = ClampInt16((int32_t)gy - gyroOffsetY);
        gz = ClampInt16((int32_t)gz - gyroOffsetZ);

        // 串口
        UART_Puts("A:"); UART_PutNum(ax);
        UART_Puts(" ");  UART_PutNum(ay);
        UART_Puts(" ");  UART_PutNum(az);
        UART_Puts("  G:"); UART_PutNum(gx);
        UART_Puts(" ");   UART_PutNum(gy);
        UART_Puts(" ");   UART_PutNum(gz);
        UART_Puts("\r\n");

        Gray_ReadAll(grayValues);
        UART_PrintGray(grayValues);
        UART_Puts("GRAY_ADDR_DOE/OUT: ");
        UART_PutNum((GPIOA->DOE31_0 >> 0) & 0x07U);
        UART_Puts("/");
        UART_PutNum((GPIOA->DOUT31_0 >> 0) & 0x07U);
        UART_Puts("  PA27_mV_approx: ");
        UART_PutNum(((int32_t)grayValues[7] * 3300) / 4095);
        UART_Puts("\r\n");
        if (g_grayAdcTimeoutCount != timeoutCountBeforeRead) {
            UART_Puts("ADC_TIMEOUT: PA27 conversion did not finish\r\n");
        }

        // LED: 倾斜检测
        if (ax > 500 || ax < -500 || ay > 500 || ay < -500)
            DL_GPIO_setPins(LED_PORT, LED_LED1_PIN);
        else
            DL_GPIO_clearPins(LED_PORT, LED_LED1_PIN);

        // OLED (16pt, 8px/字, 128/8=16列, 64/16=4行)
        OLED_Clear();  // 清上一帧
        OLED_ShowString(24, 0,  (u8 *)"ICM42688", 16);
        OLED_ShowSigned(0,  16, ax, 5, 16);
        OLED_ShowSigned(64, 16, ay, 5, 16);
        OLED_ShowSigned(0,  32, az, 5, 16);
        OLED_ShowSigned(64, 32, gx, 5, 16);
        OLED_ShowSigned(0,  48, gy, 5, 16);
        OLED_ShowSigned(64, 48, gz, 5, 16);
        OLED_Refresh();

        delay_ms(100);
    }
}
