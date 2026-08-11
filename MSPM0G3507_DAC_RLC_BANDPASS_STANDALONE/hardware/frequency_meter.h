#ifndef HW_FREQUENCY_METER_H
#define HW_FREQUENCY_METER_H

#include <stdbool.h>
#include <stdint.h>

#define FREQUENCY_METER_INPUT_PORT       (GPIOB)
#define FREQUENCY_METER_INPUT_PIN        (DL_GPIO_PIN_20)
#define FREQUENCY_METER_INPUT_IOMUX      (IOMUX_PINCM48)
#define FREQUENCY_METER_INPUT_PIN_NAME   "PB20"

typedef struct {
    bool valid;
    uint32_t frequencyHz;
    uint32_t periodUs;
    uint16_t dutyPermille;
    uint32_t edgeCount;
    uint32_t ageMs;
} FrequencyMeter_Result_t;

void FrequencyMeter_Init(void);
bool FrequencyMeter_Read(FrequencyMeter_Result_t *result);
void FrequencyMeter_GpioIsr(uint32_t status, uint64_t timestampTicks);
uint64_t FrequencyMeter_ReadTimestampTicks(void);
uint32_t FrequencyMeter_GetTicksPerSecond(void);
uint32_t FrequencyMeter_GetLastPeriodTicks(void);
uint64_t FrequencyMeter_GetLastRiseTicks(void);

#endif
