#include "adc.h"

static const ADC_ChannelConfig_t g_adc0Channel = {
    ADC12_0_INST,
    ADC12_0_ADCMEM_0,
    DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED,
};

void ADC_Start(void)
{
    DL_ADC12_enableConversions(ADC12_0_INST);
}

bool ADC_ReadOnceBlocking(const ADC_ChannelConfig_t *config, uint16_t *raw,
    uint32_t timeoutLoops)
{
    if ((config == 0) || (raw == 0)) {
        return false;
    }

    DL_ADC12_stopConversion(config->inst);
    DL_ADC12_disableDMA(config->inst);
    DL_ADC12_disableFIFO(config->inst);
    DL_ADC12_clearInterruptStatus(config->inst, config->resultLoadedMask);
    DL_ADC12_enableConversions(config->inst);
    DL_ADC12_startConversion(config->inst);
#if defined(ADC_SAMPLE_TIMER_INST)
    DL_TimerG_stopCounter(ADC_SAMPLE_TIMER_INST);
    DL_TimerG_setTimerCount(ADC_SAMPLE_TIMER_INST,
        ADC_SAMPLE_TIMER_INST_LOAD_VALUE);
    DL_TimerG_startCounter(ADC_SAMPLE_TIMER_INST);
#endif

    while ((DL_ADC12_getRawInterruptStatus(
                config->inst, config->resultLoadedMask) &
              config->resultLoadedMask) == 0U) {
        if (timeoutLoops-- == 0U) {
#if defined(ADC_SAMPLE_TIMER_INST)
            DL_TimerG_stopCounter(ADC_SAMPLE_TIMER_INST);
#endif
            DL_ADC12_stopConversion(config->inst);
            return false;
        }
        __NOP();
    }

    *raw = DL_ADC12_getMemResult(config->inst, config->memIdx);
#if defined(ADC_SAMPLE_TIMER_INST)
    DL_TimerG_stopCounter(ADC_SAMPLE_TIMER_INST);
#endif
    DL_ADC12_stopConversion(config->inst);
    DL_ADC12_clearInterruptStatus(config->inst, config->resultLoadedMask);
    DL_ADC12_enableConversions(config->inst);
    return true;
}

bool ADC_SampleBlocking(uint16_t *raw, uint32_t timeoutLoops)
{
    return ADC_ReadOnceBlocking(&g_adc0Channel, raw, timeoutLoops);
}
