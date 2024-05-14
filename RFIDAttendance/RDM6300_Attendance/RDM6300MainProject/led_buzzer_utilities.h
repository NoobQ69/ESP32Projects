#ifndef LED_BUZZER_UTILITIES_H
#define LED_BUZZER_UTILITIES_H

#define LED_WIFI 32
#define BUZZER   4

enum buzzer_sound_type 
{
  BUZZER_SUCCESS_READ_WRITE = 100,
  BUZZER_SUCCESS_SEND_RECIEVE = 101,
  BUZZER_ERROR_READ_WRITE = 110,
  BUZZER_ERROR_SEND_RECIEVE = 111,
  BUZZER_POWER_ON = 200
};

enum buzzer_sound_time 
{
  BUZZER_SHORT_TIME = 100,
  BUZZER_MEDIUM_TIME = 200,
  BUZZER_LONG_TIME = 800,
};

enum led_state 
{
  LED_OFF = LOW,
  LED_ON = HIGH
};

// FOR LED IDICATORS AND BUZZER
void SetLED(int type, int state)
{
  if (type == LED_WIFI) digitalWrite(LED_WIFI, state);
  // else if (type == LED_CARD) digitalWrite(LED_CARD, state);
}

void BlinkLED(int ledType, int times = 1, int timeInterval = 100)
{
  for (int i = 0; i < times; i++){
    SetLED(ledType, LED_ON);
    vTaskDelay(timeInterval/portTICK_PERIOD_MS);
    SetLED(ledType, LED_OFF);
    vTaskDelay(timeInterval/portTICK_PERIOD_MS);
  }
}

void buzzerOscillates(int time, int numberOfTimes = 1) 
{
  for (int i = 0; i < numberOfTimes; i++)
  {
    digitalWrite(BUZZER, HIGH);
    vTaskDelay(time/portTICK_PERIOD_MS);
    digitalWrite(BUZZER, LOW);
    vTaskDelay(time/portTICK_PERIOD_MS);
  }
}

void BuzzerMakesSound(int type)
{
  if (type == BUZZER_SUCCESS_READ_WRITE) {
    buzzerOscillates(BUZZER_SHORT_TIME);
  }
  else if (type == BUZZER_SUCCESS_SEND_RECIEVE) {
    buzzerOscillates(BUZZER_SHORT_TIME, 2);
  }
  else if (type == BUZZER_ERROR_READ_WRITE) {
    buzzerOscillates(BUZZER_LONG_TIME);
  }
  else if (type == BUZZER_ERROR_SEND_RECIEVE) {
    buzzerOscillates(BUZZER_LONG_TIME, 2);
  }
  else if (type == BUZZER_POWER_ON) {
    buzzerOscillates(BUZZER_SHORT_TIME, 3);
  }
}

#endif