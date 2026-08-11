#ifndef HW_SOFT_SPI_H
#define HW_SOFT_SPI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "main.h"

typedef struct {
    GPIO_Regs *port;
    uint32_t sclkPin;
    uint32_t mosiPin;
    uint32_t misoPin;
    uint32_t csPin;
    uint16_t delayCycles;
} SoftSPI_GPIOConfig_t;

void SoftSPI_Init(void);
void SoftSPI_SetDelay(uint16_t delayCycles);
void SoftSPI_Select(void);
void SoftSPI_Release(void);
uint8_t SoftSPI_TransferByte(uint8_t txData);
void SoftSPI_GPIOTransfer(const SoftSPI_GPIOConfig_t *config, const uint8_t *tx,
    uint8_t *rx, size_t length, bool controlCs);
void SoftSPI_Transfer(const uint8_t *tx, uint8_t *rx, size_t length);

#endif
