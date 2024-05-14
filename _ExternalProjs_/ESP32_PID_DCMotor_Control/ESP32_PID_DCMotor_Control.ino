#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

volatile double T, Pulse, Speed, SpeedSet, E, E1, E2;

volatile int ModePID, Direction, ParamterPID;

volatile double Alpha, Beta, Gamma, Kp, Kd, Ki;
volatile double Output, LastOutput; 

unsigned int TimeMilliSeconds = 100; // milli seconds
unsigned int CurrentTime, LastTime = 0;

#define ENCODER_INPUT_PIN 27 // kd 0.03 ki 0.06
#define ENABLE_PIN        14
#define DIR1_PIN          25
#define DIR2_PIN          26

#define LED_FREQUENCY     1000
#define LED_CHANNEL       0
#define LED_RESOLUTION    12

#define BTN_MODE_PID          32
#define BTN_SWITCH            35
#define BTN_INCREASE          34
#define BTN_DECREASE          23

#define PID_KP_SCALE 0.1
#define PID_KD_SCALE 0.01
#define PID_KI_SCALE 0.01

#define SPEED_MOTOR_SCALE 100

// Setting
static const uint8_t msgQueueLen = 10;

// Globals
static QueueHandle_t msgQueue;

typedef struct {
  int parameter;
  int value;
} PID_PARAM_PACKAGE;

volatile PID_PARAM_PACKAGE package1;
volatile PID_PARAM_PACKAGE item;

void setup() {
  // put your setup code here, to run once:
  //pinMode(2, INPUT_PULLUP);
  Serial.begin(115200);
  pinMode(ENCODER_INPUT_PIN, INPUT_PULLUP);
  pinMode(DIR1_PIN, OUTPUT);
  pinMode(DIR2_PIN, OUTPUT);

  pinMode(BTN_MODE_PID, INPUT);
  pinMode(BTN_SWITCH, INPUT);
  pinMode(BTN_INCREASE, INPUT);
  pinMode(BTN_DECREASE, INPUT);

  ModePID = Direction = 0;

  SpeedSet = 2500; Speed = 0;
  E = E1 = E2 = Output = LastOutput = 0;
  T = 0.1;
  Kp = 0.6; Kd = 0.03; Ki = 0.6;
  attachInterrupt(digitalPinToInterrupt(ENCODER_INPUT_PIN), countPulse, FALLING);

  // analogWrite(ENABLE_PIN, 255);
  digitalWrite(DIR1_PIN, HIGH);
  digitalWrite(DIR2_PIN, LOW);

  ledcAttachPin(ENABLE_PIN, LED_CHANNEL);
  ledcSetup(LED_CHANNEL, LED_FREQUENCY, LED_RESOLUTION);

  lcd.begin();
	lcd.backlight();
  	
  lcd.print("Hello, world!");
  lcd.clear();

  msgQueue = xQueueCreate(msgQueueLen, sizeof(PID_PARAM_PACKAGE));

  xTaskCreatePinnedToCore(menuLoop, 
                          "Menu Loop",
                          4096,
                          NULL,
                          1,
                          NULL,
                          0);

  item.parameter = 100;
  item.value = 0;
}

void countPulse()
{
  if (digitalRead(ENCODER_INPUT_PIN) == LOW)
  {
    Pulse++;
  }
}

void PID()
{
  Speed = ((Pulse/4.0)*(1/(double)T))*60;
  Pulse = 0;
  E = SpeedSet - Speed;
  Alpha = 2*T*Kp + Ki*T*T + 2*Kd;
  Beta = T*T*Ki - 4*Kd - 2*T*Kp;
  Gamma = 2*Kd;
  Output = (Alpha*E + Beta*E1 + Gamma*E2 + 2*T*LastOutput)/(double)(2*T);
  LastOutput = Output;
  E2 = E1;
  E1 = E;
  if (Output > 4095) Output = 4095;
  if (Output < 0) Output = 0;

  if (Output > 0)
  {
    ledcWrite(LED_CHANNEL, Output);
    // digitalWrite(DIR1_PIN, HIGH);
    // digitalWrite(DIR2_PIN, LOW);
  }
  else
  {
    ledcWrite(LED_CHANNEL, 0);
    // digitalWrite(DIR1_PIN, LOW);
    // digitalWrite(DIR2_PIN, LOW);
  }
}

int readButtons(int btn)
{
  if (digitalRead(btn) == LOW)
  {
    vTaskDelay(10/portTICK_PERIOD_MS);
    if (digitalRead(btn) == LOW)
    {
      while (digitalRead(btn) == LOW);
      return 1;
    }
  }
  return 0;
}

// Code for parameters
// 100, 101, 102 : kp, kd, ki
// 110           : direction
// 120           : speed

void HandlePrintParameters()
{
  Serial.print("Speed:");
  Serial.println(Speed);
    Serial.print(" Output:");
    Serial.print(Output);
    Serial.println();
  vTaskDelay(10/portTICK_PERIOD_MS);
}

void HandleChangeParametersPID()
{
  if (readButtons(BTN_SWITCH))
  {
    ParamterPID = (ParamterPID + 1) % 3;
    switch (ParamterPID)
    {
      case 0:
      {
        lcd.clear();
        item.parameter = 100;
        break;
      }
      case 1:
      {
        lcd.clear();
        item.parameter = 101;
        break;
      }
      case 2:
      {
        lcd.clear();
        item.parameter = 102;
        break;
      }
      default:
      {
        item.parameter = 100;
      }
    } 
  }
  if (readButtons(BTN_INCREASE))
  {
    item.value = 1;
    xQueueSend(msgQueue, (void *)&item, 10);
  }
  else if (readButtons(BTN_DECREASE))
  {
    item.value = -1;
    xQueueSend(msgQueue, (void *)&item, 10);
  }
}

void handleChangeSpeedDirection()
{
  if (readButtons(BTN_SWITCH))
  {
    lcd.clear();

    Direction = (Direction + 1) % 2;
    item.parameter = 110;
    item.value = Direction;
    xQueueSend(msgQueue, (void *)&item, 10);
  }
  else if (readButtons(BTN_INCREASE))
  {
    lcd.clear();
    
    item.parameter = 120;
    item.value = 1;
    xQueueSend(msgQueue, (void *)&item, 10);
  }
  else if (readButtons(BTN_DECREASE))
  {
    lcd.clear();
    
    item.parameter = 120;
    item.value = -1;
    xQueueSend(msgQueue, (void *)&item, 10);
  }
}

void menuLoop(void *parameter) {
  void (*handleFunction)() = HandlePrintParameters;
  while (1) {
    if (readButtons(BTN_MODE_PID))
    {
      ModePID = (ModePID + 1) % 3;
      if (ModePID == 0)
      {
        handleFunction = HandlePrintParameters;
      }
      if (ModePID == 1)
      {
        handleFunction = HandleChangeParametersPID;
      }
      if (ModePID == 2)
      {
        handleFunction = handleChangeSpeedDirection;
      }
    }
    if (handleFunction != NULL) handleFunction();

    vTaskDelay(10/portTICK_PERIOD_MS);
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  if (xQueueReceive(msgQueue, (void *)&package1, 0) == pdTRUE)
  {
    // Handle
    if (package1.parameter == 100) {
      package1.value == 1 ? Kp += PID_KP_SCALE : Kp -= PID_KP_SCALE;
      if (Kp < 0) Kp = 0;
    }
    else if (package1.parameter == 101) {
      package1.value == 1 ? Kd += PID_KD_SCALE : Kd -= PID_KD_SCALE;
      if (Kd < 0) Kd = 0;
    } 
    else if (package1.parameter == 102) {
      package1.value == 1 ? Ki += PID_KI_SCALE : Ki -= PID_KI_SCALE;
      if (Ki < 0) Ki = 0;
    }

    else if (package1.parameter == 110)
    {
      if (package1.value == 1)
      {
        digitalWrite(DIR1_PIN, HIGH);
        digitalWrite(DIR2_PIN, LOW);
      }
      else
      {
        digitalWrite(DIR1_PIN, LOW);
        digitalWrite(DIR2_PIN, HIGH);
      }
    }
    else if (package1.parameter == 120) {
      package1.value == 1 ? SpeedSet += SPEED_MOTOR_SCALE : SpeedSet -= SPEED_MOTOR_SCALE;
      if (SpeedSet < 0) SpeedSet = 0;
    } 

    Serial.print("Params:");
    Serial.println(package1.parameter);
    Serial.print("Value:");
    Serial.println(package1.value);
    Serial.print("Kp:");
    Serial.print(Kp);
    Serial.print(" Kd:");
    Serial.print(Kd);
    Serial.print(" Ki:");
    Serial.print(Ki);
    Serial.print(" Speed set:");
    Serial.print(SpeedSet);
    // Serial.print(" Output:");
    // Serial.print(Output);
    Serial.println();
  }
  CurrentTime = millis();
  if (CurrentTime - LastTime > TimeMilliSeconds)
  {
    LastTime = millis();
    // Speed = (Pulse/20.0)*60;
    // Pulse = 0;
    PID();
    Serial.println(Speed);
  }
}
