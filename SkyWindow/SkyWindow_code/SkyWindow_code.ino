#include <WiFi.h> //library for using ESP8266 WiFi 
#include <PubSubClient.h> //library for MQTT
#include <ArduinoJson.h> //library for Parsing JSON
#define LED D2

uint32_t delayMS;
///MQTT Credentials
//WiFi setup
const char* ssid = "Sussy";//setting your ap ssid
const char* password = "nguyenqu@ng#%$^&_";//setting your ap psk
//Mqtt broker connection setup
const char* mqttServer = "mqtt-dashboard.com"; //MQTT URL
const char* mqttUserName = "mqtt username1";  // MQTT username
const char* mqttPwd = "mqtt password";  // MQTT password
const char* clientID = "username0001"; // client id username+0001
const char* topic = "SkyWindowState"; //publish topic
//parameters for using non-blocking delay
unsigned long previousMillis = 0;
const long interval = 2000;
// Get from broker and send to broker buffer variable 
String Data = "";
String msgStr = "";      // MQTT message buffer
float temp, hum;
//setting up wifi and mqtt client
WiFiClient espClient;
PubSubClient client(espClient);

char i = 0;

void setup_wifi() {
  delay(10);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect(clientID, mqttUserName, mqttPwd)) {
      Serial.println("MQTT connected");
      client.subscribe("DoorState");
      Serial.println("Topic Subscribed");
    }
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 3 seconds");
      delay(3000);  // wait 5sec and retry
    }
  }
}
//subscribe call back
void callback(char*topic, byte* payload, unsigned int length)
{
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
    Data += (char)payload[i];
  }
  Serial.println();
  Serial.print("Message size :");
  Serial.println(length);
  Serial.println();
  Serial.println("-----------------------");
  Serial.println(Data);
}

void setup() 
{
  Serial.begin(115200);
  // Initialize device.
  //dht.begin();
  // get temperature sensor details.
  //sensor_t sensor;
  //dht.temperature().getSensor(&sensor);
  //dht.humidity().getSensor(&sensor);
  // pinMode(LED, OUTPUT);
  // digitalWrite(LED, LOW);
  setup_wifi();
  client.setServer(mqttServer, 1883); //setting MQTT server
  client.setCallback(callback); //defining function which will be called when message is received.
}

void loop() 
{
  if (!client.connected()) 
  { //if client is not connected
    reconnect(); //try to reconnect
  }
  client.loop();
  unsigned long currentMillis = millis(); //read current time
  if (currentMillis - previousMillis >= interval)// if current time - last time > 2 sec 
  {
    previousMillis = currentMillis;
    //read temp and humidity
    // sensors_event_t event;
    // dht.temperature().getEvent(&event);
    // if (isnan(event.temperature)) {
    //   Serial.println(F("Error reading temperature!"));
    // }
    Serial.print(F("Temperature: "));
    //temp = event.temperature;
    //Serial.print(temp);
    Serial.println(F("°C"));
    if (i == 0)
    {
      msgStr = "WeatherSunny";// String(temp) +","+String(hum);
    }
    else if (i == 1)
    {
      msgStr = "WeatherRain";// String(temp) +","+String(hum);
    }
    else if (i == 2)
    {
      msgStr = "WeatherCloudy";// String(temp) +","+String(hum);
      i = -1;
    }
    i++;
    byte arrSize = msgStr.length() + 1;
    char msg[arrSize];
    Serial.print("PUBLISH DATA:");
    Serial.println(msgStr);
    msgStr.toCharArray(msg, arrSize);
    client.publish(topic, msg);
    msgStr = "";
    delay(50);
  }
}