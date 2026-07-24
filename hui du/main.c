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
static void UART_PutNum(int16_t n)
{
    if (n < 0) { UART_Putc('-'); n = -n; }
    if (n == 0) { UART_Putc('0'); return; }
    char buf[8]; uint8_t i = 0;
    while (n > 0 && i < 6) { buf[i++] = '0' + (n % 10); n /= 10; }
    while (i > 0) UART_Putc(buf[--i]);
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

    uint8_t r = ICM_Init();
    if (r) {
        OLED_ShowString(0, 24, (u8 *)"ERR: No ICM", 16);
        OLED_Refresh();
        OLED_Refresh();
        UART_Puts("FAIL: ICM42688 not found\r\n");
        while (1) {
            DL_GPIO_togglePins(LED_PORT, LED_LED0_PIN);
            delay_ms(200);
        }
    }

    OLED_ShowString(0, 24, (u8 *)"OK! ID:0x47", 16);
    OLED_Refresh();
    UART_Puts("ICM42688 OK\r\n");
    DL_GPIO_setPins(LED_PORT, LED_LED0_PIN);

    while (1) {
        int16_t ax, ay, az, gx, gy, gz;
        ICM_ReadAccel(&ax, &ay, &az);
        ICM_ReadGyro(&gx, &gy, &gz);

        // 串口
        UART_Puts("A:"); UART_PutNum(ax);
        UART_Puts(" ");  UART_PutNum(ay);
        UART_Puts(" ");  UART_PutNum(az);
        UART_Puts("  G:"); UART_PutNum(gx);
        UART_Puts(" ");   UART_PutNum(gy);
        UART_Puts(" ");   UART_PutNum(gz);
        UART_Puts("\r\n");

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
