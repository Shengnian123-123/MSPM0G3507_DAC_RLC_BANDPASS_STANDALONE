#ifndef CONTROL_AGC_H
#define CONTROL_AGC_H

#include <stdint.h>

typedef enum {
    CONTROL_AGC_HOLD = 0,
    CONTROL_AGC_INCREASE,
    CONTROL_AGC_DECREASE,
} ControlAgc_Action_t;

typedef struct {
    uint16_t targetPkpkRaw;
    uint16_t lowThresholdRaw;
    uint16_t highThresholdRaw;
    uint8_t gainStep;
    uint8_t gainMin;
    uint8_t gainMax;
    uint8_t gain;
} ControlAgc_t;

void ControlAgc_Init(ControlAgc_t *agc, uint16_t targetPkpkRaw,
    uint16_t hysteresisRaw, uint8_t gainMin, uint8_t gainMax,
    uint8_t initialGain);
ControlAgc_Action_t ControlAgc_Update(ControlAgc_t *agc, uint16_t pkpkRaw);

#endif
