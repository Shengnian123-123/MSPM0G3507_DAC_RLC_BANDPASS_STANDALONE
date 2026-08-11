#ifndef HW_CAPTURE_INPUT_H
#define HW_CAPTURE_INPUT_H

#include <stdint.h>
#include "peripheral_status.h"

typedef struct {
    uint32_t frequencyHz;
    uint16_t dutyPermille;
    uint16_t phaseDeg;
} CaptureInput_Result_t;

Periph_Status_t CaptureInput_Read(CaptureInput_Result_t *result);

#endif
