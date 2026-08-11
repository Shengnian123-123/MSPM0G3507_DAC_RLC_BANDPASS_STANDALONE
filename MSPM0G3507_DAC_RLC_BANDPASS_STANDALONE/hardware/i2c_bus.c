#include "i2c_bus.h"

Periph_Status_t I2CBus_Init(void)
{
    return PERIPH_STATUS_NOT_CONFIGURED;
}

Periph_Status_t I2CBus_Write(uint8_t address7, const uint8_t *data,
    uint16_t length)
{
    (void) address7;
    (void) data;
    (void) length;
    return PERIPH_STATUS_NOT_CONFIGURED;
}

Periph_Status_t I2CBus_ReadReg(uint8_t address7, uint8_t reg, uint8_t *data,
    uint16_t length)
{
    (void) address7;
    (void) reg;
    (void) data;
    (void) length;
    return PERIPH_STATUS_NOT_CONFIGURED;
}

Periph_Status_t I2CBus_Scan(uint8_t *addresses, uint8_t maxCount,
    uint8_t *foundCount)
{
    (void) addresses;
    (void) maxCount;

    if (foundCount != 0) {
        *foundCount = 0U;
    }
    return PERIPH_STATUS_NOT_CONFIGURED;
}
