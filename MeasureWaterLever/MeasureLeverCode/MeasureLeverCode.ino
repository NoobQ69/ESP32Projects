#include <SoftwareSerial.h>
#include "HX710B.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Arduino_JSON.h>
// Khai báo chân GPIO dùng để điều khiển relay
const byte rxPin = A0;
const byte txPin = A1;
SoftwareSerial mySerial(rxPin, txPin);
#define RELAY_PIN 5
#define SCK_PIN 3
#define SDO_PIN 4
#define BUZZER 7
#define LOWER_PRESSURE  13700000
#define MIDDLE_PRESSURE 13740000
#define UPPER_PRESSURE  14200000
#define PRESSURE_RANGE    900000
long i = 0;
long j = 100;
String GetValue;
uint32_t data_raw = 0;
int volumn = 0;
HX710B air_press(SCK_PIN, SDO_PIN);
LiquidCrystal_I2C lcd(0x27, 20, 2);
JSONVar Msg;
String CANH_BAO;
String MOTOR;
String Ch,X,Y,Z;
String Chedo;
int Tocdo;
typedef struct {
  int setupLever;
  int currentLever;
  int state;
  int modeState;
} WATER_LEVER;
enum {
  STOP,
  START,
  MANUAL,
  AUTO
};
enum Lever {
  NOTFULL,
  FULL,
  ERROR_READING
};
WATER_LEVER Tub1;
bool ReadPressureSensor()
{
  if ( air_press.read(&data_raw, 500UL) != HX710B_OK )
  {
    mySerial.println(F("something error !"));
    return false;
  }
  else
  {
    mySerial.print(F("Data raw of ADC is : "));
    mySerial.println((unsigned long) data_raw);
  }

  return true;
}

int calculateWaterLever()
{
  // calculate
  if (!ReadPressureSensor())
  {
    return ERROR_READING;
  }
  else
  {
    volumn = map(data_raw, MIDDLE_PRESSURE, MIDDLE_PRESSURE + PRESSURE_RANGE, 0, 800);
    Tub1.currentLever = volumn;
  }
  if (Tub1.currentLever >= Tub1.setupLever)
  {
    CANH_BAO = "Full water";
    MOTOR = "OFF";
    tone(BUZZER, 2000);
    delay(1000);
    noTone(BUZZER);
    digitalWrite(RELAY_PIN, LOW);
    Tub1.state = STOP;
    return FULL;
  }
  return NOTFULL;
}
void setup() {
  // pinMode(floatPin, INPUT);
  Serial.begin(9600);
  mySerial.begin(9600);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  lcd.init();
  lcd.backlight();
  if ( !air_press.init() )
  {
    while (!air_press.init())
    {
      mySerial.println(F("HX710B not Found !"));
      delay(1000);
    }
  }

  digitalWrite(RELAY_PIN, LOW);
  String sendResponse = "SetupPressure=" + String(GetValue.substring(6));
  String sendResponse2 = "CurrentPressure=" + String(GetValue.substring(6));
  Tub1.setupLever = 250;
  Tub1.state = STOP;
  Tub1.modeState = MANUAL;

  Chedo = "Manual";
}

void loop() {
  if (Serial.available() > 0)
  {
    GetValue = Serial.readString();
    mySerial.println(GetValue);
    
    if (GetValue.startsWith("MANUAL"))
    {
      Chedo = "Manual";
      Tub1.modeState = MANUAL;
    }
    else if (GetValue.startsWith("AUTO"))
    {
      Chedo = "Auto";
      Tub1.modeState = AUTO;
    }
     else if (GetValue.startsWith("Vset"))
    {
      Tub1.setupLever = GetValue.substring(4).toInt();
      mySerial.println(Tub1.setupLever);
    }
    
    if (GetValue.startsWith("on"))
    {
      CANH_BAO = "";
      MOTOR = "ON";
      Tub1.state = START;
      digitalWrite(RELAY_PIN, HIGH);
    }
    
    if (Tub1.modeState == MANUAL)
    {
if (GetValue.startsWith("off"))
      {
        MOTOR = "OFF";
        Serial.println("Stop");
        Tub1.state = STOP;
        digitalWrite(RELAY_PIN, LOW);
      }
    }
  }

  if (Tub1.state == START)
  {
    // measure water lever
    lcd.setCursor(0, 0);
    lcd.print("                ");
    lcd.setCursor(0, 0);
    lcd.print("Water volumn:");
    lcd.print((char)(volumn % 1000 / 100 + 0x30));
    lcd.print((char)(volumn % 100 / 10 + 0x30));
    lcd.print((char)(volumn % 10 + 0x30));
    mySerial.println("Start");
    calculateWaterLever();
  }
  else if (Tub1.state == STOP)
  {
    mySerial.println("Stop");
  } 
  Msg["THE_TICH"] =  String(Tub1.currentLever);
  Msg["CANH_BAO"] =  CANH_BAO;
  Msg["MOTOR"] =  MOTOR;
  Msg["SET"] =  String (Tub1.setupLever);
  Msg["Ch"] =  Ch;
  Msg["Chedo"] =  Chedo; 
  String payLoad = JSON.stringify(Msg);
  Serial.println(payLoad);
  delay(1000);
}