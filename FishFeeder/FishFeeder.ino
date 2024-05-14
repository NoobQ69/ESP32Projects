#include <BluetoothSerial.h>
#include <Stepper.h>
#include "DHT.h"

#define DHTPIN 13     // Digital pin connected to the DHT sensor
// Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
// Pin 15 can work but DHT must be disconnected during program upload.

#define DHTTYPE DHT11   // DHT 11

DHT dht(DHTPIN, DHTTYPE);

#define STEPPER_PIN_1 32
#define STEPPER_PIN_2 33
#define STEPPER_PIN_3 25
#define STEPPER_PIN_4 26

#define HALL_SENSOR   14
#define LED_INDICATOR 2
#define LED_FEEDTIMEON 15

int IsFeedReady = 0;
BluetoothSerial SerialBT;
String Dataln = "";
const int stepsPerRevolution = 512; 
Stepper myStepper = Stepper(stepsPerRevolution, STEPPER_PIN_1, STEPPER_PIN_3, STEPPER_PIN_2, STEPPER_PIN_4);

typedef struct
{
  int hour;
  int minute;
  int second;
} TIME_INTERVAL;

typedef struct {
  char isFeedTimeOn;
  int timeSet;
  int timeRemain;
} FEED_TIME;

typedef struct
{
  char state;                   // 0: stop, 1: start
  TIME_INTERVAL timeCurrent;
  TIME_INTERVAL timeSet;
  FEED_TIME feedingTime;
  char timeMode;                // 0: time point, 1: time interval
  char operateMode;             // 0: once, 1: repeat
} DATA_PACKAGE;

String CurrentState = "";
TIME_INTERVAL TimeBase;
DATA_PACKAGE DataClient;

unsigned long PreviousTime = 0, SubPreviousTime;

void callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  if (event == ESP_SPP_SRV_OPEN_EVT) {
    //Serial.println("A client is connected to ESP32");
    CurrentState = getCurrentState(DataClient);
    unsigned char getData[32];
    CurrentState.getBytes(getData, CurrentState.length());
    SerialBT.write(getData, CurrentState.length());
    //Serial.print("Send:");Serial.println(CurrentState);
    CurrentState.remove(0);
  }
}

void countTimeUp(int &hours, int &minutes, int &seconds) {
  seconds += 1;

  if (seconds > 59) {
    seconds = 0;
    minutes += 1;

    if (minutes > 59) {
      minutes = 0;
      hours += 1;
      if (hours > 23) {
        hours = 0;
      }
    }
  }
}

void countTimeDown(int &hours, int &minutes, int &seconds) 
{
  if (seconds < 1) 
  {
    seconds = 60;

    if (minutes > 0) {
      minutes -= 1;
    }
    else
    {
      if (hours > 0)
      {
        minutes = 59;
        hours -= 1;
      }
    }
  }
  seconds -= 1;
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

void getTimeFromData(String timeString)
{
  String listString[3];
  splitString(listString, timeString, "-");
  DataClient.timeSet.hour = listString[0].toInt();
  DataClient.timeSet.minute = listString[1].toInt();
  DataClient.timeSet.second = listString[2].toInt();
}

void handleTime(String command, String time)
{
  if (command.startsWith("TI")) {
    DataClient.timeMode = 1;
    getTimeFromData(time.substring(3));
    DataClient.timeCurrent.hour = DataClient.timeSet.hour;
    DataClient.timeCurrent.minute = DataClient.timeSet.minute;
    DataClient.timeCurrent.second = DataClient.timeSet.second;
    Serial.println("Time interval");
  }
  else if (command.startsWith("TP")) {
    DataClient.timeMode = 0;  
    getTimeFromData(time.substring(3)); 
    Serial.println("Time point");
  }
  Serial.print("hour: "); Serial.print(DataClient.timeSet.hour); 
  Serial.print(" minute: "); Serial.print(DataClient.timeSet.minute);
  Serial.print(" second: "); Serial.print(DataClient.timeSet.second);
}

void handleOperateMode(String mode)
{
  if (mode.startsWith("RP")) {
    DataClient.operateMode = 1;
    Serial.println("Repeat");
  } 
  else if (mode.startsWith("ONC")) {
    DataClient.operateMode = 0;
    Serial.println("once");
  }
}

void handleStateCommand(String command)
{
  String listString[3];
  splitString(listString, command, "&");
  handleTime(listString[0], listString[1]);
  handleOperateMode(listString[2]);
}

void setTimeBase(String timeBaseString)
{
  String listString[3];
  splitString(listString, timeBaseString, "-");
  TimeBase.hour = listString[0].toInt();
  TimeBase.minute = listString[1].toInt();
  TimeBase.second = listString[2].toInt();
}

String handleRespondCommands(String dataln)
{
  String respondToClient;
  if (dataln.startsWith("DHT"))
  {
    int h = dht.readHumidity();
    int t = dht.readTemperature();

    // Check if any reads failed and exit early (to try again).
    if (!isnan(h) || !isnan(t)) {
      respondToClient += "DHT?";
      respondToClient += "T=" + String(t) + "&";
      respondToClient += "H=" + String(h);
    }
    else
    {
      Serial.println(F("Failed to read from DHT sensor!"));
      return "NaN";
    }
  }
  else if (dataln.startsWith("GCT"))
  {
    respondToClient += "SC?ST=";
    respondToClient += String(DataClient.timeCurrent.hour)+"-"+ String(DataClient.timeCurrent.minute)+"-"+String(DataClient.timeCurrent.second);
  }
  return respondToClient;
}

void handleUnrespondCommands(String dataln) 
{
  Serial.print("Received:");
  Serial.println(dataln);
  Serial.println();
  if (dataln.startsWith("FN")) {
    DataClient.feedingTime.timeRemain = 1;
    DataClient.feedingTime.isFeedTimeOn = 1;
    handleFeedOn();
    Serial.println("Feed now");
  }
  if (dataln.startsWith("TB")) {
    String temp = dataln.substring(3);
    setTimeBase(temp.substring(2));
    Serial.println("Set time base");
  } 
  else if (dataln.startsWith("STT")) {
    DataClient.state = 1;
    Serial.println("Start");
    
    handleStateCommand(dataln.substring(4));
  } 
  else if (dataln.startsWith("STP")) {
    DataClient.state = 0;
    Serial.println("Stop");

    handleStateCommand(dataln.substring(4));
  }
  else if (dataln.startsWith("FT")) {
    String temp = dataln.substring(3);
    DataClient.feedingTime.timeSet = temp.toInt();
    DataClient.feedingTime.timeRemain = temp.toInt();
    Serial.print("Feeding time: ");
    Serial.println(DataClient.feedingTime.timeSet);
  }
  else if (dataln.startsWith("RP")) {
    DataClient.operateMode = 1;
    Serial.println("Repeat");
  } 
  else if (dataln.startsWith("ONC")) {
    DataClient.operateMode = 0;
    Serial.println("once");
  }
}

String getDatacommand()
{
  char c;
  String s;
  while (SerialBT.available())
  {
    c = SerialBT.read();
    if (c == '$')
    {
      int i = 0;
      String respondData = handleRespondCommands(s);
      while (i < respondData.length())
      {
        SerialBT.write(respondData.charAt(i));
        i++;
      }
      return "RsData";
    }
    else
    {
      s += c;
    }
  }
  return s;
}

String getCurrentState(DATA_PACKAGE data)
{
  String currentState;
  currentState += "CT?";
  
  if (data.state == 1)
  {
    currentState += "STT&";
  }
  else
  {
    currentState += "STP&";
  }

  if (data.timeMode == 1)
  {
    currentState += "TI&";
    currentState += "ST=";
    currentState += String(data.timeSet.hour)+"-"+ String(data.timeSet.minute)+"-0";
    currentState += "&";
  }
  else
  {
    currentState += "TP&";
    currentState += "ST=";
    currentState += String(data.timeSet.hour)+"-"+ String(data.timeSet.minute)+"-"+String(data.timeSet.second); 
    currentState += "&";
  }

  if (data.operateMode == 1)
  {
    currentState += "RP&";
  }
  else 
  {
    currentState += "ONC&";
  }

  currentState += "FT=";
  currentState += String(data.feedingTime.timeSet);
  currentState += '\n';

  return currentState;
}

void handleFeedOn()
{
  if (IsFeedReady == 0)
  {
    if (digitalRead(HALL_SENSOR) == 0)
    {
      myStepper.step(-64);
    }
    else
    {
      IsFeedReady = 1;
    }
  }
  
  if (IsFeedReady == 1) 
  {
    if (digitalRead(HALL_SENSOR) == 0)
    {
      myStepper.step(0);
      IsFeedReady = 0;
      
      DataClient.feedingTime.timeRemain--;
      if (DataClient.feedingTime.timeRemain == 0)
      {
        DataClient.feedingTime.timeRemain = DataClient.feedingTime.timeSet;
        DataClient.feedingTime.isFeedTimeOn = 0;
        digitalWrite(LED_FEEDTIMEON, LOW);
      }
    }
    else
    {
      myStepper.step(-64);
    }
  }
}

void printTimeBase()
{
  Serial.print("Time base:");
  Serial.print(TimeBase.hour);Serial.print(":");Serial.print(TimeBase.minute);Serial.print(":");Serial.print(TimeBase.second);
  Serial.println();
}

void setup() {
  Serial.begin(9600);
  pinMode(HALL_SENSOR, INPUT_PULLUP);
  pinMode(LED_INDICATOR, OUTPUT);
  pinMode(LED_FEEDTIMEON, OUTPUT);


  SerialBT.register_callback(callback);
  if (!SerialBT.begin("ESP32-Bluetooth")) {
    Serial.println("An error occurred initializing Bluetooth");
  } else {
    Serial.println("Bluetooth initialized");
  }
  TimeBase.hour = 0;
  TimeBase.minute = 0;
  TimeBase.second = 0;

  DataClient.state = 0;  // 0: stop, 1: start
  DataClient.timeSet.hour = 1;
  DataClient.timeSet.minute = 30;
  DataClient.timeSet.second = 0;
  DataClient.timeCurrent.hour = 1;
  DataClient.timeCurrent.minute = 30;
  DataClient.timeCurrent.second = 0;
  DataClient.feedingTime.isFeedTimeOn = 0;  
  DataClient.feedingTime.timeSet = 1;   
  DataClient.feedingTime.timeRemain = 1;   
  DataClient.timeMode = 1;     // 0: time point, 1: time interval
  DataClient.operateMode = 1;  // 0: once, 1: repeat

  dht.begin();
  
  myStepper.setSpeed(60);
  Serial.println("Turning feeder case");
  while (digitalRead(HALL_SENSOR) != 0)
  {
    Serial.println(".");
    myStepper.step(64);
    delay(50);
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  if (millis() - PreviousTime > 999) {
    PreviousTime = millis();
    digitalWrite(LED_INDICATOR, LOW);
    countTimeUp(TimeBase.hour, TimeBase.minute, TimeBase.second);
    printTimeBase();

    if (SerialBT.available() > 0)
    {
      digitalWrite(LED_INDICATOR, HIGH);
      String getData = getDatacommand();
      if (!getData.startsWith("RsData"))
      {
        handleUnrespondCommands(getData);
      }
    }

    if (DataClient.state == 1)
    {
      if (DataClient.timeMode == 1)
      {
        countTimeDown(DataClient.timeCurrent.hour, DataClient.timeCurrent.minute, DataClient.timeCurrent.second);
        if (DataClient.timeCurrent.hour==0 &&
            DataClient.timeCurrent.minute==0 &&
              DataClient.timeCurrent.second==0)
              {
                DataClient.feedingTime.isFeedTimeOn = 1;
                if (DataClient.operateMode == 1)
                {
                  DataClient.state = 1;
                  DataClient.timeCurrent.hour   = DataClient.timeSet.hour;
                  DataClient.timeCurrent.minute = DataClient.timeSet.minute;
                  DataClient.timeCurrent.second = DataClient.timeSet.second;
                }
                else
                {
                  DataClient.state = 0;
                }
              }
      }
      else
      {
        if (DataClient.timeSet.hour==TimeBase.hour &&
          DataClient.timeSet.minute==TimeBase.minute &&
            DataClient.timeSet.second==TimeBase.second)
            {
              DataClient.feedingTime.isFeedTimeOn = 1;
                if (DataClient.operateMode == 0)
                {
                  DataClient.state = 0;
                }
            }
      }
    }

  }
  if (millis() - SubPreviousTime > 100)
  {
    SubPreviousTime = millis();
    if (DataClient.feedingTime.isFeedTimeOn == 1)
    {
      digitalWrite(LED_FEEDTIMEON, HIGH);
      handleFeedOn();
    }
  }
}
