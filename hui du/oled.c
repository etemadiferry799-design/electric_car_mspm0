#include "oled.h"
#include "oledfont.h"

#define OLED_ADDR 0x3C

u8 OLED_GRAM[128][8];
extern void delay_ms(uint32_t ms);

void OLED_WR_Byte(uint8_t dat, uint8_t mode)
{
    uint8_t tx[2];
    tx[0] = mode ? 0x40 : 0x00;
    tx[1] = dat;

    while (!(DL_I2C_getControllerStatus(OLED_INST) & DL_I2C_CONTROLLER_STATUS_IDLE));
    DL_I2C_fillControllerTXFIFO(OLED_INST, tx, 2);
    DL_I2C_startControllerTransfer(OLED_INST, OLED_ADDR,
        DL_I2C_CONTROLLER_DIRECTION_TX, 2);
    while (!(DL_I2C_getControllerStatus(OLED_INST) & DL_I2C_CONTROLLER_STATUS_IDLE));
}

void OLED_DisPlay_On(void)
{
    OLED_WR_Byte(0x8D, OLED_CMD);
    OLED_WR_Byte(0x14, OLED_CMD);
    OLED_WR_Byte(0xAF, OLED_CMD);
}

void OLED_DisPlay_Off(void)
{
    OLED_WR_Byte(0x8D, OLED_CMD);
    OLED_WR_Byte(0x10, OLED_CMD);
    OLED_WR_Byte(0xAE, OLED_CMD);
}

void OLED_Refresh(void)
{
    u8 i, n;
    for (i = 0; i < 8; i++) {
        OLED_WR_Byte(0xb0 + i, OLED_CMD);
        OLED_WR_Byte(0x00, OLED_CMD);
        OLED_WR_Byte(0x10, OLED_CMD);
        for (n = 0; n < 128; n++)
            OLED_WR_Byte(OLED_GRAM[n][i], OLED_DATA);
    }
}

void OLED_Clear(void)
{
    u8 i, n;
    for (i = 0; i < 8; i++)
        for (n = 0; n < 128; n++)
            OLED_GRAM[n][i] = 0;
    OLED_Refresh();
}

void OLED_DrawPoint(u8 x, u8 y)
{
    u8 i, m;
    if (x >= 128 || y >= 64) return;
    i = y / 8;
    m = 1 << (y % 8);
    OLED_GRAM[x][i] |= m;
}

void OLED_ClearPoint(u8 x, u8 y)
{
    u8 i, m;
    if (x >= 128 || y >= 64) return;
    i = y / 8;
    m = 1 << (y % 8);
    OLED_GRAM[x][i] &= ~m;
}

void OLED_ShowChar(u8 x, u8 y, u8 chr, u8 size1)
{
    u8 i, m, temp, size2, chr1;
    u8 y0 = y;
    size2 = (size1 / 8 + ((size1 % 8) ? 1 : 0)) * (size1 / 2);
    chr1 = chr - ' ';
    for (i = 0; i < size2; i++) {
        if (size1 == 12)      temp = asc2_1206[chr1][i];
        else if (size1 == 16) temp = asc2_1608[chr1][i];
        else if (size1 == 24) temp = asc2_2412[chr1][i];
        else return;
        for (m = 0; m < 8; m++) {
            if (temp & 0x80) OLED_DrawPoint(x, y);
            else OLED_ClearPoint(x, y);
            temp <<= 1;
            y++;
            if ((y - y0) == size1) { y = y0; x++; break; }
        }
    }
}

void OLED_ShowString(u8 x, u8 y, u8 *chr, u8 size1)
{
    while (*chr >= ' ' && *chr <= '~') {
        OLED_ShowChar(x, y, *chr, size1);
        x += size1 / 2;
        if (x > 128 - size1) { x = 0; y += size1; }
        chr++;
    }
}

static u32 OLED_Pow(u8 m, u8 n)
{
    u32 r = 1;
    while (n--) r *= m;
    return r;
}

void OLED_ShowNum(u8 x, u8 y, u32 num, u8 len, u8 size1)
{
    u8 t;
    for (t = 0; t < len; t++)
        OLED_ShowChar(x + (size1 / 2) * t, y,
            '0' + (num / OLED_Pow(10, len - t - 1)) % 10, size1);
}

void OLED_Init(void)
{
    delay_ms(100);

    OLED_WR_Byte(0xAE, OLED_CMD); // display off
    OLED_WR_Byte(0x00, OLED_CMD);
    OLED_WR_Byte(0x10, OLED_CMD);
    OLED_WR_Byte(0x40, OLED_CMD);
    OLED_WR_Byte(0x81, OLED_CMD); // contrast
    OLED_WR_Byte(0xCF, OLED_CMD);
    OLED_WR_Byte(0xA1, OLED_CMD); // segment remap
    OLED_WR_Byte(0xC8, OLED_CMD); // COM scan
    OLED_WR_Byte(0xA6, OLED_CMD); // normal
    OLED_WR_Byte(0xA8, OLED_CMD); // multiplex
    OLED_WR_Byte(0x3F, OLED_CMD); // 1/64 duty
    OLED_WR_Byte(0xD3, OLED_CMD);
    OLED_WR_Byte(0x00, OLED_CMD);
    OLED_WR_Byte(0xD5, OLED_CMD);
    OLED_WR_Byte(0x80, OLED_CMD);
    OLED_WR_Byte(0xD9, OLED_CMD); // pre-charge
    OLED_WR_Byte(0xF1, OLED_CMD);
    OLED_WR_Byte(0xDA, OLED_CMD); // COM pins
    OLED_WR_Byte(0x12, OLED_CMD);
    OLED_WR_Byte(0xDB, OLED_CMD); // VCOMH
    OLED_WR_Byte(0x40, OLED_CMD);
    OLED_WR_Byte(0x20, OLED_CMD);
    OLED_WR_Byte(0x02, OLED_CMD); // page addressing
    OLED_WR_Byte(0x8D, OLED_CMD); // charge pump
    OLED_WR_Byte(0x14, OLED_CMD);
    OLED_WR_Byte(0xA4, OLED_CMD);
    OLED_WR_Byte(0xA6, OLED_CMD);
    OLED_WR_Byte(0xAF, OLED_CMD); // display on

    OLED_Clear();
}
