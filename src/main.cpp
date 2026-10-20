#include <Arduino.h>
#include "robot_app.h"
#include "scheduler.h"
void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  robot_app_init(micros());
  if(!scheduler_init(micros())) Serial.println("pb_init");
}



void loop()
{
  scheduler_run(service);
}
