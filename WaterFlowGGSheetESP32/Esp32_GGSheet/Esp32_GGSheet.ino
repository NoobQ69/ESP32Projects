#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif

#define RXD2 16
#define TXD2 17

// Replace with your SSID and Password
const char* ssid     = "Sussy";
const char* password = "nguyenqu@ng10";

// Replace with your unique IFTTT URL resource
const char* resource = "/trigger/FlowSensor_value/with/key/cyLWyMDKg4u0zdCjNK7W3l";

// How your resource variable should look like, but with your own API KEY (that API KEY below is just an example):
//const char* resource = "/trigger/bme280_readings/with/key/nAZjOphL3d-ZO4N3k64-1A7gTlNSrxMJdmqy3";

// Maker Webhooks IFTTT
const char* server = "maker.ifttt.com";

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  delay(2000);
 
  initWiFi();
  // makeIFTTTRequest();
    
  // #ifdef ESP32
  //   // enable timer deep sleep
  //   esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);    
  //   Serial.println("Going to sleep now");
  //   // start deep sleep for 3600 seconds (60 minutes)
  //   esp_deep_sleep_start();
  // #else
  //   // Deep sleep mode for 3600 seconds (60 minutes)
  //   Serial.println("Going to sleep now");
  //   ESP.deepSleep(TIME_TO_SLEEP * uS_TO_S_FACTOR); 
  // #endif
}

void loop() {
  // sleeping so wont get here 
  if (Serial2.available() > 0)
  {
    char temp;
    String getValue = "";
    while (Serial2.available() > 0)
    {
      temp = Serial2.read();
      if (temp == '\n') break;
      getValue += temp;
    }
    Serial.println(getValue);
    makeIFTTTRequest(getValue);
  }
}

void splitString(String* arrayString, String text, String index)
{
  int i = 0, delimiter = -1, delimiterPrevious = -1;
  do
  {
    delimiter = text.indexOf(index, delimiter+1);
    if (delimiter != -1)
    {
      arrayString[i] = String(text.substring(delimiterPrevious+1, delimiter));
      i++;
      delimiterPrevious = delimiter;
    }
  }
  while (delimiter != -1);
  
  arrayString[i] = String(text.substring(delimiterPrevious+1));
}

// Establish a Wi-Fi connection with your router
void initWiFi() {
  Serial.print("Connecting to: "); 
  Serial.print(ssid);
  WiFi.begin(ssid, password);  

  int timeout = 10 * 4; // 10 seconds
  while(WiFi.status() != WL_CONNECTED  && (timeout-- > 0)) {
    delay(250);
    Serial.print(".");
  }
  Serial.println("");

  if(WiFi.status() != WL_CONNECTED) {
     Serial.println("Failed to connect, going back to sleep");
  }

  Serial.print("WiFi connected in: "); 
  Serial.println(millis());
  Serial.print("IP address: "); 
  Serial.println(WiFi.localIP());
}

// Make an HTTP request to the IFTTT web service
void makeIFTTTRequest(String flowSensorValue) {
  Serial.print("Connecting to "); 
  Serial.print(server);
  
  WiFiClient client;
  int retries = 5;
  while(!!!client.connect(server, 80) && (retries-- > 0)) {
    Serial.print(".");
  }
  Serial.println();
  if(!!!client.connected()) {
    Serial.println("Failed to connect...");
  }
  
  Serial.print("Request resource: "); 
  Serial.println(resource);

  String getData[3];
  splitString(getData, flowSensorValue, ",");
  // Temperature in Celsius
  String jsonObject = String("{\"value1\":\"") + getData[0] + "\",\"value2\":\"" + getData[1]
                      + "\",\"value3\":\"" + getData[2] + "\"}";
  // String jsonObject = String("{\"value1\":\"") + flowSensorValue;
                      
  // Comment the previous line and uncomment the next line to publish temperature readings in Fahrenheit                    
  /*String jsonObject = String("{\"value1\":\"") + (1.8 * bme.readTemperature() + 32) + "\",\"value2\":\"" 
                      + (bme.readPressure()/100.0F) + "\",\"value3\":\"" + bme.readHumidity() + "\"}";*/
                      
  client.println(String("POST ") + resource + " HTTP/1.1");
  client.println(String("Host: ") + server); 
  client.println("Connection: close\r\nContent-Type: application/json");
  client.print("Content-Length: ");
  client.println(jsonObject.length());
  client.println();
  client.println(jsonObject);
        
  int timeout = 5 * 10; // 5 seconds             
  while(!!!client.available() && (timeout-- > 0)){
    delay(100);
  }
  if(!!!client.available()) {
    Serial.println("No response...");
  }
  while(client.available()){
    Serial.write(client.read());
  }
  
  Serial.println("\nclosing connection");
  client.stop(); 
}