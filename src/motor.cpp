#include "motor.h"
#include "Arduino.h"

#define MOTOR_MAX_PWM 255
#define MOTOR_VOLTAGE_2_PWM ((float)(MOTOR_MAX_PWM) / MOTOR_SUPPLY_VOLTAGE_V)


/* motor.cpp — représentation privée */
struct Motor {
    uint8_t in1, in2, pwm;
    float applied_v;
    bool initialized;
};

static Motor motors[2] = {
    { MOTOR_LEFT_I1_PIN,
      MOTOR_LEFT_I2_PIN,
      MOTOR_LEFT_PWM_PIN,
      0.0f, false },

    { MOTOR_RIGHT_I1_PIN,
      MOTOR_RIGHT_I2_PIN,
      MOTOR_RIGHT_PWM_PIN,
      0.0f, false }
};

Motor *motor_get(MotorId id)
{
    switch (id) {
    case MOTOR_LEFT:  return &motors[0];
    case MOTOR_RIGHT: return &motors[1];
    default:         return NULL;
    }
}


bool motor_init(Motor *motor){
     if (motor == NULL) {
        return false;
    }

    if (motor->initialized) {
        return true;  /* Déjà initialisé : aucune modification. */
    }

    // on pourrait aussi vérifier si les broches sont compatible avec la pwm ...


    pinMode(motor->in1, OUTPUT);
    pinMode(motor->in2, OUTPUT);
    pinMode(motor->pwm, OUTPUT);
    motor->initialized = true;
    motor->applied_v = 0.0f;

    return true;
}

bool motor_set_voltage(Motor *motor, float voltage_v){
    if(motor==NULL) 
        return false;
    
    if(!motor->initialized)
        return false;
    
    if (!isfinite(voltage_v)) {
        motor_stop(motor, MOTOR_COAST);
        return false;
    }

    voltage_v = constrain(voltage_v, -MAX_MOTOR_VOLTAGE_V, MAX_MOTOR_VOLTAGE_V);
    motor->applied_v = voltage_v;
    if(voltage_v ==0)
    {
        motor_stop(motor,MOTOR_COAST);
        return true;
    }

    if (voltage_v > 0)
    {
        digitalWrite(motor->in2, LOW);
        digitalWrite(motor->in1, HIGH);
    }
    else
    {
        digitalWrite(motor->in1, LOW);
        digitalWrite(motor->in2, HIGH);
    }
    analogWrite(motor->pwm, abs(voltage_v) * MOTOR_VOLTAGE_2_PWM);
    return true;
}

void motor_stop(Motor *motor, MotorStopMode mode){
    if(motor==NULL)
        return;

    switch (mode)
    {
    case MOTOR_BRAKE:
        digitalWrite(motor->in2, HIGH);
        digitalWrite(motor->in1, HIGH);
        analogWrite(motor->pwm,0);
        /* code */
        break;
    case MOTOR_COAST:
        digitalWrite(motor->in2, LOW);
        digitalWrite(motor->in1, LOW);
        analogWrite(motor->pwm,0);
        /* code */
        break;
    
    default:
        break;
    }
}

bool motor_get_applied_voltage(Motor * motor, float * voltage_v){
    if(motor==NULL)
        return false;
    if(!motor->initialized)
        return false;

    *voltage_v = motor->applied_v;
    return true;

}

