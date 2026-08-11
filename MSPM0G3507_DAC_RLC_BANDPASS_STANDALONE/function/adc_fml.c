#include "adc_fml.h"
#include "adc.h"

void ADC_Fml_Reset(void)
{
}

bool ADC_Fml_ReadFiltered(uint16_t *raw)
{
    uint32_t sum = 0;
    uint16_t sample;

    if (raw == 0) {
        return false;
    }

    for (uint8_t i = 0; i < 4U; i++) {
        if (!ADC_SampleBlocking(&sample, 20000U)) {
            return false;
        }
        sum += sample;
    }

    *raw = (uint16_t) (sum / 4U);
    return true;
}

uint16_t ADC_Fml_RawToMillivolt(uint16_t raw)
{
    return (uint16_t) (((uint32_t) raw * ADC_FML_VREF_MV) / ADC_FML_FULL_SCALE_RAW);
}

ADC_Bll_State_t ADC_Fml_DetectWindow(uint16_t raw)
{
    if (raw < ADC_FML_LOW_LIMIT_RAW) {
        return ADC_BLL_STATE_LOW;
    }

    if (raw > ADC_FML_HIGH_LIMIT_RAW) {
        return ADC_BLL_STATE_HIGH;
    }

    return ADC_BLL_STATE_NORMAL;
}
