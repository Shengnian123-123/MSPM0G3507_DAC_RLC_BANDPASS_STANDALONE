#ifndef HW_SCREEN_UART_H
#define HW_SCREEN_UART_H

#include <stdbool.h>
#include <stdint.h>

void ScreenUart_Init(void);
bool ScreenUart_ReadByte(uint8_t *data);
void ScreenUart_WriteByte(uint8_t data);
void ScreenUart_WriteString(const char *text);
void ScreenUart_WriteCommandEnd(void);

#endif
