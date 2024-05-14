/*
 * With this library an ESP8266 can ping a remote machine and know if it's reachable. 
 * It provides some basic measurements on ping messages (avg response time).
 */
static const BaseType_t app_cpu = 0;

#include <WiFi.h>
#include <ESP32Ping.h>

const char* ssid     = "Sussy_hotspot";
const char* password = "nguyenqu@ng10";

const IPAddress remote_ip(192, 168, 0, 1);

void taskFoo(void *parameter)
{
  while (1)
  {
    Serial.println("Text...");
    vTaskDelay(500/portTICK_PERIOD_MS);
  }
}

void taskPing(void *parameter)
{
  while (1)
  {
    if(Ping.ping("www.google.com")) {
      Serial.println("Success!!");
    } else {
      Serial.println("Error :(");
    }
    vTaskDelay(5000/portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);
  delay(10);

  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println("Connecting to WiFi");
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi connected with ip ");  
  Serial.println(WiFi.localIP());

  Serial.print("Pinging ip ");
  Serial.println(remote_ip);

    // Task to run forever
  xTaskCreatePinnedToCore(  //
                          taskPing,              // Function to be called
                          "Task ping",           // Name of task
                          2048,                   // Stack size (bytes in ESP32, word in FreeRTOS)
                          NULL,                   // parameter to pass function
                          2,                      // Task priority ( 0 to configMAX_PRIORITIES - 1)
                          NULL,                   // Task handle
                          app_cpu);               // Run on one core for demo purposes (ESP32 only)
  xTaskCreatePinnedToCore(  //
                          taskFoo,              // Function to be called
                          "Task foo",           // Name of task
                          2048,                   // Stack size (bytes in ESP32, word in FreeRTOS)
                          NULL,                   // parameter to pass function
                          2,                      // Task priority ( 0 to configMAX_PRIORITIES - 1)
                          NULL,                   // Task handle
                          app_cpu);               // Run on one core for demo purposes (ESP32 only)

}

void loop() 
{ 
}
