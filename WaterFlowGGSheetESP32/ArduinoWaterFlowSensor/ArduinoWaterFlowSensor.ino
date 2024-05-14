#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

#define FLOWSENSOR_PIN 2   // Sensor Input

volatile int FlowFrequency;  // Measures flow sensor pulses
// Calculated litres/hour
float Volumn = 0.0, LitrePerMinute;

unsigned long CurrentTime, CloopTime = 0;

LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial mySerial(10, 11); // RX, TX

void flow()  // Interrupt function
{
  FlowFrequency++;
}

void setup() {
  Serial.begin(9600);

  mySerial.begin(9600);
  
  pinMode(FLOWSENSOR_PIN, INPUT);
  digitalWrite(FLOWSENSOR_PIN, HIGH);  // Optional Internal Pull-Up
  
  lcd.begin();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Water Flow Meter");
  lcd.setCursor(0, 1);

  attachInterrupt(digitalPinToInterrupt(FLOWSENSOR_PIN), flow, RISING);  // Setup Interrupt
}

void loop() {
  CurrentTime = millis();
  // Every second, calculate and print litres/hour
  if (CurrentTime >= (CloopTime + 1000)) {
    CloopTime = CurrentTime;  // Updates cloopTime

    if (FlowFrequency != 0) {
      String getValue = "";
      // Pulse frequency (Hz) = 7.5Q, Q is flow rate in L/min.
      LitrePerMinute = (FlowFrequency / 7.5);  // (Pulse frequency x 60 min) / 7.5Q = flowrate in L/hour
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Rate: ");
      lcd.print(LitrePerMinute);
      getValue += String(LitrePerMinute);
      getValue += ",";
      lcd.print(" L/M");
      LitrePerMinute = LitrePerMinute / 60;
      lcd.setCursor(0, 1);

      Volumn = Volumn + LitrePerMinute;
      getValue += String(Volumn);
      getValue += ",";
      lcd.print("Vol:");
      lcd.print(Volumn);
      lcd.print(" L");
      FlowFrequency = 0;                  // Reset Counter
      Serial.print(LitrePerMinute, DEC);  // Print litres/hour
      getValue += String(LitrePerMinute);
      getValue += ",";
      Serial.println(" L/Sec");
      
      mySerial.write(getValue.c_str());
    }
    else {
      Serial.println(" flow rate = 0 ");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Rate: ");
      lcd.print(FlowFrequency);
      lcd.print(" L/M");

      lcd.setCursor(0, 1);
      lcd.print("Vol:");
      lcd.print(Volumn);
      lcd.print(" L");
    }
  }
}