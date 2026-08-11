#include "oled_ssd1306.h"
#include "i2c_bus.h"

static uint8_t g_oledAddress = OLED_SSD1306_DEFAULT_ADDR;

Periph_Status_t OledSsd1306_Init(uint8_t address7)
{
    Periph_Status_t status;

    g_oledAddress = address7;
    status = I2CBus_Init();
    if (status != PERIPH_STATUS_OK) {
        return status;
    }

    return PERIPH_STATUS_NOT_CONFIGURED;
}

Periph_Status_t OledSsd1306_Clear(void)
{
    (void) g_oledAddress;
    return PERIPH_STATUS_NOT_CONFIGURED;
}

Periph_Status_t OledSsd1306_WriteText(uint8_t x, uint8_t page,
    const char *text)
{
    (void) x;
    (void) page;
    (void) text;
    (void) g_oledAddress;
    return PERIPH_STATUS_NOT_CONFIGURED;
}
