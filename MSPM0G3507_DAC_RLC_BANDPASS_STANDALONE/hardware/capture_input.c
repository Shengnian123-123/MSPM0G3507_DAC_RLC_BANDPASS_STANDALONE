#include "capture_input.h"

Periph_Status_t CaptureInput_Read(CaptureInput_Result_t *result)
{
    if (result != 0) {
        result->frequencyHz = 0U;
        result->dutyPermille = 0U;
        result->phaseDeg = 0U;
    }
    return PERIPH_STATUS_NOT_CONFIGURED;
}
