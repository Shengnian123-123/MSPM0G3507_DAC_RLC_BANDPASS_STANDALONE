#ifndef CONTROL_PID_H
#define CONTROL_PID_H

#include <stdint.h>

typedef struct {
    int32_t kp;
    int32_t ki;
    int32_t kd;
    int32_t integral;
    int32_t previousError;
    int32_t outputMin;
    int32_t outputMax;
    uint8_t shift;
} ControlPid_t;

void ControlPid_Init(ControlPid_t *pid, int32_t kp, int32_t ki, int32_t kd,
    int32_t outputMin, int32_t outputMax, uint8_t shift);
void ControlPid_Reset(ControlPid_t *pid);
int32_t ControlPid_Update(ControlPid_t *pid, int32_t setpoint,
    int32_t feedback);

#endif
