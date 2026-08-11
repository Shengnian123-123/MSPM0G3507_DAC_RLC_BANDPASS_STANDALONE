#ifndef HW_ADC_DMA_SCOPE_H
#define HW_ADC_DMA_SCOPE_H

#include <stdbool.h>
#include <stdint.h>

#define ADC_DMA_SCOPE_MAX_SAMPLES       (1024U)
#define ADC_DMA_SCOPE_DEFAULT_RATE_HZ   (10000UL)
#define ADC_DMA_SCOPE_MIN_RATE_HZ       (10UL)
#define ADC_DMA_SCOPE_BLOCKING_MAX_RATE_HZ (200000UL)
#define ADC_DMA_SCOPE_MAX_RATE_HZ       (2000000UL)
#define ADC_DMA_SCOPE_VOFA_MAX_RATE_HZ  (200UL)

typedef struct {
    uint32_t sampleRateHz;
    uint16_t sampleCount;
    bool usedDma;
    uint16_t samples[ADC_DMA_SCOPE_MAX_SAMPLES];
} AdcDmaScope_Frame_t;

bool AdcDmaScope_Capture(uint32_t sampleRateHz, uint16_t sampleCount,
    AdcDmaScope_Frame_t *frame);
bool AdcDmaScope_StartVofa(uint32_t sampleRateHz);
void AdcDmaScope_StopVofa(void);
bool AdcDmaScope_IsVofaRunning(void);
void AdcDmaScope_Proc(void);

#endif
