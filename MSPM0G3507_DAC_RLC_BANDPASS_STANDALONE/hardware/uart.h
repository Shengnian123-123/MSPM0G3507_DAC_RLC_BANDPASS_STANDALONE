#ifndef HW_UART_H
#define HW_UART_H

#include <stdbool.h>
#include <stdint.h>
#include "main.h"

void Uart_Init(void);
bool Uart_ReadByte(uint8_t *data);
void Uart_WriteByte(uint8_t data);
void Uart_WriteString(const char *text);
void Uart_WriteU32(uint32_t value);
void Uart_WriteU64(uint64_t value);
void Uart_WriteHex8(uint8_t value);
void Uart_Printf(const char *fmt, ...);

#endif
