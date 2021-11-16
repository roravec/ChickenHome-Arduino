#include <Servo.h> 

#define CHECK_LIGHT_SLEEP_TIME 5000
#define SERVO_CHECK_SLEEP_TIME 20
#define CHECK_LIGHT_TICKS_TO_DAYSTATE_CHANGE 5

enum DayState {
  unknownDayState,
  dayDayState,
  nightDayState
};

Servo myservo;  // create servo object to control a servo 
                // a maximum of eight servo objects can be created 
const short int servoPin = 10;
const short int buttonUpPin = 12;
const short int buttonDownPin = 8;  
const short int photosensorAnalogPin = A0; 

const short ledRedPin = 3;
const short ledGreenPin = 5;
const short ledBluePin = 6;

DayState currentDayState = unknownDayState;
DayState previousDayState = unknownDayState;
DayState dayStateToBeSet = unknownDayState;
unsigned int TicksOnOtherDayState = 0;

short int rotateDirectionOpen = 180;
short int rotateDirectionClose = 0;
short int rotateDirection = -1;    // variable to store the servo position 
short int lightCal = 0;
short int lightVal = 0;
short int lightValNight = 280;
short int lightValDay = 400;

// lower the value means darker

void setup() 
{ 
  Serial.begin(9600);
  pinMode(ledRedPin, OUTPUT);
  pinMode(ledGreenPin, OUTPUT);
  pinMode(ledBluePin, OUTPUT);
  
  pinMode(buttonUpPin, INPUT_PULLUP);
  pinMode(buttonDownPin, INPUT_PULLUP);
  lightCal = analogRead(photosensorAnalogPin);
  //UpdateLedByDoorState();
  UpdateLightValue();
  CheckLightAndChangeState();
} 

void loop() 
{ 
  UpdateLightValue();
  if (rotateDirection == -1) // not rotating, waiting for night or morning
  {
    UpdateLedByDoorState();
    delay(CHECK_LIGHT_SLEEP_TIME);
    CheckLightAndChangeState();
  }
  if (rotateDirection != -1) // wait for button to stop
  {
    UpdateLedByDoorState();
    StopRotateServoIfInPosition();
    delay(SERVO_CHECK_SLEEP_TIME);
  }
}

void LightLED(short int r, short int g, short int b)
{
  digitalWrite(ledRedPin, r);
  digitalWrite(ledGreenPin, g);
  digitalWrite(ledBluePin, b);
}

bool DetectIfDoorOpen()
{
  int buttonOpenState = digitalRead(buttonUpPin);
  delay(10);
  if (buttonOpenState == HIGH)
  {
    buttonOpenState = digitalRead(buttonUpPin);
    if (buttonOpenState == HIGH) // clear bounce
    {
      return true;
    }
  }
  return false;
}
bool DetectIfDoorClosed()
{
  int buttonClosedState = digitalRead(buttonDownPin);
  delay(10);
  if (buttonClosedState == HIGH)
  {
    buttonClosedState = digitalRead(buttonDownPin);
    if (buttonClosedState == HIGH) // clear bounce
    {
      return true;
    }
  }
  return false;
}

void UpdateLedByDoorState()
{
  bool openDoor = DetectIfDoorOpen();
  bool closedDoor = DetectIfDoorClosed();
  //LightLED(0, closedDoor, openDoor);
  /*Serial.print(" OPEN: ");
  Serial.print(openDoor);
  Serial.print(" CLOSED: ");
  Serial.print(closedDoor);
  Serial.println();*/
}
void UpdateLightValue()
{
  lightVal = analogRead(photosensorAnalogPin);
  TurnOnLedByLightSensor(lightVal);
}

void CheckLightAndChangeState()
{
  UpdateLightValue();
  //lightVal = analogRead(photosensorAnalogPin);
  //TurnOnLedByLightSensor(lightVal);
  Serial.print("Light: ");
  Serial.print(lightVal);
  Serial.println();
  Serial.print("Current DayState before change: ");
  Serial.print(currentDayState);
  Serial.println();
  SetDayState(lightVal);
}
void TurnOnLedByLightSensor(int value)
{
  if (value <= lightValNight)
    LightLED(0,0,1);
   else if (value > lightValNight && value < lightValDay)
    LightLED(0,1,0);
   else
    LightLED(1,0,0);
}

void SetDayState(short int lightVal)
{
  if (currentDayState == unknownDayState) // set new state instantly
  {
    if (lightVal > lightValNight)
      SetNewDayState(dayDayState);
    else
      SetNewDayState(nightDayState);
    dayStateToBeSet = currentDayState;
  }
  else
  {
    DayState newDayState = unknownDayState;
    if (currentDayState == dayDayState && lightVal <= lightValNight)
      newDayState = nightDayState;
    else if (currentDayState == nightDayState && lightVal >= lightValDay)
      newDayState = dayDayState;
    if (newDayState != unknownDayState)
      DayStateChangeTickValidator(newDayState); // state was changed
  }
}

bool DayStateChangeTickValidator(DayState dayState)
{
  if (dayStateToBeSet == unknownDayState) // speed up the process
  {
    dayStateToBeSet = dayState;
    TicksOnOtherDayState = CHECK_LIGHT_TICKS_TO_DAYSTATE_CHANGE;
  }
  if (dayStateToBeSet == dayState)
  {
    TicksOnOtherDayState++;
    if (TicksOnOtherDayState >= CHECK_LIGHT_TICKS_TO_DAYSTATE_CHANGE && currentDayState != dayStateToBeSet) // change state
    {
      SetNewDayState(dayState);
      return true;
    }
    else
      return false;
  }
  else
  {
    dayStateToBeSet = dayState;
    TicksOnOtherDayState=0;
    return false;
  }
}
void SetNewDayState(DayState newState)
{
  Serial.print("State change: ");
  Serial.print(newState);
  Serial.println();
  previousDayState = currentDayState;
  currentDayState = newState;
  RotateServo(currentDayState);
}

void RotateServo(DayState currentState)
{
  switch (currentState)
  {
    case unknownDayState:
      RotateServo(rotateDirectionOpen);
      break;
    case dayDayState:
      RotateServo(rotateDirectionOpen);
      break;
    case nightDayState:
      RotateServo(rotateDirectionClose);
      break;
    default:
      RotateServo(rotateDirectionOpen);
      break;
  }
}

void RotateServo(short int rotDirection)
{
  rotateDirection = rotDirection;
  myservo.attach(servoPin);
  myservo.write(rotateDirection);
}
void StopRotateServoIfInPosition()
{
  if (rotateDirection == rotateDirectionOpen && DetectIfDoorOpen()) // going to open
  {
    //Serial.println("Servo in position");
    StopRotateServo();
  }
  if (rotateDirection == rotateDirectionClose && DetectIfDoorClosed()) // going to close
  {
    //Serial.println("Servo in position");
    StopRotateServo();
  }
}
void StopRotateServo()
{
  myservo.detach();
  rotateDirection = -1;
}

void ResetDoorStateWithRotation()
{
  if (!DetectIfDoorOpen() && !DetectIfDoorClosed())
    RotateServo(rotateDirectionOpen);
}
