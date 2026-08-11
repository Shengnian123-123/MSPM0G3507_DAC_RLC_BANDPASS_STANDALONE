#ifndef HW_ADC_H
#define HW_ADC_H

#include <stdbool.h>
#include <stdint.h>
#include "main.h"

typedef struct {
    ADC12_Regs *inst;
    DL_ADC12_MEM_IDX memIdx;
    uint32_t resultLoadedMask;
} ADC_ChannelConfig_t;

void ADC_Start(void);
bool ADC_ReadOnceBlocking(const ADC_ChannelConfig_t *config, uint16_t *raw,
    uint32_t timeoutLoops);
bool ADC_SampleBlocking(uint16_t *raw, uint32_t timeoutLoops);

#endif
