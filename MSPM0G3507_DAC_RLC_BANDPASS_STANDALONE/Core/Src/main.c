#include "main.h"
#include "dac_waveform.h"

int main(void)
{
    SYSCFG_DL_init();
    DacWaveform_Init();

    while (1) {
        __WFI();
    }
}

void Error_Handler(void)
{
    while (1) {
        __WFI();
    }
}
