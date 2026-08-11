#ifndef ADC_FML_H
#define ADC_FML_H

#include <stdbool.h>
#include <stdint.h>
#include "adc_bll.h"

#define ADC_FML_LOW_LIMIT_RAW     (1000U)
#define ADC_FML_HIGH_LIMIT_RAW    (3000U)
#define ADC_FML_VREF_MV           (3300U)
#define ADC_FML_FULL_SCALE_RAW    (4095U)

void ADC_Fml_Reset(void);
bool ADC_Fml_ReadFiltered(uint16_t *raw);
uint16_t ADC_Fml_RawToMillivolt(uint16_t raw);
ADC_Bll_State_t ADC_Fml_DetectWindow(uint16_t raw);

#endif
