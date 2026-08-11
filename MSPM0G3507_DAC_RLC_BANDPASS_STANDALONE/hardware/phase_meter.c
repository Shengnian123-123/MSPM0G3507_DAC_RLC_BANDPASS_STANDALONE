#include "phase_meter.h"
#include "frequency_meter.h"

static volatile uint64_t g_lastRiseA;
static volatile uint64_t g_previousRiseA;
static volatile uint64_t g_lastRiseB;
static volatile uint32_t g_periodTicksA;
static volatile uint32_t g_edgeCountA;
static volatile uint32_t g_edgeCountB;
static volatile bool g_hasPeriodA;
static volatile bool g_hasRiseB;

static bool PinHigh(GPIO_Regs *port, uint32_t pin)
{
    return ((DL_GPIO_readPins(port, pin) & pin) != 0U);
}

void PhaseMeter_Init(void)
{
    DL_GPIO_initDigitalInputFeatures(PHASE_METER_A_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_DOWN,
        DL_GPIO_HYSTERESIS_ENABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(PHASE_METER_B_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_DOWN,
        DL_GPIO_HYSTERESIS_ENABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_setUpperPinsPolarity(GPIOB,
        DL_GPIO_PIN_20_EDGE_RISE_FALL | DL_GPIO_PIN_21_EDGE_RISE);
    DL_GPIO_clearInterruptStatus(GPIOB, PHASE_METER_A_PIN | PHASE_METER_B_PIN);
    DL_GPIO_enableInterrupt(GPIOB, PHASE_METER_A_PIN | PHASE_METER_B_PIN);
    NVIC_ClearPendingIRQ(GPIOB_INT_IRQn);
    NVIC_EnableIRQ(GPIOB_INT_IRQn);
}

void PhaseMeter_GpioIsr(uint32_t status, uint64_t timestampTicks)
{
    if (((status & PHASE_METER_A_PIN) != 0U) &&
        PinHigh(PHASE_METER_A_PORT, PHASE_METER_A_PIN)) {
        g_edgeCountA++;
        g_previousRiseA = g_lastRiseA;
        g_lastRiseA = timestampTicks;
        if ((g_previousRiseA != 0ULL) && (g_lastRiseA > g_previousRiseA)) {
            g_periodTicksA = (uint32_t) (g_lastRiseA - g_previousRiseA);
            g_hasPeriodA = true;
        }
    }

    if (((status & PHASE_METER_B_PIN) != 0U) &&
        PinHigh(PHASE_METER_B_PORT, PHASE_METER_B_PIN)) {
        g_edgeCountB++;
        g_lastRiseB = timestampTicks;
        g_hasRiseB = true;
    }
}

bool PhaseMeter_Read(PhaseMeter_Result_t *result)
{
    uint64_t riseA;
    uint64_t riseB;
    uint32_t periodTicks;
    uint32_t deltaTicks;
    uint32_t phaseDeg;

    if (result == 0) {
        return false;
    }

    __disable_irq();
    riseA = g_lastRiseA;
    riseB = g_lastRiseB;
    periodTicks = g_periodTicksA;
    result->edgeCountA = g_edgeCountA;
    result->edgeCountB = g_edgeCountB;
    result->valid = g_hasPeriodA && g_hasRiseB && (periodTicks != 0U);
    __enable_irq();

    result->phaseDeg = 0U;
    result->signedDeg = 0;
    result->deltaUs = 0U;
    result->periodUs = 0U;

    if (!result->valid) {
        return false;
    }

    if (riseB >= riseA) {
        deltaTicks = (uint32_t) ((riseB - riseA) % periodTicks);
    } else {
        uint32_t lagTicks = (uint32_t) ((riseA - riseB) % periodTicks);

        deltaTicks = (lagTicks == 0U) ? 0U : (periodTicks - lagTicks);
    }

    phaseDeg = (uint32_t) ((deltaTicks * 360ULL) / periodTicks);
    result->phaseDeg = (uint16_t) phaseDeg;
    result->signedDeg = (phaseDeg > 180U) ?
        (int16_t) ((int32_t) phaseDeg - 360L) : (int16_t) phaseDeg;
    result->deltaUs = (uint32_t) ((deltaTicks * 1000000ULL) /
                                  FrequencyMeter_GetTicksPerSecond());
    result->periodUs = (uint32_t) ((periodTicks * 1000000ULL) /
                                   FrequencyMeter_GetTicksPerSecond());
    return true;
}
