#include <Arduino.h>
#include <P1AM.h>

enum MachineStates {
  Waiting, // 0
  ColorSensing, // 1
  CountedMovement, //2 
  EjectState, // 3
  ArmSignal // 4
  };

MachineStates currentState = Waiting; 

// modules
int modInput = 1;
int modOutput = 2; 
int modAnalog = 3; 

// inputs
int pulse = 1;
int lbIn = 2; 
int lbOut = 3; 
int lbWhite = 4;
int lbRed = 5; 
int lbBlue = 6; 

// outputs
int conveyor = 1; 
int compressor = 2; 
int ejectW = 3; 
int ejectR = 4; 
int ejectB = 5; 
int armW = 6; 
int armR = 7; 
int armB = 8; 

// analog intputs 
int color = 1; 

// vars 
int colorValue = 10000;
int distanceToEject = 0; 
int distanceMoved = 0;
bool prevKeyState = false;
bool currentKey = false;
char targetColor = 'b';


void setup() {
  delay(1000);
  Serial.begin(9600);
  delay(1000);

  // Start up P1am modules
  while (!P1.init()){
    delay(10);
  } 
}

// helpful to read easier 
bool InputTriggered(){ 
  return !P1.readDiscrete(modInput, lbIn);
}

bool OutputTriggered(){ 
  return !P1.readDiscrete(modInput, lbOut);
}

void ToggleConveyor(bool s){
  P1.writeDiscrete(s ,modOutput, conveyor);
}

int GetColor(){
  return P1.readAnalog(modAnalog, color);
}

bool GetPulseKey(){ 
  return P1.readDiscrete(modInput, pulse);
}

void ToggleCompressor(bool s){
  P1.writeDiscrete(s, modOutput, compressor);
}

void UseEjector(char c){
  int tempPin;
  if (c == 'w'){
    tempPin = ejectW; 
  } else if (c == 'r'){
    tempPin = ejectR; 
  } else {
    tempPin = ejectB; 
  }
  P1.writeDiscrete(true, modOutput, tempPin);
  delay(1500);
  P1.writeDiscrete(false, modOutput, tempPin);
}

void OverheadSignal(char c){
  int armSig; 
  int colorSig = 1;
  if (c == 'w'){
    armSig = armW; 
    // while loop and ones below it ensure there are not misloads and ensures there is actually a part in the load area before sending a signal to the robot to pick it up 
    while (colorSig ==1){
      colorSig = P1.readDiscrete(modInput, lbWhite);
      Serial.println(colorSig);
      delay(200);
    } 
    if (colorSig != 1){
      Serial.println("Robot moves to grab the WHITE cylinder");
      // P1.writeDiscrete(true, modOutput, armSig);
    }
  } else if (c == 'r'){
    armSig = armR; 
    while (colorSig ==1){
      colorSig = P1.readDiscrete(modInput, lbRed);
      Serial.println(colorSig);
      delay(200);
    } 
    if (colorSig != 1){
      Serial.println("Robot moves to grab the RED cylinder");
      // P1.writeDiscrete(true, modOutput, armSig);
    }  
  } else {
    while (colorSig ==1){
      colorSig = P1.readDiscrete(modInput, lbBlue);
      Serial.println(colorSig);
      delay(200);
    } 
    if (colorSig != 1){
      Serial.println("Robot moves to grab the BLUE cylinder");
      // P1.writeDiscrete(true, modOutput, armSig);
    }  
  }
}

void loop() {
  switch(currentState) 
  { 
    case Waiting: 
    // wait for light barrier to be broken 
    // after tripped, switch state to color sensing mode and turn on conveyor
    if (InputTriggered()){
      currentState = ColorSensing; 
      ToggleConveyor(true);
      colorValue = 10000;
    }
    break; 

    case ColorSensing: 
    // Get color and find the min 
    colorValue = min(GetColor(), colorValue);
    // Keep on going until second light barrier
    // then switch states 
    if (OutputTriggered()){ 
      currentState = CountedMovement;  
      distanceMoved = 0;  
      // decide how far to move 
      if (colorValue < 2500){
        distanceToEject = 3;
        targetColor = 'w';
      } else if (colorValue < 4600){
        distanceToEject = 9;
        targetColor = 'r';
      } else {
        distanceToEject = 14;
        targetColor = 'b';
      }
      ToggleCompressor(true);
    }   
    break;

    case CountedMovement: 
    // watch pulse key to move that far 
    currentKey = GetPulseKey();
    if (currentKey && !prevKeyState){
      distanceMoved++;
    }
    prevKeyState = currentKey;
    // switch states and turn off conveyor 
    if (distanceMoved >= distanceToEject){
      currentState = EjectState;
      ToggleConveyor(false);
    }
    break;

    case EjectState: 
    UseEjector(targetColor);
    currentState = ArmSignal;
    break;

    case ArmSignal:
    OverheadSignal(targetColor);
    currentState = Waiting;
    break;
  }
}
