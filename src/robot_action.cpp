#include "robot_actions.h"
#include "encoder.h"
#include "motor.h"
#include "speed_controller.h"
#include "line_sensor.h"
#include <stddef.h>
#include <Arduino.h>
#include "robot_app.h"

static Motor *motor_left;
static Motor *motor_right;

struct WheelSpeed
{
  Encoder *encoder;
  uint32_t previous_sample_us;
  float speed_ticks_s;
  bool valid;
};

static WheelSpeed left_speed = {NULL, 0, 0.0f, false};
static WheelSpeed right_speed = {NULL, 0, 0.0f, false};

static SpeedController left_controller = {
  0.01f,
  0.01f,
  0.0f,
  -9.0f,
  9.0f,
};
static SpeedController right_controller{
  0.01f,
  0.01f,
  0.0f,
  -9.0f,
  9.0f,
};

static LineReading line_reading = {0,0};

static uint32_t last_controller_us = 0;
static bool controller_time_valid = false;


bool robot_hw_init(void){
  motor_left = motor_get(MOTOR_LEFT);
  motor_right = motor_get(MOTOR_RIGHT);
  const bool motor_left_ok = motor_init(motor_left);
  const bool motor_right_ok = motor_init(motor_right);

  if (!motor_left_ok || !motor_right_ok)
  {
    return false;
  }

  left_speed.encoder = encoder_get(ENCODER_LEFT);
  right_speed.encoder = encoder_get(ENCODER_RIGHT);
  const bool left_ok = encoder_init(left_speed.encoder);
  const bool right_ok = encoder_init(right_speed.encoder);

  if (!left_ok || !right_ok)
  {
    return false;
  }

  /* Commencer les fenêtres de mesure après l'initialisation. */
  const bool left_reset_ok = encoder_reset(left_speed.encoder);
  const bool right_reset_ok = encoder_reset(right_speed.encoder);

  if (!left_reset_ok || !right_reset_ok)
  {
    return false;
  }

  const uint32_t start_us = micros();

  left_speed.previous_sample_us = start_us;
  right_speed.previous_sample_us = start_us;

  speed_controller_init(&left_controller);
  speed_controller_init(&right_controller);

  line_sensor_init();

  // Telemetry
  Serial.println("temps_ms,consigne_gauche_V,consigne_droite_V,"
                 "vitesse_gauche_ticks_s,vitesse_droite_ticks_s");
  return true;

}

void robot_stop_all(void){
    motor_stop(motor_left, MOTOR_COAST);
    motor_stop(motor_right, MOTOR_COAST);
    controller_time_valid = false;    
}

void robot_reset_controllers(void){
    speed_controller_init(&left_controller);
    speed_controller_init(&right_controller);
    controller_time_valid = false;
}

void robot_report_fault(void){
 // il faudrait mémoriser les erreurs avec des flags et les afficher
    Serial.println("Erreur");
}

bool robot_fault_resolved(void){
    // not implemented
    return false;
}

static void update_speed(WheelSpeed *wheel, const uint32_t now_us)
{
  const uint32_t dt_us = now_us - wheel->previous_sample_us;

  if (dt_us == 0)
  {
    wheel->valid = false;
    return;
  }

  int32_t delta_ticks;

  if (!encoder_get_and_reset_delta(wheel->encoder, &delta_ticks))
  {
    wheel->valid = false;
    return;
  }

  /* Le calcul reste hors de la section atomique du pilote. */
  wheel->speed_ticks_s =
      (float)delta_ticks * 1000000.0f / (float)dt_us;

  wheel->previous_sample_us = now_us;
  wheel->valid = true;
}

bool robot_measure_speed(uint32_t now_us){
    update_speed(&left_speed, now_us);
    update_speed(&right_speed, now_us);
    if(left_speed.valid && right_speed.valid){
        return true;
    }else{
        return false;
    }
}

bool robot_read_line(bool *present){
    if(present==NULL)
    {
        return false;
    }
    line_sensor_read(&line_reading);
    *present = line_reading.left_detected || line_reading.right_detected;
    return true;
}

static bool update_speed_refs(float* left_ref, float* right_ref)
{
  if(left_ref == NULL || right_ref == NULL)
    return false;

  if(line_reading.left_detected && line_reading.right_detected){
    *left_ref = 0.0f;
    *right_ref = 0.0f;
    return true;
  }

  if(line_reading.left_detected ){
    *left_ref = 0.0f;
    *right_ref = +50.0f;
    return true;
  }

  if(line_reading.right_detected ){
    *left_ref = +50.0f;
    *right_ref = 0.0f;
    return true;
  }

  *left_ref = +50.0f;
  *right_ref = -50.0f;
  return true;
}


bool robot_run_control(uint32_t now_us){
    

    /* en cas de reprise, on initialise la prise de temps pour le calcul de delta_t */
    if (!controller_time_valid) {
        last_controller_us = now_us;
        controller_time_valid = true;
        return true;
    }

    float voltage_left, voltage_right;
    const uint32_t dt_us = now_us - last_controller_us;

    if(dt_us==0){
      return false;
    }
    const float dt = (float)dt_us * 1e-6f;
    
    last_controller_us = now_us ;

    const float ref_tick_s = 50.0f;

    float left_ref_tick_s,right_ref_tick_s;

    if(!update_speed_refs(&left_ref_tick_s,&right_ref_tick_s)){
      return false;
    }

    const bool left_controller_ok = 
      speed_controller_update(&left_controller,
                              -ref_tick_s-left_ref_tick_s,
                              left_speed.speed_ticks_s,
                              dt,
                              &voltage_left
                            );
    const bool right_controller_ok = 
      speed_controller_update(&right_controller,
                              +ref_tick_s+right_ref_tick_s,
                              right_speed.speed_ticks_s,
                              dt,
                              &voltage_right
                            );
    if(!left_controller_ok || !right_controller_ok){
      return false;
    }

    /* Évaluer les deux appels, sans court-circuit logique. */
    const bool left_ok =
        motor_set_voltage(motor_left, voltage_left);

    const bool right_ok =
        motor_set_voltage(motor_right, voltage_right);

    return left_ok && right_ok;
}

/* Publier une valeur uniquement si la mesure est valide. */
static void print_speed(const WheelSpeed *wheel)
{
  if (wheel->valid)
  {
    Serial.print(wheel->speed_ticks_s, 1);
  }
  else
  {
    Serial.print("NA");
  }
}

static void print_voltage(const Motor *motor)
{
  float voltage_v;
  if (motor_get_applied_voltage(motor, &voltage_v))
  {
    Serial.print(voltage_v);
  }
  else
  {
    Serial.print("NA");
  }
}

void robot_run_telemetry(uint32_t now_us){
Serial.print(millis());
  Serial.print(',');

  print_voltage(motor_left);
  Serial.print(',');

  print_voltage(motor_right);
  Serial.print(',');

  print_speed(&left_speed);
  Serial.print(',');

  print_speed(&right_speed);
  Serial.print(',');

  Serial.print(robot_app_get_state());
  Serial.println();
}