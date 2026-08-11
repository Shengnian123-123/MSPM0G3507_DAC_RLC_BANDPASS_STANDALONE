#include "adc_apl.h"
#include "adc.h"
#include "tim.h"

static ADC_Bll_Result_t g_adcResult;
static uint32_t g_lastSampleTick;

void ADC_Apl_Init(void)
{
    ADC_Bll_Init();
    ADC_Start();
    g_lastSampleTick = Timer_GetTickMs();
}

void ADC_Apl_Proc(void)
{
    if ((Timer_GetTickMs() - g_lastSampleTick) < 10U) {
        return;
    }

    g_lastSampleTick = Timer_GetTickMs();
    if (ADC_Bll_SampleAndDetect(&g_adcResult)) {
        if (g_adcResult.state == ADC_BLL_STATE_NORMAL) {
            DL_GPIO_setPins(GPIO_STATUS_PORT, GPIO_STATUS_LED_PIN);
        } else {
            DL_GPIO_clearPins(GPIO_STATUS_PORT, GPIO_STATUS_LED_PIN);
        }
    }
}

const ADC_Bll_Result_t *ADC_Apl_GetResult(void)
{
    return &g_adcResult;
}
