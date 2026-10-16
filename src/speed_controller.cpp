#include "speed_controller.h"
#include <stddef.h>
#include <math.h>
void speed_controller_init(SpeedController *controller){
    if(controller==NULL){
        return;
    }
    controller->integral = 0.0f;

}




bool speed_controller_update(
    SpeedController *controller,
    float reference,
    float measurement,
    float dt,
    float *voltage_v)
{
    if(controller==NULL){
        return false;
    }

    if(voltage_v==NULL){
        return false;
    }

    if( (!isfinite(reference))  
        || (!isfinite(measurement)) 
        || (!isfinite(dt)) )
    {
        return false;
    }

    if(dt<0){
        return false;
    }
    
    float error = reference - measurement;
    
    float u = controller->kp * error 
        + controller-> ki * controller->integral;
    /* antiwindup*/
    if ((controller->ki * error)>0 && 
            u > controller->out_max) 
    {
        /* Clamping*/
        *voltage_v = controller->out_max;
        return true;
    } 
    
    if ((controller->ki * error)<0 && 
                u < controller->out_min) 
    {
        /* Clamping*/
        *voltage_v = controller->out_min;
        return true;
    } 

    /* no saturation = integrate */
    controller->integral += dt * error;
    
    
    u = u>controller->out_max ? controller->out_max : u;
    u = u<controller->out_min ? controller->out_min : u;
    *voltage_v = u;
    return true;

}
