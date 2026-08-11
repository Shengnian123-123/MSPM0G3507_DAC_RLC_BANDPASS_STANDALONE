#ifndef ADC_BLL_H
#define ADC_BLL_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    ADC_BLL_STATE_LOW = 0,
    ADC_BLL_STATE_NORMAL,
    ADC_BLL_STATE_HIGH
} ADC_Bll_State_t;

typedef struct {
    uint16_t raw;
    uint16_t millivolt;
    ADC_Bll_State_t state;
} ADC_Bll_Result_t;

void ADC_Bll_Init(void);
bool ADC_Bll_SampleAndDetect(ADC_Bll_Result_t *result);

#endif
