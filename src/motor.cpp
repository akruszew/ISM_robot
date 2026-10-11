#include "motor.h"
#include "Arduino.h"
#include "config.h"
#include <math.h>


/*
 * Hypothèses de configuration :
 * - alimentation et limite de commande finies et positives ;
 * - MAX_MOTOR_VOLTAGE_V <= MOTOR_SUPPLY_VOLTAGE_V.
 * La tension d'alimentation sert à convertir en PWM ;
 * elle est distincte de la limite de commande.
 */
#define MOTOR_MAX_PWM 255
#define MOTOR_VOLTAGE_2_PWM ((float)(MOTOR_MAX_PWM) / MOTOR_SUPPLY_VOLTAGE_V)


/* motor.cpp — représentation privée */
struct Motor {
    uint8_t in1, in2, pwm;
    
    /* Consigne saturée mémorisée, pas une tension mesurée.
       Une valeur nulle ne prouve pas l'immobilité du moteur. */
    float applied_v;
    bool initialized;
};


/*
 * Instances détenues par le pilote, sans allocation dynamique.
 * Les broches viennent de config.h ; la configuration du matériel
 * est réalisée par motor_init().
 */
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


/*
 * Helpers internes : supposent une instance valide et des sorties
 * configurées. Utilisables pendant l'initialisation, avant que
 * initialized soit positionné.
 *
 * Séquences COAST/BRAKE vérifiées sur le montage utilisé
 * et confrontées à la documentation du MP80495.
 */
static void motor_apply_coast(Motor *motor)
{
    analogWrite(motor->pwm, 0);
    digitalWrite(motor->in1, LOW);
    digitalWrite(motor->in2, LOW);
    
    motor->applied_v = 0.0f;
}

static void motor_apply_brake(Motor *motor)
{
    analogWrite(motor->pwm, 0);
    digitalWrite(motor->in1, HIGH);
    digitalWrite(motor->in2, HIGH);
   
    motor->applied_v = 0.0f;
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
    motor_apply_coast(motor);
    
    motor->initialized = true; // doit être fait en dernier
    return true;
}


bool motor_set_voltage(Motor *motor, float voltage_v)
{
    if (motor == NULL || !motor->initialized) {
        return false;
    }

    /* Repli local en roue libre ; l'application traite le défaut global. */
    if (!isfinite(voltage_v)) {
        motor_apply_coast(motor);
        return false;
    }

    voltage_v = constrain(
        voltage_v,
        -MAX_MOTOR_VOLTAGE_V,
        MAX_MOTOR_VOLTAGE_V);

    if (voltage_v == 0.0f) {
        motor_apply_coast(motor);
        return true;
    }

    const uint8_t pwm = (uint8_t)lroundf(
        fabsf(voltage_v) * MOTOR_VOLTAGE_2_PWM);

    /*
    * Couper l'ancien PWM avant de modifier la direction.
    * Cette séquence ne garantit pas à elle seule une inversion
    * mécanique sans surintensité.
    */
    analogWrite(motor->pwm, 0);

    if (voltage_v > 0.0f) {
        digitalWrite(motor->in2, LOW);
        digitalWrite(motor->in1, HIGH);
    } else {
        digitalWrite(motor->in1, LOW);
        digitalWrite(motor->in2, HIGH);
    }

    analogWrite(motor->pwm, pwm);

     /* Consigne mémorisée, pas tension mesurée ni preuve d'immobilité. */
    motor->applied_v = voltage_v;

    return true;
}

void motor_stop(Motor *motor, MotorStopMode mode){
    if(motor==NULL)
        return;
    if(motor->initialized==false)
        return;

    switch (mode)
    {
    case MOTOR_BRAKE:
        motor_apply_brake(motor);
        break;
    case MOTOR_COAST:
        motor_apply_coast(motor);
        break;
    
    default:
        /* Politique choisie : mode inconnu -> roue libre. */
        motor_apply_coast(motor);
        break;
    }
}

bool motor_get_applied_voltage(const Motor * motor, float * voltage_v){
    if(motor==NULL)
        return false;
    if(voltage_v==NULL)
        return false;
    if(!motor->initialized)
        return false;

    *voltage_v = motor->applied_v;
    return true;

}

