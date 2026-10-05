#include "motor.h"
#include "Arduino.h"

#define MOTOR_MAX_PWM 255
#define MOTOR_VOLTAGE_2_PWM ((float)(MOTOR_MAX_PWM) / MOTOR_SUPPLY_VOLTAGE_V)

float motor_last_voltage = 0;

bool motor_init(void)
{
    pinMode(MOTOR_I1_PIN, OUTPUT);
    pinMode(MOTOR_I2_PIN, OUTPUT);
    pinMode(MOTOR_PWM_PIN, OUTPUT);
    return true;
}

void motor_set_voltage(float voltage_v)
{
    if(voltage_v == NAN)
        return;
    if(voltage_v == INFINITY)
        return;
    if(voltage_v == -INFINITY)
        return;
        
    voltage_v = constrain(voltage_v, -MAX_MOTOR_VOLTAGE_V, MAX_MOTOR_VOLTAGE_V);
    motor_last_voltage = voltage_v;
    if(voltage_v ==0)
    {
        motor_stop(MOTOR_COAST);
        return;
    }

    if (voltage_v > 0)
    {
        digitalWrite(MOTOR_I2_PIN, LOW);
        digitalWrite(MOTOR_I1_PIN, HIGH);
    }
    else
    {
        digitalWrite(MOTOR_I1_PIN, LOW);
        digitalWrite(MOTOR_I2_PIN, HIGH);
    }
    analogWrite(MOTOR_PWM_PIN, abs(voltage_v) * MOTOR_VOLTAGE_2_PWM);
}

void motor_stop(MotorStopMode mode)
{
    switch (mode)
    {
    case MOTOR_BRAKE:
        digitalWrite(MOTOR_I2_PIN, HIGH);
        digitalWrite(MOTOR_I1_PIN, HIGH);
        analogWrite(MOTOR_PWM_PIN,0);
        /* code */
        break;
    case MOTOR_COAST:
        digitalWrite(MOTOR_I2_PIN, LOW);
        digitalWrite(MOTOR_I1_PIN, LOW);
        analogWrite(MOTOR_PWM_PIN,0);
        /* code */
        break;
    
    default:
        break;
    }
}

float motor_get_applied_voltage(void){
    return motor_last_voltage;
}