#include "adc_bll.h"
#include "adc_fml.h"

void ADC_Bll_Init(void)
{
    ADC_Fml_Reset();
}

bool ADC_Bll_SampleAndDetect(ADC_Bll_Result_t *result)
{
    uint16_t raw;

    if ((result == 0) || !ADC_Fml_ReadFiltered(&raw)) {
        return false;
    }

    result->raw       = raw;
    result->millivolt = ADC_Fml_RawToMillivolt(raw);
    result->state     = ADC_Fml_DetectWindow(raw);

    return true;
}
