#include "screen_uart.h"
#include "main.h"

#define SCREEN_UART_RX_BUFFER_SIZE (128U)
#define SCREEN_UART_RX_BUFFER_MASK (SCREEN_UART_RX_BUFFER_SIZE - 1U)

static volatile uint8_t g_screenRxBuffer[SCREEN_UART_RX_BUFFER_SIZE];
static volatile uint16_t g_screenRxHead;
static volatile uint16_t g_screenRxTail;
static volatile bool g_screenRxOverflow;

static void ScreenUart_StoreRx(uint8_t data)
{
    uint16_t nextHead = (uint16_t) ((g_screenRxHead + 1U) &
        SCREEN_UART_RX_BUFFER_MASK);

    if (nextHead != g_screenRxTail) {
        g_screenRxBuffer[g_screenRxHead] = data;
        g_screenRxHead = nextHead;
    } else {
        g_screenRxOverflow = true;
    }
}

void ScreenUart_Init(void)
{
    g_screenRxHead = 0U;
    g_screenRxTail = 0U;
    g_screenRxOverflow = false;

    NVIC_ClearPendingIRQ(UART_SCREEN_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_SCREEN_INST_INT_IRQN);
}

bool ScreenUart_ReadByte(uint8_t *data)
{
    if ((data == 0) || (g_screenRxHead == g_screenRxTail)) {
        return false;
    }

    *data = g_screenRxBuffer[g_screenRxTail];
    g_screenRxTail = (uint16_t) ((g_screenRxTail + 1U) &
        SCREEN_UART_RX_BUFFER_MASK);
    return true;
}

void ScreenUart_WriteByte(uint8_t data)
{
    DL_UART_Main_transmitDataBlocking(UART_SCREEN_INST, data);
}

void ScreenUart_WriteString(const char *text)
{
    if (text == 0) {
        return;
    }

    while (*text != '\0') {
        ScreenUart_WriteByte((uint8_t) *text);
        text++;
    }
}

void ScreenUart_WriteCommandEnd(void)
{
    ScreenUart_WriteByte(0xFFU);
    ScreenUart_WriteByte(0xFFU);
    ScreenUart_WriteByte(0xFFU);
}

void UART_SCREEN_INST_IRQHandler(void)
{
    switch (DL_UART_Main_getPendingInterrupt(UART_SCREEN_INST)) {
        case DL_UART_MAIN_IIDX_RX:
            while (!DL_UART_Main_isRXFIFOEmpty(UART_SCREEN_INST)) {
                ScreenUart_StoreRx(DL_UART_Main_receiveData(UART_SCREEN_INST));
            }
            break;
        default:
            break;
    }

    if (g_screenRxOverflow) {
        g_screenRxTail = g_screenRxHead;
        g_screenRxOverflow = false;
    }
}
