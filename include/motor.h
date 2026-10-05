#ifndef MOTOR_H
#define MOTOR_H

#include "config.h"

typedef enum {
  MOTOR_COAST,
  MOTOR_BRAKE
} MotorStopMode;
bool motor_init(void);
void motor_set_voltage(float voltage_v);
void motor_stop(MotorStopMode mode);
float motor_get_applied_voltage(void);

#endif