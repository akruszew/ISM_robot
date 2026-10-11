#ifndef MOTOR_H
#define MOTOR_H

#include "config.h"

typedef struct Motor Motor;
typedef enum { MOTOR_LEFT, MOTOR_RIGHT } MotorId;
typedef enum { MOTOR_COAST, MOTOR_BRAKE } MotorStopMode;

Motor *motor_get(MotorId id);
bool motor_init(Motor *motor);
bool motor_set_voltage(Motor *motor, float voltage_v);
void motor_stop(Motor *motor, MotorStopMode mode);
bool motor_get_applied_voltage(Motor * motor, float * voltage_v);

#endif