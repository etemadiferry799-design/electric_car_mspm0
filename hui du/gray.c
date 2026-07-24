#include "gray.h"

#ifndef GRAY_AD0_PORT
#define GRAY_AD0_PORT GPIOA
#define GRAY_AD0_PIN  DL_GPIO_PIN_0
#endif

#ifndef GRAY_AD1_PORT
#define GRAY_AD1_PORT GPIOA
#define GRAY_AD1_PIN  DL_GPIO_PIN_1
#endif

#ifndef GRAY_AD2_PORT
#define GRAY_AD2_PORT GPIOA
#define GRAY_AD2_PIN  DL_GPIO_PIN_2
#endif

static GrayAdcReadFn g_adcReadFn;

static void Gray_DelayForMux(void)
{
    for (volatile uint32_t i = 0; i < 800U; i++) {
        __NOP();
    }
}

static void Gray_WriteAddressPin(GPIO_Regs *port, uint32_t pin, uint8_t level)
{
    if (level) {
        DL_GPIO_setPins(port, pin);
    } else {
        DL_GPIO_clearPins(port, pin);
    }
}

void Gray_Init(GrayAdcReadFn adcReadFn)
{
    g_adcReadFn = adcReadFn;
    Gray_SelectChannel(0U);
}

void Gray_SelectChannel(uint8_t channel)
{
    channel &= 0x07U;
    Gray_WriteAddressPin(GRAY_AD0_PORT, GRAY_AD0_PIN, channel & 0x01U);
    Gray_WriteAddressPin(GRAY_AD1_PORT, GRAY_AD1_PIN, channel & 0x02U);
    Gray_WriteAddressPin(GRAY_AD2_PORT, GRAY_AD2_PIN, channel & 0x04U);
    Gray_DelayForMux();
}

uint16_t Gray_ReadChannel(uint8_t channel)
{
    Gray_SelectChannel(channel);
    return (g_adcReadFn != 0) ? g_adcReadFn() : 0U;
}

void Gray_ReadAll(uint16_t values[GRAY_CHANNEL_COUNT])
{
    for (uint8_t i = 0U; i < GRAY_CHANNEL_COUNT; i++) {
        values[i] = Gray_ReadChannel(i);
    }
}

int16_t Gray_CalcPositionError(const uint16_t values[GRAY_CHANNEL_COUNT], uint16_t threshold, uint8_t blackIsHigh)
{
    static const int16_t weights[GRAY_CHANNEL_COUNT] = {-35, -25, -15, -5, 5, 15, 25, 35};
    int32_t weightedSum = 0;
    int32_t activeCount = 0;

    for (uint8_t i = 0U; i < GRAY_CHANNEL_COUNT; i++) {
        uint8_t onLine = blackIsHigh ? (values[i] > threshold) : (values[i] < threshold);
        if (onLine) {
            weightedSum += weights[i];
            activeCount++;
        }
    }

    if (activeCount == 0) {
        return 0;
    }

    return (int16_t)(weightedSum / activeCount);
}
