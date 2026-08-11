#ifndef HW_PWM_OUT_H
#define HW_PWM_OUT_H

#include <stdint.h>
#include "peripheral_status.h"

Periph_Status_t PwmOut_StartSoftLed(uint32_t frequencyHz,
    uint16_t dutyPermille, uint32_t durationMs);
Periph_Status_t PwmOut_StartHardware(uint32_t frequencyHz,
    uint16_t dutyPermille);
Periph_Status_t PwmOut_StopHardware(void);

#endif
