#include "control_pid.h"

static int32_t ClampI32(int32_t value, int32_t minValue, int32_t maxValue)
{
    if (value < minValue) {
        return minValue;
    }
    if (value > maxValue) {
        return maxValue;
    }
    return value;
}

void ControlPid_Init(ControlPid_t *pid, int32_t kp, int32_t ki, int32_t kd,
    int32_t outputMin, int32_t outputMax, uint8_t shift)
{
    if (pid == 0) {
        return;
    }

    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->integral = 0;
    pid->previousError = 0;
    pid->outputMin = outputMin;
    pid->outputMax = outputMax;
    pid->shift = shift;
}

void ControlPid_Reset(ControlPid_t *pid)
{
    if (pid == 0) {
        return;
    }

    pid->integral = 0;
    pid->previousError = 0;
}

int32_t ControlPid_Update(ControlPid_t *pid, int32_t setpoint,
    int32_t feedback)
{
    int32_t error;
    int32_t derivative;
    int64_t output;

    if (pid == 0) {
        return 0;
    }

    error = setpoint - feedback;
    derivative = error - pid->previousError;
    pid->integral = ClampI32(pid->integral + error, -1000000L, 1000000L);

    output = ((int64_t) pid->kp * error) +
        ((int64_t) pid->ki * pid->integral) +
        ((int64_t) pid->kd * derivative);
    if (pid->shift > 0U) {
        output >>= pid->shift;
    }

    pid->previousError = error;
    return ClampI32((int32_t) output, pid->outputMin, pid->outputMax);
}
