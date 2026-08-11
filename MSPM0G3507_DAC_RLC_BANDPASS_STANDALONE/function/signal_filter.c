#include "signal_filter.h"

static uint16_t MedianOf3(uint16_t a, uint16_t b, uint16_t c)
{
    if (a > b) {
        uint16_t t = a;
        a = b;
        b = t;
    }
    if (b > c) {
        uint16_t t = b;
        b = c;
        c = t;
    }
    if (a > b) {
        b = a;
    }
    return b;
}

void SignalFilter_Copy(const uint16_t *input, uint16_t *output, uint16_t count)
{
    if ((input == 0) || (output == 0)) {
        return;
    }

    for (uint16_t i = 0U; i < count; i++) {
        output[i] = input[i];
    }
}

void SignalFilter_MovingAverage3(const uint16_t *input, uint16_t *output,
    uint16_t count)
{
    if ((input == 0) || (output == 0) || (count == 0U)) {
        return;
    }

    if (count < 3U) {
        SignalFilter_Copy(input, output, count);
        return;
    }

    output[0] = input[0];
    for (uint16_t i = 1U; i < (count - 1U); i++) {
        output[i] = (uint16_t) (((uint32_t) input[i - 1U] + input[i] +
                                    input[i + 1U]) /
            3U);
    }
    output[count - 1U] = input[count - 1U];
}

void SignalFilter_Median3(const uint16_t *input, uint16_t *output,
    uint16_t count)
{
    if ((input == 0) || (output == 0) || (count == 0U)) {
        return;
    }

    if (count < 3U) {
        SignalFilter_Copy(input, output, count);
        return;
    }

    output[0] = input[0];
    for (uint16_t i = 1U; i < (count - 1U); i++) {
        output[i] = MedianOf3(input[i - 1U], input[i], input[i + 1U]);
    }
    output[count - 1U] = input[count - 1U];
}

void SignalFilter_IirLowPass(const uint16_t *input, uint16_t *output,
    uint16_t count, uint8_t shift)
{
    int32_t state;

    if ((input == 0) || (output == 0) || (count == 0U)) {
        return;
    }

    if ((shift == 0U) || (shift > 8U)) {
        SignalFilter_Copy(input, output, count);
        return;
    }

    state     = (int32_t) input[0];
    output[0] = input[0];
    for (uint16_t i = 1U; i < count; i++) {
        state += (((int32_t) input[i]) - state) >> shift;
        if (state < 0) {
            state = 0;
        } else if (state > 0xFFFFL) {
            state = 0xFFFFL;
        }
        output[i] = (uint16_t) state;
    }
}
