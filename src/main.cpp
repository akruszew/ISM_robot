#include <Arduino.h>
#include "motor.h"
#include "encoder.h"
#include "config.h"
#define DT 5


static Motor *motor_left;
static Motor *motor_right;

float getSpeed();

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Hello");
  motor_left = motor_get(MOTOR_LEFT);
  motor_right = motor_get(MOTOR_RIGHT);
  
  if(motor_init(motor_left)==false){
    Serial.println("failled init left motor");
    while (1) {    }
  }
  
  if(motor_init(motor_right)==false){
    Serial.println("failled init right motor");
    while (1) {    }
  }
  
  if(encoder_init()==false){
    Serial.println("failled init encoder");
    while (1) {    }
  }
 
}

static void telemetry_run(uint32_t now_ms) // static = fonction privée du module
{

    float left_v, right_v;

    bool left_valid =
        motor_get_applied_voltage(motor_left, &left_v);
    bool right_valid =
        motor_get_applied_voltage(motor_right, &right_v);

    Serial.print(now_ms);

    Serial.print(',');
    if (left_valid) Serial.print(left_v, 3);
    else            Serial.print("NA");

    Serial.print(',');
    if (right_valid) Serial.print(right_v, 3);
    else             Serial.print("NA");

    Serial.println();

}

static bool period_is_due(
    uint32_t now_us,
    uint32_t *last_release_us,
    uint32_t period_us)
{
  if ((uint32_t)(now_us - *last_release_us)
      < period_us) return false;
  *last_release_us += period_us;
  return true;
}

float update_voltage_sequence(uint32_t time_us){
    float u = 0;
  if(time_us < 1000000) 
  {
      u=0;
  }else
  if (time_us <2000000)
  {
    u= 5;
  }else
  if(time_us>4000000){
  u =0  ;
  }
  return u;
  
}

void loop() {
  // put your main code here, to run repeatedly:
  static uint32_t last_release_speed_us = 0;
  static uint32_t last_release_telemetry_us = 0;
  static float speed_tick_per_s = 0;

    
  uint32_t current_time_us = micros();

  float motor_voltage_v = update_voltage_sequence(current_time_us);
  motor_set_voltage(motor_left,motor_voltage_v);
  motor_set_voltage(motor_right,-motor_voltage_v);

  if( period_is_due(current_time_us,&last_release_telemetry_us,TELEMETRY_PERIOD_US) ){
    telemetry_run(current_time_us);
  }

  if( period_is_due(current_time_us,&last_release_speed_us,SPEED_PERIOD_US) ){
    speed_tick_per_s = getSpeed();
  }
  
}

float getSpeed(){
  static uint32_t last_sample_time_us = 0;
  float delta_ticks = encoder_get_and_reset_delta();
  uint32_t current_time_us = micros();
  float delta_time = current_time_us-last_sample_time_us;
  last_sample_time_us = current_time_us;
  return delta_ticks*1e6/delta_time;
}
