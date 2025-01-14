#include <AccelStepper.h>
#define motorPin1 8
#define motorPin2 9
#define motorPin3 10
#define motorPin4 11
#define MotorInterfaceType 8
int int_cahaya = A0;
int air = A2;
AccelStepper stepper = AccelStepper(MotorInterfaceType,
motorPin1, motorPin3, motorPin2, motorPin4);
const int bawah=0;
const int atas=1024;
void setup() {
    pinMode(int_cahaya,INPUT);
    pinMode(air,INPUT);
  stepper.setMaxSpeed(500);
    stepper.setAcceleration(900);
    Serial.begin(9600);
}

void loop() {
  int data_air=analogRead(air);
  int range=map(data_air,bawah,atas,0,3);
  int cahaya=analogRead(int_cahaya);
  Serial.print("Air : ");
  Serial.println(range);
  Serial.print("Cahaya : ");
  Serial.println(cahaya);
  if((cahaya<=110)&&(range==2))
  { stepper.runToNewPosition(10050); }
  else
  { stepper.runToNewPosition(0); }
 }
