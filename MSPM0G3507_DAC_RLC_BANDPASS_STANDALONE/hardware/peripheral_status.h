#ifndef PERIPHERAL_STATUS_H
#define PERIPHERAL_STATUS_H

typedef enum {
    PERIPH_STATUS_OK = 0,
    PERIPH_STATUS_BAD_PARAM,
    PERIPH_STATUS_TIMEOUT,
    PERIPH_STATUS_NOT_CONFIGURED,
    PERIPH_STATUS_ERROR,
} Periph_Status_t;

const char *Periph_StatusText(Periph_Status_t status);

#endif
