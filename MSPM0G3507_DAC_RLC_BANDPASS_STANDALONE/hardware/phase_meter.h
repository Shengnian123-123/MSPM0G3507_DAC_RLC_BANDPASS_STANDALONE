#ifndef HW_PHASE_METER_H
#define HW_PHASE_METER_H

#include <stdbool.h>
#include <stdint.h>
#include "main.h"

#define PHASE_METER_A_PORT       (GPIOB)
#define PHASE_METER_A_PIN        (DL_GPIO_PIN_20)
#define PHASE_METER_A_IOMUX      (IOMUX_PINCM48)
#define PHASE_METER_A_PIN_NAME   "PB20"
#define PHASE_METER_B_PORT       (GPIOB)
#define PHASE_METER_B_PIN        (DL_GPIO_PIN_21)
#define PHASE_METER_B_IOMUX      (IOMUX_PINCM49)
#define PHASE_METER_B_PIN_NAME   "PB21"

typedef struct {
    bool valid;
    int16_t signedDeg;
    uint16_t phaseDeg;
    uint32_t deltaUs;
    uint32_t periodUs;
    uint32_t edgeCountA;
    uint32_t edgeCountB;
} PhaseMeter_Result_t;

void PhaseMeter_Init(void);
bool PhaseMeter_Read(PhaseMeter_Result_t *result);
void PhaseMeter_GpioIsr(uint32_t status, uint64_t timestampTicks);

#endif
