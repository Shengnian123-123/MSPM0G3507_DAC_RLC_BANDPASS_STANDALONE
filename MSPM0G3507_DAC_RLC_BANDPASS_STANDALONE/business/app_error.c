#include "app_error.h"
#include "uart.h"

void App_ErrorHandler(const char *reason)
{
    __disable_irq();

    Uart_WriteString("\r\nERR fatal");
    if (reason != 0) {
        Uart_WriteString(" reason=");
        Uart_WriteString(reason);
    }
    Uart_WriteString("\r\n");

    while (1) {
    }
}

void App_AssertFailed(const char *expr, const char *file, uint32_t line)
{
    __disable_irq();

    Uart_WriteString("\r\nERR assert expr=");
    Uart_WriteString(expr);
    Uart_WriteString(" file=");
    Uart_WriteString(file);
    Uart_WriteString(" line=");
    Uart_WriteU32(line);
    Uart_WriteString("\r\n");

    while (1) {
    }
}
