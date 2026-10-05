#include <Arduino.h>
#include "motor.h"
#include "encoder.h"
#define DT 5




float getSpeed();
void ISR_VA();

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Hello");

  if(motor_init()==false){
    Serial.println("failled init motor");
    while (1) {    }
  }
  
  if(encoder_init()==false){
    Serial.println("failled init encoder");
    while (1) {    }
  }

  
}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned long temps=0;
  static unsigned long temps_prec=0;
  
  temps = millis();
  while( (temps-temps_prec)<DT)
  {
    temps = millis();
  }
  temps_prec = temps;

  
  float u = 0;
  if(temps < 1000) 
  {
      u=0;
  }else
  if (temps <2000)
  {
    u= 5;
  }else
  if(temps>4000){
  u =0  ;
  }
   
  motor_set_voltage(u);
  Serial.print(temps);
  Serial.print(" ,");
  Serial.print(u);
  Serial.print(" ,");
  Serial.print(getSpeed());
  Serial.print(" ;");
  
  Serial.println(" ");
}

float getSpeed(){
  static uint32_t last_sample_time_us = 0;
  float delta_ticks = encoder_get_and_reset_delta();
  uint32_t current_time_us = micros();
  float delta_time = current_time_us-last_sample_time_us;
  last_sample_time_us = current_time_us;
  return delta_ticks*1e6/delta_time;
}
