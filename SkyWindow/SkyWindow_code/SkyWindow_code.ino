#include <WiFi.h>          //library for using ESP8266 WiFi
#include <WiFiClient.h>          //library for using ESP8266 WiFi
#include <PubSubClient.h>  //library for MQTT
#include <NTPtimeESP.h>
// #include <ArduinoJson.h>   //library for Parsing JSON

#define LED D2
#define LDR_PIN_1 34
#define LDR_PIN_2 35
#define LDR_PIN_3 32
#define LDR_PIN_4 33
#define RAIN_SENSOR 36
#define HALL_SENSOR 15

#define EN_PIN 5
#define STEP_PIN 16
#define DIR_PIN 17

TaskHandle_t Task1;

NTPtime NTPch("ch.pool.ntp.org");  //kết nối đến Server NTP
int Hour;                          // biến giờ
int mi;                           // biến phút
int sen;                           // biến giây
int Week;

strDateTime dateTime;

uint32_t delayMS;
///MQTT Credentials
//WiFi setup
const char* ssid = "Sussy";                  //setting your ap ssid
const char* password = "nguyenqu@ng10";  //setting your ap psk
//Mqtt broker connection setup
const char* mqttServer = "mqtt-dashboard.com";  //MQTT URL
const char* mqttUserName = "mqttusername1";    // MQTT username
const char* mqttPwd = "";          // MQTT password
const char* clientID = "Wuang6910012002";          // client id username+0001
const char* topic = "GetSunroofState610019";           //publish topic
//parameters for using non-blocking delay
unsigned long previousMillis = 0;
const long interval = 2000;
// Get from broker and send to broker buffer variable
String Data = "";
String msgStr = "";  // MQTT message buffer

char i = 0;
int angleStep = 0;
//setting up wifi and mqtt client
// WiFiServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);

enum booleanType
{
  NO = 0,
  YES = 1 
};

enum windowState
{
  OPEN, 
  CLOSE,
  AUTO_MODE,
  MANUAL_MODE
};

enum weather {
  SUNNY,
  CLOUDY,
  RAIN
};

enum stepperMotorUtilities
{
  CLOCKWISE = LOW,
  COUNTERCLOCKWISE = HIGH,
  MAX_ANGLE = 200,
  MIN_ANGLE = 0
};

enum {
  FROM,
  TO
};

typedef struct {
  int hour;
  int minute;
  int second;
} WINDOW_TIME;

typedef struct {
  char isEnforced;
  char windowState;
  char weatherState;
  char isStateChange;
  char inMode;
  WINDOW_TIME timeOpen;
  WINDOW_TIME timeClose;
} SKYWINDOW;

SKYWINDOW window1;
// for NTC time
void updateTime(void *parameter)
{
  char getIndexTime;
  while (1)
  {
    getTime();                            // hàm để in thời gian ra
    vTaskDelay(2000/portTICK_PERIOD_MS);
  }
}

void getTime() {
  dateTime = NTPch.getNTPtime(7.0, 0);  // set múi giờ việt nam thứ 7

  if (dateTime.valid) {
    // NTPch.printDateTime(dateTime); // in ra ngày giờ tháng năm
    Hour = dateTime.hour;    // Gio
    mi = dateTime.minute;    // Phut
    sen = dateTime.second;   // Giay

    Week = dateTime.dayofWeek;

    if (sen == 0)
    {
      Serial.print("Thoi gian: ");
      Serial.print(Hour);
      Serial.print(" h: ");
      Serial.print(mi);
      Serial.print(" m: ");
      Serial.print(sen);
      Serial.print(" s    ");

      Serial.print("Thu ");
      Serial.print(Week);
    }
  }
}

int compareTime(int timebaseHour, int timebaseMinute, int timeToCompareHour, int timeToCompareMinute, int type)
{
  if (type == FROM)
  {
    if (timebaseHour == timeToCompareHour && timebaseMinute == timeToCompareMinute)
    {
      return 1;
    }
  }
  else if (type == TO)
  {
    if (timebaseHour == timeToCompareHour && timebaseMinute == timeToCompareMinute)
    {
      return 1;
    }
  }
  return 0;
}
// FOR MQTT 
void setup_wifi() {
  delay(10);
  // WiFi.mode(WIFI_STA);
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
    client.setServer(mqttServer, 1883);  //setting MQTT server
    if (client.connect(clientID, mqttUserName, mqttPwd)) {
      digitalWrite(LED_BUILTIN, HIGH);
      // Serial.println("MQTT connected");
      client.subscribe("GetState");
      client.subscribe("DoorState");
      client.subscribe("SetTime");
      // Serial.println("Topic Subscribed");
    } else {
      digitalWrite(LED_BUILTIN, LOW);
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 3 seconds");
      delay(3000);  // wait 5sec and retry
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) { //subscribe call back
  String getTopic(topic);
  Serial.print("Message arrived in topic: ");
  Serial.println(getTopic);
  Serial.print("Message:");
  Data = "";
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    Data += (char)payload[i];
  }
  Serial.println();
  Serial.print("Message size :");
  Serial.println(length);
  Serial.println();
  Serial.println("-----------------------");
  Serial.println(Data);
  handleReceivedMessage(getTopic, Data);
}

void publishData(String data) {
  byte arrSize = data.length() + 1;
  char msg[arrSize];
  Serial.print("PUBLISH DATA:");
  Serial.println(data);
  data.toCharArray(msg, arrSize);
  client.publish(topic, msg);
  delay(50);
}

int handleReceivedMessage(String topic, String payload)
{
  if (topic.startsWith("DoorState"))
  {
    if (payload.startsWith("Close"))
    {
      window1.windowState = CLOSE;
      CloseWindow();
    }
    else if (payload.startsWith("Open"))
    {
      window1.windowState = OPEN;
      OpenWindow();
    }
    else
    {
      return -1;
    }
  }
  else if (topic.startsWith("SetTime"))
  {
    handleSetTimeData(payload);
  }
  else if (topic.startsWith("GetState"))
  {
    handleSendingMessage(payload);
  }
  return 0;
}

void handleSendingMessage(String message)
{
  if (message.startsWith("AM"))
  {
    window1.inMode = AUTO_MODE;
    publishData("AM");
  }
  else if (message.startsWith("MM"))
  {
    window1.inMode = MANUAL_MODE;
    publishData("MM");
  }
  else
    publishData(getCurrentStateToCLient());
}

void handleSetTimeData(String data)
{
  String getTime[2];
  splitString(getTime, data.substring(3), "&");
  
  String temp[2];
  splitString(temp, getTime[0], "-");
  window1.timeOpen.hour = temp[0].toInt();
  window1.timeOpen.minute = temp[1].toInt();

  Serial.print("Get to time:");
  Serial.print(getTime[1]);
  splitString(temp, getTime[1], "-");
  window1.timeClose.hour = temp[0].toInt();
  window1.timeClose.minute = temp[1].toInt();
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

String getCurrentStateToCLient()
{
  String currentState = "State?";
  if (window1.windowState == OPEN)
    currentState += "DoorOpen";
  else
    currentState += "DoorClose";
  
  currentState += ",";

  if (window1.weatherState == SUNNY)
    currentState += "WeatherSunny";
  else if (window1.weatherState == CLOUDY)
    currentState += "WeatherCloudy";
  else if (window1.weatherState == RAIN)
    currentState += "WeatherRain";

  currentState += ",";

  if (window1.inMode == AUTO_MODE)
    currentState += "AM";
  else if (window1.inMode == MANUAL_MODE)
    currentState += "MM";
  
  currentState += ",";

  currentState += "ST=";
  currentState += String(window1.timeOpen.hour);
  currentState += "-";
  currentState += String(window1.timeOpen.minute);
  currentState += "&";
  currentState += String(window1.timeClose.hour);
  currentState += "-";
  currentState += String(window1.timeClose.minute);
  return currentState;
}

// FOR LDR SENSORS
int ReadRainSensor()
{
  unsigned int readRainSensor = analogRead(RAIN_SENSOR);
  if (readRainSensor < 2300)
  {
    window1.weatherState = RAIN;
  }
  return readRainSensor;
}

// FOR LDR SENSORS
void ReadLDR()
{
  unsigned int readLDR1 = analogRead(LDR_PIN_1);
  unsigned int readLDR2 = analogRead(LDR_PIN_2);
  unsigned int readLDR3 = analogRead(LDR_PIN_3);
  unsigned int readLDR4 = analogRead(LDR_PIN_4);

  if (readLDR1 > 3500 || readLDR2 > 3500 || readLDR3 > 3500 || readLDR4 > 3500)
  {
    if (ReadRainSensor() > 2500)
      window1.weatherState = SUNNY;
  }
  else if (readLDR1 < 1500 || readLDR2 < 1500 || readLDR3 < 1500 || readLDR4 < 1500)
    if (ReadRainSensor() > 2500)
      window1.weatherState = CLOUDY;

  if (readLDR1 > 3800 && readLDR2 > 3800 && readLDR3 > 3800 && readLDR4 > 3800)
  {
    if (window1.inMode == AUTO_MODE)
      window1.windowState = CLOSE;
  }
  else if (readLDR2 > 3800)
  {
    if (window1.inMode == AUTO_MODE)
      window1.windowState = CLOSE;
  }
}

// FOR STEPPER MOTOR

void RunOneStep(int speed = 800) {
  digitalWrite(STEP_PIN, HIGH);
  delay(speed);
  digitalWrite(STEP_PIN, LOW);
  delay(speed);
}

void RunStepperMotor(int step = 200, int direction = CLOCKWISE) {
  if (direction == CLOCKWISE) {
    digitalWrite(DIR_PIN, CLOCKWISE);
  }
  else {
    digitalWrite(DIR_PIN, COUNTERCLOCKWISE);
  }
  digitalWrite(EN_PIN, LOW);
  for (int i = 0; i < step; i++) {
    if (direction == CLOCKWISE) {
      if (angleStep >= MIN_ANGLE) {
        angleStep--;
        RunOneStep(20);
      }
    }
    else if (direction == COUNTERCLOCKWISE) {
      if (angleStep < MAX_ANGLE) {
        angleStep++;
        RunOneStep(20);
      }
    }
  }
  digitalWrite(EN_PIN, HIGH);
}

void OpenWindow()
{
  Serial.println("Open window");
  RunStepperMotor(MAX_ANGLE, COUNTERCLOCKWISE);
}

void CloseWindow()
{
  Serial.println("Close window");
  RunStepperMotor(MAX_ANGLE, CLOCKWISE);
}

// MAIN 
void setup() {
  Serial.begin(9600);
  setup_wifi();

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);

  pinMode(LDR_PIN_1, INPUT);
  pinMode(LDR_PIN_2, INPUT);
  pinMode(LDR_PIN_3, INPUT);
  pinMode(LDR_PIN_4, INPUT);

  pinMode(RAIN_SENSOR, INPUT);
  pinMode(HALL_SENSOR, INPUT);


  window1.isEnforced = YES;
  window1.windowState = CLOSE;
  window1.weatherState = SUNNY;
  // window1.isStateChange = YES;
  window1.inMode = MANUAL_MODE;

  window1.timeOpen.hour = 6;
  window1.timeOpen.minute = 0;
  window1.timeOpen.second = 0;
  
  window1.timeClose.hour = 18;
  window1.timeClose.minute = 0;
  window1.timeClose.second = 0;
  
  digitalWrite(DIR_PIN, LOW);
  xTaskCreatePinnedToCore(  //
                        updateTime,              // Function to be called
                        "Task 1",           // Name of task
                        4096,                   // Stack size (bytes in ESP32, word in FreeRTOS)
                        NULL,                   // parameter to pass function
                        1,                      // Task priority ( 0 to configMAX_PRIORITIES - 1)
                        &Task1,                   // Task handle
                        0);               // Run on one core for demo purposes (ESP32 only)
  digitalWrite(EN_PIN, LOW);
  while (digitalRead(HALL_SENSOR) == 0)
  {
    RunOneStep(15);
  }
  digitalWrite(EN_PIN, HIGH);
  client.setServer(mqttServer, 1883);  //setting MQTT server
  client.setCallback(callback);        //defining function which will be called when message is received.
  // RunStepperMotor(MAX_ANGLE, COUNTERCLOCKWISE);
  // RunStepperMotor(MAX_ANGLE, CLOCKWISE);
}

void printValue()
{
  Serial.print("LDR1: ");
  Serial.println(analogRead(LDR_PIN_1));
  Serial.print("LDR2: ");
  Serial.println(analogRead(LDR_PIN_2));
  Serial.print("LDR3: ");
  Serial.println(analogRead(LDR_PIN_3));
  Serial.print("LDR4: ");
  Serial.println(analogRead(LDR_PIN_4));
  Serial.print("RAIN SENSOR:");
  Serial.println(analogRead(RAIN_SENSOR));
  Serial.print("HALL SENSOR:");
  Serial.println(digitalRead(HALL_SENSOR));
}

void loop() {
  if (!client.connected()) {  // if client is not connected
    reconnect();              // try to reconnect
  }
  client.loop();
  unsigned long currentMillis = millis();          // read current time
  if (currentMillis - previousMillis >= 5000)  // if current time - last time > 2 sec
  {
    previousMillis = currentMillis;
    ReadRainSensor();
    ReadLDR();

    printValue();
    if (window1.weatherState == SUNNY) {
      msgStr = "WeatherSunny";  // String(temp) +","+String(hum);
      Serial.println("Weather sunny");
    } 
    else if (window1.weatherState == RAIN) {
      msgStr = "WeatherRain";  // String(temp) +","+String(hum);
      window1.windowState = CLOSE;
      Serial.println("Weather rain");
    } 
    else if (window1.weatherState == CLOUDY) {
      msgStr = "WeatherCloudy";  // String(temp) +","+String(hum); 
      Serial.println("Weather cloudy");
    }
    publishData(msgStr);

    if (window1.inMode == AUTO_MODE)
    {
      if (window1.weatherState == RAIN) {
        window1.windowState = CLOSE;
      }
      // if (window1.weatherState == SUNNY || window1.weatherState == CLOUDY) {
      //   window1.windowState = OPEN;
      // } 

      if (window1.windowState == OPEN) {
        msgStr = "DoorOpen";  // String(temp) +","+String(hum);
      }
      if (window1.windowState == CLOSE) {
        msgStr = "DoorClose";  // String(temp) +","+String(hum);
      } 
      publishData(msgStr);
      
      if (compareTime(Hour, mi, window1.timeOpen.hour, window1.timeOpen.minute, FROM) == 1)
      {
        Serial.println("Door open");
        window1.windowState = OPEN;
      }
      if (compareTime(Hour, mi, window1.timeClose.hour, window1.timeClose.minute, TO) == 1)
      {
        Serial.println("Door close");
        window1.windowState = CLOSE;
      }
    }

    if (window1.weatherState == RAIN) {
      window1.windowState = CLOSE;
    }

    if (window1.windowState == OPEN)
    {
      OpenWindow();
    }
    else if (window1.windowState == CLOSE)
    {
      CloseWindow();
    }
    Serial.println();
  }
}

