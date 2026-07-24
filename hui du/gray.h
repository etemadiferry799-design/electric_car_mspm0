#ifndef __GRAY_H
#define __GRAY_H

#include "ti_msp_dl_config.h"
#include <stdint.h>

#define GRAY_CHANNEL_COUNT 8U

typedef uint16_t (*GrayAdcReadFn)(void);

void Gray_Init(GrayAdcReadFn adcReadFn);
void Gray_SelectChannel(uint8_t channel);
uint16_t Gray_ReadChannel(uint8_t channel);
void Gray_ReadAll(uint16_t values[GRAY_CHANNEL_COUNT]);
int16_t Gray_CalcPositionError(const uint16_t values[GRAY_CHANNEL_COUNT], uint16_t threshold, uint8_t blackIsHigh);

#endif
