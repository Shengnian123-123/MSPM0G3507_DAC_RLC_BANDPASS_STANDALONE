#include "uart.h"
#include <stdarg.h>
#include <stdio.h>

#define UART_RX_BUFFER_SIZE (128U)
#define UART_RX_BUFFER_MASK (UART_RX_BUFFER_SIZE - 1U)
#define UART_PRINTF_BUFFER_SIZE (160U)

static volatile uint8_t g_rxBuffer[UART_RX_BUFFER_SIZE];
static volatile uint16_t g_rxHead;
static volatile uint16_t g_rxTail;

static void Uart_StoreRx(uint8_t data)
{
    uint16_t nextHead = (uint16_t) ((g_rxHead + 1U) & UART_RX_BUFFER_MASK);

    if (nextHead != g_rxTail) {
        g_rxBuffer[g_rxHead] = data;
        g_rxHead             = nextHead;
    }
}

void Uart_Init(void)
{
    g_rxHead = 0U;
    g_rxTail = 0U;

    NVIC_ClearPendingIRQ(UART_0_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_0_INST_INT_IRQN);
}

bool Uart_ReadByte(uint8_t *data)
{
    if ((data == 0) || (g_rxHead == g_rxTail)) {
        return false;
    }

    *data    = g_rxBuffer[g_rxTail];
    g_rxTail = (uint16_t) ((g_rxTail + 1U) & UART_RX_BUFFER_MASK);
    return true;
}

void Uart_WriteByte(uint8_t data)
{
    DL_UART_Main_transmitDataBlocking(UART_0_INST, data);
}

void Uart_WriteString(const char *text)
{
    if (text == 0) {
        return;
    }

    while (*text != '\0') {
        Uart_WriteByte((uint8_t) *text);
        text++;
    }
}

void Uart_WriteU32(uint32_t value)
{
    char buffer[10];
    uint8_t index = 0U;

    if (value == 0U) {
        Uart_WriteByte('0');
        return;
    }

    while ((value > 0U) && (index < sizeof(buffer))) {
        buffer[index++] = (char) ('0' + (value % 10U));
        value /= 10U;
    }

    while (index > 0U) {
        Uart_WriteByte((uint8_t) buffer[--index]);
    }
}

void Uart_WriteU64(uint64_t value)
{
    char buffer[20];
    uint8_t index = 0U;

    if (value == 0U) {
        Uart_WriteByte('0');
        return;
    }

    while ((value > 0U) && (index < sizeof(buffer))) {
        buffer[index++] = (char) ('0' + (value % 10U));
        value /= 10U;
    }

    while (index > 0U) {
        Uart_WriteByte((uint8_t) buffer[--index]);
    }
}

void Uart_WriteHex8(uint8_t value)
{
    static const char hex[] = "0123456789ABCDEF";

    Uart_WriteByte((uint8_t) hex[(value >> 4U) & 0x0FU]);
    Uart_WriteByte((uint8_t) hex[value & 0x0FU]);
}

void Uart_Printf(const char *fmt, ...)
{
    char buffer[UART_PRINTF_BUFFER_SIZE];
    va_list args;
    int length;

    if (fmt == 0) {
        return;
    }

    va_start(args, fmt);
    length = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (length <= 0) {
        return;
    }

    buffer[sizeof(buffer) - 1U] = '\0';
    Uart_WriteString(buffer);
}

void UART_0_INST_IRQHandler(void)
{
    switch (DL_UART_Main_getPendingInterrupt(UART_0_INST)) {
        case DL_UART_MAIN_IIDX_RX:
            while (!DL_UART_Main_isRXFIFOEmpty(UART_0_INST)) {
                Uart_StoreRx(DL_UART_Main_receiveData(UART_0_INST));
            }
            break;
        default:
            break;
    }
}
