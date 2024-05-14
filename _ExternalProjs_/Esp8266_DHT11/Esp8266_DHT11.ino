#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
// #include <BlynkSimpleEsp8266.h>
#include "DHT.h"
// #include <SimpleTimer.h> 
#define DHTTYPE DHT11
#define dht_dpin D4
DHT dht(dht_dpin, DHTTYPE); 
// SimpleTimer timer;
char auth[] = "Your Auth Code";
char ssid[] = "Your WiFI";
char pass[] = "Your Wifi Password";
float t;
float h;

int PreviousMillis = 0;

void setup()
{
    Serial.begin(9600);
    // Blynk.begin(auth, ssid, pass);
    dht.begin();
    // timer.setInterval(2000, sendUptime);
}

void loop()
{
  if (millis() - PreviousMillis > 2000) {
    PreviousMillis = millis();
    sendTempAndHumid();
  }
  // Blynk.run();
  // timer.run();
}

void sendTempAndHumid()
{
  float h = dht.readHumidity();
  float t = dht.readTemperature(); 
  Serial.println("Humidity and temperature\n\n");
  Serial.print("Current humidity = ");
  Serial.print(h);
  Serial.print("%  ");
  Serial.print("temperature = ");
  Serial.print(t); 
  // Blynk.virtualWrite(V0, t);
  // Blynk.virtualWrite(V1, h); 
}