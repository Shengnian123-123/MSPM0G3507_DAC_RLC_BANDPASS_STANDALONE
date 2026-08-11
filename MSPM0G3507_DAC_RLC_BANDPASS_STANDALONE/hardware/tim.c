#include "tim.h"

static volatile uint32_t g_tickMs;
static volatile uint32_t g_timer1Ticks;

static const Timer_ChannelConfig_t g_timer0 = {
    TIMER_0_INST,
    TIMER_0_INST_INT_IRQN,
    &g_tickMs,
};

static const Timer_ChannelConfig_t g_timer1 = {
    TIMER_1_INST,
    TIMER_1_INST_INT_IRQN,
    &g_timer1Ticks,
};

void Timer_Start(void)
{
    Timer_StartOne(&g_timer0);
    Timer_StartOne(&g_timer1);
}

void Timer_StartOne(const Timer_ChannelConfig_t *config)
{
    if (config == 0) {
        return;
    }

    DL_TimerG_clearInterruptStatus(config->inst, DL_TIMERG_INTERRUPT_ZERO_EVENT);
    NVIC_ClearPendingIRQ(config->irq);
    NVIC_EnableIRQ(config->irq);
    DL_TimerG_startCounter(config->inst);
}

bool Timer_CountZeroEvent(const Timer_ChannelConfig_t *config)
{
    if ((config == 0) || (config->tick == 0)) {
        return false;
    }

    if (DL_TimerG_getPendingInterrupt(config->inst) == DL_TIMER_IIDX_ZERO) {
        (*config->tick)++;
        return true;
    }

    return false;
}

uint32_t Timer_GetTickMs(void)
{
    return g_tickMs;
}

uint32_t Timer_GetTimer1Ticks(void)
{
    return g_timer1Ticks;
}

void TIMER_0_INST_IRQHandler(void)
{
    (void) Timer_CountZeroEvent(&g_timer0);
}

void TIMER_1_INST_IRQHandler(void)
{
    (void) Timer_CountZeroEvent(&g_timer1);
}
