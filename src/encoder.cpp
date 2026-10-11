#include "encoder.h"
#include "config.h"
#include "Arduino.h"
#include "util/atomic.h"

volatile long encoder_ticks = 0;


void ISR_VA(){
  if(digitalRead(ENCODER_LEFT_VB)){
    encoder_ticks++;  
  }else{
    encoder_ticks--;
  }
}

bool encoder_init(void){
  pinMode(ENCODER_LEFT_VA, INPUT_PULLUP);
  pinMode(ENCODER_LEFT_VB, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_LEFT_VA), ISR_VA, RISING);
  return true;
}

int32_t encoder_get_ticks(void){
    int32_t result;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        result = encoder_ticks;
    }
    return result;
}

int32_t encoder_get_and_reset_delta(void){
    int32_t result = encoder_get_ticks();
    encoder_reset();
    return result;
}

void encoder_reset(void){
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        encoder_ticks = 0;
    }
}

