#ifndef COMPONENT_AD9833_H
#define COMPONENT_AD9833_H

#include <stdbool.h>
#include <stdint.h>

#define AD9833_DEFAULT_MCLK_HZ (25000000UL)
#define AD9833_MAX_PHASE_DEG   (359U)

typedef enum {
    AD9833_WAVE_SINE = 0,
    AD9833_WAVE_TRIANGLE,
    AD9833_WAVE_SQUARE,
} AD9833_Waveform_t;

typedef enum {
    AD9833_CHANNEL_ALL = 0,
    AD9833_CHANNEL_1,
    AD9833_CHANNEL_2,
} AD9833_Channel_t;

void AD9833_Init(void);
void AD9833_SetMclkHz(uint32_t mclkHz);
uint32_t AD9833_GetMclkHz(void);
void AD9833_SetMclkChannel(AD9833_Channel_t channel, uint32_t mclkHz);
uint32_t AD9833_GetMclkChannel(AD9833_Channel_t channel);
uint32_t AD9833_GetMaxOutputHz(void);
uint32_t AD9833_GetMaxOutputHzChannel(AD9833_Channel_t channel);
bool AD9833_SetOutput(AD9833_Waveform_t waveform, uint32_t frequencyHz,
    uint16_t phaseDeg);
bool AD9833_SetOutputChannel(AD9833_Channel_t channel,
    AD9833_Waveform_t waveform, uint32_t frequencyHz, uint16_t phaseDeg);
bool AD9833_SetAmplitude(uint8_t amplitude);
bool AD9833_SetAmplitudeChannel(AD9833_Channel_t channel, uint8_t amplitude);
void AD9833_ResetOutput(void);
void AD9833_ResetOutputChannel(AD9833_Channel_t channel);
bool AD9833_SelfTestPattern(void);

#endif
