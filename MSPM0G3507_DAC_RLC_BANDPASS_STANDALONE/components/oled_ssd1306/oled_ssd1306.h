#ifndef OLED_SSD1306_H
#define OLED_SSD1306_H

#include <stdint.h>
#include "peripheral_status.h"

#define OLED_SSD1306_DEFAULT_ADDR (0x3CU)

Periph_Status_t OledSsd1306_Init(uint8_t address7);
Periph_Status_t OledSsd1306_Clear(void);
Periph_Status_t OledSsd1306_WriteText(uint8_t x, uint8_t page,
    const char *text);

#endif
