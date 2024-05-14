// if you dont have FreeRTOS please install RTOS lib before upload
// link reference: https://youtu.be/JIr7Xm_riRs?si=gJ5iWQtCVBr4sgkt
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

#define STEP 17
#define DIR 16
#define EN 5
/* MS1 MS2 MS3
    0   0   0  Full step
    1   0   0  Half step
    0   1   0  Wave step
*/
#define MODEBIT0 4
#define MODEBIT1 15
#define MODEBIT2 18

#define BTN0 32
#define BTN1 33
#define BTN2 25
// #define MODEBIT2 5
unsigned long PreviousTime = 0;
unsigned long OperateMode = 0 /*0: full step, 1: half step, 2: wave step*/, DirectionMode = 0/*0: clockwise, 1: counter clockwise*/, StateMode = 1/*0: off, 1: on*/; 
unsigned char KeyBuffer[3] = {0, 0, 0};

char IsChangedModeStep = 0;
int Step = 200, CurrentStep = 200;

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  // for stepper driver
  pinMode(STEP, OUTPUT);
  pinMode(DIR, OUTPUT);
  pinMode(EN, OUTPUT);
  
  // // for control step on stepper driver
  // pinMode(MODEBIT0, OUTPUT);
  // pinMode(MODEBIT1, OUTPUT);
  // pinMode(MODEBIT2, OUTPUT);

  // // for buttons
  // pinMode(BTN0, INPUT_PULLUP);
  // pinMode(BTN1, INPUT_PULLUP);
  // pinMode(BTN2, INPUT_PULLUP);

  // Task to run forever
  xTaskCreate(        //
    RunStepperMotor,  // Function to be called
    "display Task",   // Name of task
    2048,             // Stack size (bytes in ESP32, word in FreeRTOS)
    NULL,             // parameter to pass function
    1,                // Task priority ( 0 to configMAX_PRIORITIES - 1)
    NULL);            // Run on one core for demo purposes (ESP32 only)

  /// initial setup 
  digitalWrite(EN, LOW);          // pull EN low to enable stepper to run stepper motor
  /* MODEBIT0 MODEBIT1 MODEBIT2
    0         0        0        Full step
    1         0        0        Half step
    0         1        0        Wave step
*/
	lcd.begin();

	// Turn on the blacklight and print a message.
	lcd.backlight();
	lcd.print("State:");
  
  // digitalWrite(BTN0, HIGH);
  // digitalWrite(BTN1, HIGH);
  // digitalWrite(BTN2, HIGH);
  // digitalWrite(MODEBIT0, LOW);
  // digitalWrite(MODEBIT1, LOW);
  // digitalWrite(MODEBIT2, LOW);
}

void RunOneStep(int speed = 800) {
  digitalWrite(STEP, HIGH);
  delayMicroseconds(speed);
  digitalWrite(STEP, LOW);
  delayMicroseconds(speed);
}

void RunStepperMotor(void *parameter) {
  while (1) {
    // quay đủ 1 vòng
    if (IsChangedModeStep)
    {
      IsChangedModeStep = 0;
      CurrentStep = Step;
    }

    for (int i = 0; i < CurrentStep; i++) {
      RunOneStep(1500);
    }
    delay(1000);
  }
}

void HandleModeStep(int bit2, int bit1, int bit0) {
  // digitalWrite(MODEBIT0, bit0);
  // digitalWrite(MODEBIT1, bit1);
  // digitalWrite(MODEBIT2, bit2);
}

int ReadOneButton(int button)
{
  if (digitalRead(button) == LOW)
  {
    delay(10);

    if (digitalRead(button) == LOW)
      while (digitalRead(button) == LOW);
    return 1;
  }
  return 0;
}

int ReadButtons()
{
  if (ReadOneButton(BTN0) == 1)
  {
    return 1;
  }
  else if (ReadOneButton(BTN1) == 1)
  {
    return 2;
  }
  else if (ReadOneButton(BTN2) == 1)
  {
    return 3;
  }
  return -1; // for exception
}

void HandleStateMode(int mode)
{
  if (mode == 0)
  {
    lcd.setCursor(0, 5);
    lcd.print("   ");
    lcd.print("OFF");
    Serial.println("OFF");
    digitalWrite(EN, HIGH);
    return;
  }
  lcd.setCursor(0, 5);
  lcd.print("   ");
  lcd.setCursor(0, 5);
  lcd.print("ON");
  Serial.println("ON");
  digitalWrite(EN, LOW);

}

void HandleOperateMode(int mode)
{
  if (mode == 0)
  {
    lcd.setCursor(1, 0);
    lcd.print("         ");
    lcd.setCursor(1, 0);
    lcd.print("FULL STEP");
    Serial.println("FULL STEP");
    Step = 200;
    IsChangedModeStep = 1;
    HandleModeStep(0,0,0);
  }
  if (mode == 1)
  {
    lcd.setCursor(1, 0);
    lcd.print("         ");
    lcd.setCursor(1, 0);
    lcd.print("HALF STEP");
    Serial.println("HALF STEP");
    Step = 400;
    IsChangedModeStep = 1;
    HandleModeStep(0,0,1);
  }
  if (mode == 2)
  {
    lcd.setCursor(0, 1);
    lcd.print("         ");
    lcd.setCursor(0, 1);
    lcd.print("WAVE STEP");
    Step = 800;
    IsChangedModeStep = 1;
    Serial.println("WAVE STEP");
    HandleModeStep(0,1,0);
  }
}

void HandleDirectionStep(int mode)
{
  if (mode == 0)
  {
    lcd.setCursor(12, 1);
    lcd.print("   ");
    lcd.setCursor(12, 1);
    lcd.print("CW");
    Serial.println("Clockwise");
    digitalWrite(DIR, LOW);
    return;
  }
  lcd.setCursor(12, 1);
  lcd.print("   ");
  lcd.setCursor(12, 1);
  lcd.print("CCW");
  Serial.println("Counter-clockwise");
  digitalWrite(DIR, HIGH);
}

void loop() {
  // int getButton = ReadButtons();

  // if (getButton == 1) {
  //   StateMode = (StateMode + 1) % 2;
  //   Serial.print("State change: ");
  //   HandleStateMode(StateMode);
  // } 
  // else if (getButton == 2) {
  //   OperateMode = (OperateMode + 1) % 3;
  //   Serial.print("Operate change: ");
  //   HandleOperateMode(OperateMode);
  // } 
  // else if (getButton == 3) {
  //   DirectionMode = (DirectionMode + 1) % 2;
  //   Serial.print("Direction change: ");
  //   HandleDirectionStep(DirectionMode);
  // }
  delay(1000);
}
