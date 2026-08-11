#include "pwm_out.h"
#include "delay.h"
#include "gpio_io.h"
#include "tim.h"

#define PWM_OUT_SOFT_MAX_FREQ_HZ (1000U)
#define PWM_OUT_SOFT_MAX_MS      (5000U)

Periph_Status_t PwmOut_StartSoftLed(uint32_t frequencyHz,
    uint16_t dutyPermille, uint32_t durationMs)
{
    uint32_t periodUs;
    uint32_t onUs;
    uint32_t offUs;
    uint32_t startMs;

    if ((frequencyHz == 0U) || (frequencyHz > PWM_OUT_SOFT_MAX_FREQ_HZ) ||
        (dutyPermille > 1000U) || (durationMs > PWM_OUT_SOFT_MAX_MS)) {
        return PERIPH_STATUS_BAD_PARAM;
    }

    periodUs = 1000000UL / frequencyHz;
    onUs = (periodUs * dutyPermille) / 1000UL;
    offUs = periodUs - onUs;
    startMs = Timer_GetTickMs();

    while ((Timer_GetTickMs() - startMs) < durationMs) {
        if (onUs > 0U) {
            GPIOIO_LedOn();
            Delay_Us(onUs);
        }
        if (offUs > 0U) {
            GPIOIO_LedOff();
            Delay_Us(offUs);
        }
    }

    return PERIPH_STATUS_OK;
}

Periph_Status_t PwmOut_StartHardware(uint32_t frequencyHz,
    uint16_t dutyPermille)
{
    (void) frequencyHz;
    (void) dutyPermille;
    return PERIPH_STATUS_NOT_CONFIGURED;
}

Periph_Status_t PwmOut_StopHardware(void)
{
    return PERIPH_STATUS_NOT_CONFIGURED;
}
