#include  <Adafruit_ST7735.h>
#include  <Adafruit_GFX.h>
#include  <SPI.h>
#include <ESP32Servo.h>
#include <BluetoothSerial.h>
#define TIMEOUT        3000

#define TFT_CS        5
#define TFT_RST       4                      
#define TFT_DC        2
#define TFT_SCLK      18   
#define TFT_MOSI      23
#define SW1           13
#define SW2           14
#define SW3           15
#define RELAY         25

#define START_BUTTON 32
#define STOP_BUTTON 33
#define LED_INDICATOR 2

Servo servoMotor1;
Servo servoMotor2;

int servo1Pin = 26; // Example, need to redefine
int servo2Pin = 27;
// Published values for SG90 servos; adjust if needed
int minUs = 500;
int maxUs = 2500;

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);
BluetoothSerial SerialBT;


int State = 0;

bool readSwitch1()
{
  return digitalRead(SW1) == 0 ? true : false;
}
bool readSwitch2()
{
  return digitalRead(SW2) == 0 ? true : false;
}
bool readSwitch3()
{
  return digitalRead(SW3) == 0 ? true : false;
}

void RunMachine()
{
  tft.fillScreen(ST7735_BLACK);
  printText("ON", ST7735_GREEN,23,70,2);
  printText("Hook on", ST7735_BLUE,20,90,2);
  for (int i = 0; i < 180; i++)
  {
    servoMotor1.write(i);
    delay(30);

    if (SerialBT.available() > 0)
    {
      if (getDatacommand().startsWith("Stop"))
      {
        return;
      }
    }
    if (readSwitch2() == true) break;
    if (digitalRead(STOP_BUTTON) == 0) return;
  }
  
  tft.fillScreen(ST7735_BLACK);
  printText("ON", ST7735_GREEN,20,70,2);
  printText("Pull string", ST7735_BLUE,20,90,2);
  digitalWrite(RELAY, HIGH);
  for (int i = 0; i < 100; i++)
  {
    delay(20);
    if (SerialBT.available() > 0)
    {
      if (getDatacommand().startsWith("Stop"))
      {
        return;
      }
    }
    if (digitalRead(STOP_BUTTON) == 0) return;
  }
  digitalWrite(RELAY, LOW);

  tft.fillScreen(ST7735_BLACK);
  printText("ON", ST7735_GREEN,23,70,2);
  printText("Cut string", ST7735_BLUE,20,90,2);

  for (int i = 0; i < 75; i++)
  {
    servoMotor2.write(0);
      if (SerialBT.available() > 0)
      {
        if (getDatacommand().startsWith("Stop"))
        {
          return;
        }
      }
    if (digitalRead(STOP_BUTTON) == 0) return;
    delay(30);
  }

  tft.fillScreen(ST7735_BLACK);
  printText("ON", ST7735_GREEN,20,70,2);
  printText("Return", ST7735_BLUE,20,90,2);
}

void ResetMachine()
{
  servoMotor1.write(0);
  int getLastTime = millis();
  while (readSwitch3() != true && abs(int(millis() - getLastTime)) <= TIMEOUT)
  {
    servoMotor2.write(180);
  }
  servoMotor2.write(90);
  digitalWrite(RELAY, LOW);
}

String getDatacommand()
{
  char c;
  String s = "";
  while (SerialBT.available())
  {
    c = SerialBT.read();
    s += c;
  }
  return s;
}

void callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  if (event == ESP_SPP_SRV_OPEN_EVT) {
    Serial.println("A client is connected to ESP32");
  }
}

void printText(char *text, uint16_t color, int x, int y,int textSize)
{
  tft.setCursor(x, y);
  tft.setTextColor(color);
  tft.setTextSize(textSize); // 1 is default 6x8 px, 2 is 12x16, 3 is 18x24, etc
  tft.setTextWrap(true);
  tft.print(text);
}

void setup () 
{
  Serial.begin(9600);
  if (!SerialBT.begin("ESP32-Bluetooth")) {
    Serial.println("An error occurred initializing Bluetooth");
  } else {
    Serial.println("Bluetooth initialized");
  }
  SerialBT.register_callback(callback);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(3);
  tft.fillScreen(ST7735_BLACK);
  printText("OFF", ST7735_GREEN,20,70,2);

  ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
  
  // For servo config
  servoMotor1.setPeriodHertz(50);      // Standard 50hz servo
	servoMotor2.setPeriodHertz(50);      // Standard 50hz servo

  servoMotor1.attach(servo1Pin, minUs, maxUs);
	servoMotor2.attach(servo2Pin, minUs, maxUs);
  servoMotor1.write(0);
  servoMotor2.write(90);
  pinMode(SW1, INPUT_PULLUP);
  pinMode(SW2, INPUT_PULLUP);
  pinMode(SW3, INPUT_PULLUP);
  pinMode(RELAY, OUTPUT);
  pinMode(START_BUTTON, INPUT_PULLUP);
  pinMode(STOP_BUTTON, INPUT_PULLUP);
  // pinMode(LED_INDICATOR, OUTPUT);

  digitalWrite(SW1, HIGH);
  digitalWrite(SW2, HIGH);
  digitalWrite(SW3, HIGH);
  digitalWrite(START_BUTTON, HIGH);
  digitalWrite(STOP_BUTTON, HIGH);
  
  int getLastTime = millis();
  while (readSwitch3() != true && abs(int(millis() - getLastTime)) <= TIMEOUT)
  {
    servoMotor2.write(180);
  }
  servoMotor2.write(90);
}

void loop () {
  if (digitalRead(START_BUTTON) == 0)
  {
    State = 1;
  }
  if (SerialBT.available() > 0)
  {
    if (getDatacommand().startsWith("Run"))
    {
      State = 1;
    }
  }
  if (Serial.available() > 0)
  {
    char c = Serial.read();
    if (c == 's')
    {
      State = 1;
    }
    else if (c == 'p')
    {
      State = 0;
    }
  }
  if (State == 1)
  {
    RunMachine();
    ResetMachine();
    State = 0;
    tft.fillScreen(ST7735_BLACK);
    printText("OFF", ST7735_BLUE,20,70,2);
  }
  delay(100);
}
