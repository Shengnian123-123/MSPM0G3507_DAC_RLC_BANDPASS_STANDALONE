#ifndef HW_GPIO_IO_H
#define HW_GPIO_IO_H

#include <stdbool.h>
#include <stdint.h>
#include "main.h"

void GPIOIO_Init(void);

void GPIOIO_LedOn(void);
void GPIOIO_LedOff(void);
void GPIOIO_LedToggle(void);
bool GPIOIO_LedIsOn(void);

bool GPIOIO_KeyIsPressed(void);

void GPIOIO_BuzzerOn(void);
void GPIOIO_BuzzerOff(void);
void GPIOIO_BuzzerBeep(uint32_t onMs);
bool GPIOIO_BuzzerIsOn(void);

#endif
