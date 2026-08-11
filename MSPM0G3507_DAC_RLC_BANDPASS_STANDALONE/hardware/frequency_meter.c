#include "frequency_meter.h"
#include "phase_meter.h"
#include "main.h"
#include "tim.h"

#define FREQUENCY_METER_TIMER_DIVIDER       (31UL)
#define FREQUENCY_METER_TIMER_TICKS_PER_SEC (CPUCLK_FREQ / FREQUENCY_METER_TIMER_DIVIDER)
#define FREQUENCY_METER_TIMER_PERIOD_TICKS  (TIMER_0_INST_LOAD_VALUE + 1UL)

static volatile uint64_t g_lastRiseTicks;
static volatile uint64_t g_previousRiseTicks;
static volatile uint64_t g_lastFallTicks;
static volatile uint32_t g_lastPeriodTicks;
static volatile uint32_t g_lastHighTicks;
static volatile uint32_t g_edgeCount;
static volatile bool g_hasRise;
static volatile bool g_hasPeriod;
static volatile bool g_hasHigh;

static bool ReadInputHigh(void)
{
    return ((DL_GPIO_readPins(FREQUENCY_METER_INPUT_PORT,
                 FREQUENCY_METER_INPUT_PIN) &
             FREQUENCY_METER_INPUT_PIN) != 0U);
}

uint64_t FrequencyMeter_ReadTimestampTicks(void)
{
    uint32_t msA;
    uint32_t msB;
    uint32_t count;
    uint32_t elapsed;
    bool zeroPending;

    do {
        msA = Timer_GetTickMs();
        count = DL_TimerG_getTimerCount(TIMER_0_INST);
        zeroPending =
            ((DL_TimerG_getRawInterruptStatus(TIMER_0_INST,
                  DL_TIMERG_INTERRUPT_ZERO_EVENT) &
                 DL_TIMERG_INTERRUPT_ZERO_EVENT) != 0U);
        msB = Timer_GetTickMs();
    } while (msA != msB);

    if (count > TIMER_0_INST_LOAD_VALUE) {
        elapsed = 0U;
    } else {
        elapsed = TIMER_0_INST_LOAD_VALUE - count;
    }

    if (zeroPending) {
        msA++;
    }

    return ((uint64_t) msA * FREQUENCY_METER_TIMER_PERIOD_TICKS) + elapsed;
}

uint32_t FrequencyMeter_GetTicksPerSecond(void)
{
    return FREQUENCY_METER_TIMER_TICKS_PER_SEC;
}

uint32_t FrequencyMeter_GetLastPeriodTicks(void)
{
    return g_lastPeriodTicks;
}

uint64_t FrequencyMeter_GetLastRiseTicks(void)
{
    return g_lastRiseTicks;
}

void FrequencyMeter_Init(void)
{
    DL_GPIO_initDigitalInputFeatures(FREQUENCY_METER_INPUT_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_DOWN,
        DL_GPIO_HYSTERESIS_ENABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_setUpperPinsPolarity(FREQUENCY_METER_INPUT_PORT,
        DL_GPIO_PIN_20_EDGE_RISE_FALL);
    DL_GPIO_clearInterruptStatus(FREQUENCY_METER_INPUT_PORT,
        FREQUENCY_METER_INPUT_PIN);
    DL_GPIO_enableInterrupt(FREQUENCY_METER_INPUT_PORT,
        FREQUENCY_METER_INPUT_PIN);
    NVIC_ClearPendingIRQ(GPIOB_INT_IRQn);
    NVIC_EnableIRQ(GPIOB_INT_IRQn);
}

void FrequencyMeter_GpioIsr(uint32_t status, uint64_t timestampTicks)
{
    if ((status & FREQUENCY_METER_INPUT_PIN) == 0U) {
        return;
    }

    g_edgeCount++;
    if (ReadInputHigh()) {
        g_previousRiseTicks = g_lastRiseTicks;
        g_lastRiseTicks = timestampTicks;
        if (g_hasRise && (g_lastRiseTicks > g_previousRiseTicks)) {
            g_lastPeriodTicks =
                (uint32_t) (g_lastRiseTicks - g_previousRiseTicks);
            g_hasPeriod = true;
        }
        g_hasRise = true;
    } else {
        g_lastFallTicks = timestampTicks;
        if (g_hasRise && (g_lastFallTicks > g_lastRiseTicks)) {
            g_lastHighTicks = (uint32_t) (g_lastFallTicks - g_lastRiseTicks);
            g_hasHigh = true;
        }
    }
}

bool FrequencyMeter_Read(FrequencyMeter_Result_t *result)
{
    uint32_t periodTicks;
    uint32_t highTicks;
    uint32_t edgeCount;
    uint64_t lastRise;
    uint64_t now;
    bool hasHigh;

    if (result == 0) {
        return false;
    }

    __disable_irq();
    periodTicks = g_lastPeriodTicks;
    highTicks = g_lastHighTicks;
    edgeCount = g_edgeCount;
    lastRise = g_lastRiseTicks;
    hasHigh = g_hasHigh;
    result->valid = g_hasPeriod;
    __enable_irq();

    result->edgeCount = edgeCount;
    result->frequencyHz = 0U;
    result->periodUs = 0U;
    result->dutyPermille = 0U;
    now = FrequencyMeter_ReadTimestampTicks();
    result->ageMs = (uint32_t) (((now - lastRise) * 1000ULL) /
                                FREQUENCY_METER_TIMER_TICKS_PER_SEC);

    if (!result->valid || (periodTicks == 0U)) {
        return false;
    }

    result->frequencyHz =
        (uint32_t) (FREQUENCY_METER_TIMER_TICKS_PER_SEC / periodTicks);
    result->periodUs =
        (uint32_t) ((periodTicks * 1000000ULL) /
                    FREQUENCY_METER_TIMER_TICKS_PER_SEC);
    if (hasHigh && (highTicks <= periodTicks)) {
        result->dutyPermille =
            (uint16_t) ((highTicks * 1000UL) / periodTicks);
    }

    return true;
}

void GROUP1_IRQHandler(void)
{
    uint32_t status = DL_GPIO_getEnabledInterruptStatus(GPIOB,
        FREQUENCY_METER_INPUT_PIN | PHASE_METER_B_PIN);
    uint64_t timestampTicks = FrequencyMeter_ReadTimestampTicks();

    FrequencyMeter_GpioIsr(status, timestampTicks);
    PhaseMeter_GpioIsr(status, timestampTicks);
    DL_GPIO_clearInterruptStatus(GPIOB,
        FREQUENCY_METER_INPUT_PIN | PHASE_METER_B_PIN);
}
