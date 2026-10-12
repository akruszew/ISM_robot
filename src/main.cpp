#include <Arduino.h>
#include "robot_app.h"


void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  robot_app_init(micros());
}

void loop()
{
  robot_app_run(micros());
  if(Serial.available()){
    Serial.read();
    robot_app_request_start();
  }
}
