#include "peripheral_status.h"

const char *Periph_StatusText(Periph_Status_t status)
{
    switch (status) {
        case PERIPH_STATUS_OK:
            return "OK";
        case PERIPH_STATUS_BAD_PARAM:
            return "BAD_PARAM";
        case PERIPH_STATUS_TIMEOUT:
            return "TIMEOUT";
        case PERIPH_STATUS_NOT_CONFIGURED:
            return "NOT_CONFIGURED";
        case PERIPH_STATUS_ERROR:
        default:
            return "ERROR";
    }
}
