#include "delay.h"

void Delay_Us(uint32_t delayUs)
{
    const uint32_t cyclesPerUs = CPUCLK_FREQ / 1000000UL;

    while (delayUs > 0U) {
        delay_cycles(cyclesPerUs);
        delayUs--;
    }
}

void Delay_Ms(uint32_t delayMs)
{
    while (delayMs > 0U) {
        Delay_Us(1000U);
        delayMs--;
    }
}
