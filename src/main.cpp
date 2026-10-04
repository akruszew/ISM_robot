#include <Arduino.h>

#define DT 5
#define VA 18
#define VB 31
#define I1 34
#define I2 35
#define PWM 12

volatile long compteur = 0;

float getSpeed();
void ISR_VA();

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Hello");
  pinMode(VA, INPUT_PULLUP);
  pinMode(VB, INPUT_PULLUP);
  
  pinMode(I1, OUTPUT);
  pinMode(I2, OUTPUT);
  pinMode(PWM, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(VA), ISR_VA, RISING);
}


void Moteur(float voltage){
  voltage = constrain(voltage,-9.0, 9.0);
  if(voltage>0)
  {
    digitalWrite(I2,LOW);
    digitalWrite(I1,HIGH);
  }else{
    digitalWrite(I1,LOW);
    digitalWrite(I2,HIGH);
  }
  analogWrite(PWM, abs(voltage)*255.0/12.0   );
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
  noInterrupts();
  long compteur_copy = compteur;
  interrupts();
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
   
  Moteur(u);
  Serial.print(temps);
  Serial.print(" ,");
  Serial.print(u);
  Serial.print(" ,");
  Serial.print(getSpeed());
  Serial.print(" ;");
  
  Serial.println(" ");
}

volatile long  t_ISR_last = 0;
volatile long  t_ISR = 0;

void ISR_VA(){
  if(digitalRead(VB)){
    compteur++;  
  }else{
    compteur--;
  }
  t_ISR_last = t_ISR;
  t_ISR = micros();
}

float getSpeed(){
  float  t_ISR_last_copy;
  float  t_ISR_copy;
  noInterrupts();
  t_ISR_last_copy =t_ISR_last;
  t_ISR_copy = t_ISR;
  interrupts();
  if( t_ISR==t_ISR_last)
    return 0;
  else
    return (1.0/(t_ISR_copy-t_ISR_last_copy) )*1e6;
}