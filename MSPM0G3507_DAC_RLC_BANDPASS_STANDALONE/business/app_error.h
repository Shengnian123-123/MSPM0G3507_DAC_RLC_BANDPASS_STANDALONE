#ifndef APP_ERROR_H
#define APP_ERROR_H

#include <stdint.h>

void App_ErrorHandler(const char *reason);
void App_AssertFailed(const char *expr, const char *file, uint32_t line);

#define APP_ASSERT(expr) \
    do { \
        if (!(expr)) { \
            App_AssertFailed(#expr, __FILE__, (uint32_t) __LINE__); \
        } \
    } while (0)

#endif
