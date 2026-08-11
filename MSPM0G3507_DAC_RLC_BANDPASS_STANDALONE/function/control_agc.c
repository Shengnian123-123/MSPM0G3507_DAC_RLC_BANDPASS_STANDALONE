#include "control_agc.h"

static uint8_t ClampU8(uint8_t value, uint8_t minValue, uint8_t maxValue)
{
    if (value < minValue) {
        return minValue;
    }
    if (value > maxValue) {
        return maxValue;
    }
    return value;
}

void ControlAgc_Init(ControlAgc_t *agc, uint16_t targetPkpkRaw,
    uint16_t hysteresisRaw, uint8_t gainMin, uint8_t gainMax,
    uint8_t initialGain)
{
    uint32_t highThreshold;

    if (agc == 0) {
        return;
    }

    if (gainMin > gainMax) {
        uint8_t temp = gainMin;
        gainMin = gainMax;
        gainMax = temp;
    }

    agc->targetPkpkRaw = targetPkpkRaw;
    agc->lowThresholdRaw = (targetPkpkRaw > hysteresisRaw) ?
        (uint16_t) (targetPkpkRaw - hysteresisRaw) :
        0U;
    highThreshold = (uint32_t) targetPkpkRaw + hysteresisRaw;
    agc->highThresholdRaw = (highThreshold > 0xFFFFUL) ?
        0xFFFFU : (uint16_t) highThreshold;
    agc->gainStep = 1U;
    agc->gainMin = gainMin;
    agc->gainMax = gainMax;
    agc->gain = ClampU8(initialGain, gainMin, gainMax);
}

ControlAgc_Action_t ControlAgc_Update(ControlAgc_t *agc, uint16_t pkpkRaw)
{
    if (agc == 0) {
        return CONTROL_AGC_HOLD;
    }

    if ((pkpkRaw < agc->lowThresholdRaw) && (agc->gain < agc->gainMax)) {
        agc->gain += agc->gainStep;
        if (agc->gain > agc->gainMax) {
            agc->gain = agc->gainMax;
        }
        return CONTROL_AGC_INCREASE;
    }

    if ((pkpkRaw > agc->highThresholdRaw) && (agc->gain > agc->gainMin)) {
        agc->gain -= agc->gainStep;
        if (agc->gain < agc->gainMin) {
            agc->gain = agc->gainMin;
        }
        return CONTROL_AGC_DECREASE;
    }

    return CONTROL_AGC_HOLD;
}
