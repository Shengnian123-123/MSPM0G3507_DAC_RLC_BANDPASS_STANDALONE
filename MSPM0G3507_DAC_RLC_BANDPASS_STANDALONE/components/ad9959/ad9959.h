#ifndef COMPONENT_AD9959_H
#define COMPONENT_AD9959_H

#include <stdbool.h>
#include <stdint.h>

#define AD9959_CHANNEL_COUNT       (4U)
#define AD9959_DEFAULT_SYSCLK_HZ   (500000000UL)
#define AD9959_MAX_OUTPUT_HZ       (200000000UL)
#define AD9959_MAX_AMPLITUDE       (1023U)

typedef enum {
    AD9959_CH0 = 0,
    AD9959_CH1 = 1,
    AD9959_CH2 = 2,
    AD9959_CH3 = 3,
} AD9959_Channel_t;

void AD9959_Init(void);
void AD9959_SetSysclkHz(uint32_t sysclkHz);
uint32_t AD9959_GetSysclkHz(void);
void AD9959_PowerDown(bool enable);
bool AD9959_SetSingleTone(AD9959_Channel_t channel, uint32_t frequencyHz,
    uint16_t phaseDeg, uint16_t amplitude);
bool AD9959_SetFrequency(AD9959_Channel_t channel, uint32_t frequencyHz);
bool AD9959_SetPhase(AD9959_Channel_t channel, uint16_t phaseDeg);
bool AD9959_SetAmplitude(AD9959_Channel_t channel, uint16_t amplitude);
bool AD9959_SetProfilePins(uint8_t value);
bool AD9959_SelfTestPattern(void);

#endif
