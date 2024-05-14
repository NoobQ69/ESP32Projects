#define LED_WIFI 3
#define RESET_WIFI_BTN  7
#define SETCARD_BTN     5
#define SETTIMEOUT_BTN  6
#define BUZZER   4

void setup() {
  // put your setup code here, to run once:
  pinMode(LED_WIFI, OUTPUT);
  pinMode(RESET_WIFI_BTN, INPUT);
  pinMode(SETCARD_BTN, INPUT);
  pinMode(SETTIMEOUT_BTN, INPUT);
  pinMode(BUZZER, INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  int result = digitalRead(RESET_WIFI_BTN);
  if (result == HIGH)
  {
    Serial.println("Reset WiFi button is pushed");
  }
  result = digitalRead(SETCARD_BTN);
  if (result == HIGH)
  {
    Serial.println("Set card button is pushed");
  }
  result = digitalRead(SETTIMEOUT_BTN);
  if (result == HIGH)
  {
    digitalWrite(LED_WIFI, HIGH);
    // Serial.println("Set timeout button is high");
  }
  else
  {
    digitalWrite(LED_WIFI, LOW);
    // Serial.println("Set timeout button is low");
  }


  delay(100);
}
