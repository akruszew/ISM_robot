#ifndef CONFIG_H
#define CONFIG_H

// Motor pinout config
#define MAX_MOTOR_VOLTAGE_V 9.0f
#define MOTOR_SUPPLY_VOLTAGE_V 12.0f

#define MOTOR_I1_PIN 34
#define MOTOR_I2_PIN 35
#define MOTOR_PWM_PIN 12

// Encoder pinout
#define ENCODER_VA 18
#define ENCODER_VB 31

// Periods
#define SPEED_PERIOD_US 5000
#define TELEMETRY_PERIOD_US 50000

#endif