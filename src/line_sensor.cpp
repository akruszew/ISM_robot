#include "line_sensor.h"
#include "config.h"
#include <Arduino.h>

bool line_sensor_init(void){

    // il faudrait tester si LINE_SENSOR_1 et LINE_SENSOR_2 sont valides

    pinMode(LINE_SENSOR_1, INPUT_PULLUP);
    pinMode(LINE_SENSOR_2, INPUT_PULLUP);
    return true;
}


bool line_sensor_read(LineReading *reading){
    if(reading==NULL)
    {
        return false;
    }

    reading->left_detected = digitalRead(LINE_SENSOR_1)==0;
    reading->right_detected = digitalRead(LINE_SENSOR_2)==0;
    return true;

}