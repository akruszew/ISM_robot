#ifndef CONFIG_H
#define CONFIG_H

// Motors pinout config
#define MAX_MOTOR_VOLTAGE_V 9.0f
#define MOTOR_SUPPLY_VOLTAGE_V 12.0f

#define MOTOR_LEFT_I1_PIN 34
#define MOTOR_LEFT_I2_PIN 35
#define MOTOR_LEFT_PWM_PIN 12

#define MOTOR_RIGHT_I1_PIN 37
#define MOTOR_RIGHT_I2_PIN 36
#define MOTOR_RIGHT_PWM_PIN 8

// Encoders pinout
#define ENCODER_LEFT_VA 18
#define ENCODER_LEFT_VB 31

#define ENCODER_RIGHT_VA 19
#define ENCODER_RIGHT_VB 38


// line sensor 
#define LINE_SENSOR_1 A10
#define LINE_SENSOR_2 A11

#endif