#ifndef ADC_APL_H
#define ADC_APL_H

#include <stdint.h>
#include "adc_bll.h"

void ADC_Apl_Init(void);
void ADC_Apl_Proc(void);
const ADC_Bll_Result_t *ADC_Apl_GetResult(void);

#endif
