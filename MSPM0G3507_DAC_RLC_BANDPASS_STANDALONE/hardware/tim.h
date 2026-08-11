#ifndef HW_TIM_H
#define HW_TIM_H

#include <stdbool.h>
#include <stdint.h>
#include "main.h"

typedef struct {
    GPTIMER_Regs *inst;
    IRQn_Type irq;
    volatile uint32_t *tick;
} Timer_ChannelConfig_t;

void Timer_Start(void);
void Timer_StartOne(const Timer_ChannelConfig_t *config);
bool Timer_CountZeroEvent(const Timer_ChannelConfig_t *config);
uint32_t Timer_GetTickMs(void);
uint32_t Timer_GetTimer1Ticks(void);

#endif
