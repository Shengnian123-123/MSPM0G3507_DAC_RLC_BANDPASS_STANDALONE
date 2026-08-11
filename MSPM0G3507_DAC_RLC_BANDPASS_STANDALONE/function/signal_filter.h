#ifndef SIGNAL_FILTER_H
#define SIGNAL_FILTER_H

#include <stdint.h>

void SignalFilter_Copy(const uint16_t *input, uint16_t *output, uint16_t count);
void SignalFilter_MovingAverage3(const uint16_t *input, uint16_t *output,
    uint16_t count);
void SignalFilter_Median3(const uint16_t *input, uint16_t *output,
    uint16_t count);
void SignalFilter_IirLowPass(const uint16_t *input, uint16_t *output,
    uint16_t count, uint8_t shift);

#endif
