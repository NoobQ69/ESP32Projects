#include <WiFi.h>
#include <NTPtimeESP.h>
#include <WebServer.h>
#include <LiquidCrystal_I2C.h>

#define RELAY1_PIN 32
#define RELAY2_PIN 33
#define RELAY3_PIN 25
#define RELAY4_PIN 26
#define LDR_PIN 35

TaskHandle_t Task1;

enum DaysOfWeek {
  MODAY, 
  TUESDAY, 
  WEDNESDAY,
  THURSDAY,
  FRIDAY, 
  SATURDAY,
  SUNDAY
};

enum typeInterval
{
  FROM,
  TO
};

enum relayState
{
  OFF,
  ON
};

typedef struct {
  char operateMode;
  char setupTimeMode;
} MODE;

typedef struct {
  char relay1;
  char relay2;
  char relay3;
  char relay4;
  MODE mode;
} RelayState;

RelayState state1;

NTPtime NTPch("ch.pool.ntp.org");  //kết nối đến Server NTP
int Hour;                          // biến giờ
int mi;                           // biến phút
int sen;                           // biến giây
int Week;

int SetHourFrom = 17, SetMinuteFrom = 0, SetHourTo = 6, SetMinuteTo = 30, SetSecond = 0;

char SetDateOfWeekLight1[8][4] = {
  {18, 0, 6, 0},
  {18, 0, 6, 0},
  {18, 0, 6, 0},
  {18, 0, 6, 0},
  {18, 0, 6, 0},
  {18, 0, 6, 0},
  {18, 0, 6, 0},
  {18, 0, 6, 0}
};
char SetDateOfWeekLight2[8][4] = {
  {18, 0, 6, 0},
  {18, 0, 6, 0},
  {18, 0, 6, 0},
  {18, 0, 6, 0},
  {18, 0, 6, 0},
  {18, 0, 6, 0},
  {18, 0, 6, 0},
  {18, 0, 6, 0}
};
char SetDateOfWeekLight3[8][4] = {
  {18, 0, 6, 0},
  {18, 0, 6, 0},
  {18, 0, 6, 0},
  {18, 0, 6, 0},
  {18, 0, 6, 0},
  {18, 0, 6, 0},
  {18, 0, 6, 0},
  {18, 0, 6, 0}
};
char SetDateOfWeekLight4[8][4] = {
  {18, 0, 6, 0},
  {18, 0, 6, 0},
  {18, 0, 6, 0},
  {18, 0, 6, 0},
  {18, 0, 6, 0},
  {18, 0, 6, 0},
  {18, 0, 6, 0},
  {18, 0, 6, 0}
};

strDateTime dateTime;

String ClientRequest, myresultat, DataBuffer;
WiFiServer server(80);
WiFiClient client;

// Set your Static IP address
IPAddress local_IP(192, 168, 60, 3);
// Set your Gateway IP address
IPAddress gateway(192, 168, 1, 1);

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  ClientRequest = "";

  // initialize LCD
  lcd.begin();
  // turn on LCD backlight
  lcd.backlight();

  Serial.begin(9600);

  WiFi.disconnect();
  delay(3000);
  Serial.println("START");
  WiFi.begin("Sussy", "nguyenqu@ng10");
  while ((!(WiFi.status() == WL_CONNECTED))) {
    delay(300);
    Serial.print("..");
  }
  Serial.println("Connected");
  Serial.println("Your IP is");
  Serial.println((WiFi.localIP()));

  server.begin();
  //  set cursor to first column, first row
  lcd.setCursor(0, 0);
  lcd.print("IP:");
  lcd.print(WiFi.localIP());
  // print message
  Serial.print("IP:");
  Serial.println(WiFi.localIP());
  delay(500);

  state1.relay1 = 0;
  state1.relay2 = 0;
  state1.relay3 = 0;
  state1.relay4 = 0;
  state1.mode.operateMode = 0; // 0: manual, 1: specific mode
  state1.mode.setupTimeMode = 1; // 0: general, 1 specific mode
  
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);
  pinMode(RELAY4_PIN, OUTPUT);
  pinMode(LDR_PIN, INPUT);
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);
  digitalWrite(RELAY3_PIN, LOW);
  digitalWrite(RELAY4_PIN, LOW);

    // Task to run forever
  xTaskCreatePinnedToCore(  //
                          updateTime,              // Function to be called
                          "Task 1",           // Name of task
                          4096,                   // Stack size (bytes in ESP32, word in FreeRTOS)
                          NULL,                   // parameter to pass function
                          1,                      // Task priority ( 0 to configMAX_PRIORITIES - 1)
                          &Task1,                   // Task handle
                          0);               // Run on one core for demo purposes (ESP32 only)
}

void updateTime(void *parameter)
{
  char getIndexTime;
  while (1)
  {
    dateTime = NTPch.getNTPtime(7.0, 0);  // set múi giờ việt nam thứ 7
    getTime();                            // hàm để in thời gian ra

    if (state1.mode.setupTimeMode == 1)
    {
      getIndexTime = Week-2;
    }
    else
    {
      getIndexTime = 7;
    }

    if (state1.mode.operateMode == 1)
    {
      // CHECK RELAY 1 TIME
      if (compareTime(Hour, mi, SetDateOfWeekLight1[getIndexTime][0], SetDateOfWeekLight1[getIndexTime][1], FROM)) {
        state1.relay1 = ON;
        handleLightState(RELAY1_PIN, ON, "Den 1 bat");
      }
      else if (compareTime(Hour, mi, SetDateOfWeekLight1[getIndexTime][2], SetDateOfWeekLight1[getIndexTime][3], TO)) {
        state1.relay1 = OFF;
        handleLightState(RELAY1_PIN, OFF, "Den 1 tat");
      }
      // CHECK RELAY 2 TIME
      if (compareTime(Hour, mi, SetDateOfWeekLight2[getIndexTime][0], SetDateOfWeekLight2[getIndexTime][1], FROM)) {
        state1.relay2 = ON;
        handleLightState(RELAY2_PIN, ON, "Den 2 bat");
      }
      else if (compareTime(Hour, mi, SetDateOfWeekLight2[getIndexTime][2], SetDateOfWeekLight2[getIndexTime][3], TO)) {
        state1.relay2 = OFF;
        handleLightState(RELAY2_PIN, OFF, "Den 2 tat");
      }
      // CHECK RELAY 3 TIME
      if (compareTime(Hour, mi, SetDateOfWeekLight3[getIndexTime][0], SetDateOfWeekLight3[getIndexTime][1], FROM)) {
        state1.relay3 = ON;
        handleLightState(RELAY3_PIN, ON, "Den 3 bat");
      }
      else if (compareTime(Hour, mi, SetDateOfWeekLight3[getIndexTime][2], SetDateOfWeekLight3[getIndexTime][3], TO)) {
        state1.relay3 = OFF;
        handleLightState(RELAY3_PIN, OFF, "Den 3 tat");
      }
      // CHECK RELAY 4 TIME
      if (compareTime(Hour, mi, SetDateOfWeekLight4[getIndexTime][0], SetDateOfWeekLight4[getIndexTime][1], FROM)) {
        state1.relay4 = ON;
        handleLightState(RELAY4_PIN, ON, "Den 4 bat");
      }
      else if (compareTime(Hour, mi, SetDateOfWeekLight4[getIndexTime][2], SetDateOfWeekLight4[getIndexTime][3], TO)) {
        state1.relay4 = OFF;
        handleLightState(RELAY4_PIN, OFF, "Den 4 tat");
      }
    }
    Serial.print("Sensor:");
    Serial.println(analogRead(LDR_PIN));
    if (analogRead(LDR_PIN) > 3000)
    {
      state1.relay1 = 1;
      state1.relay2 = 1;
      state1.relay3 = 1;
      state1.relay4 = 1;
      handleLightState(RELAY1_PIN, ON, "Den 1 bat");
      handleLightState(RELAY2_PIN, ON, "Den 2 bat");
      handleLightState(RELAY3_PIN, ON, "Den 3 bat");
      handleLightState(RELAY4_PIN, ON, "Den 4 bat");
    }

    vTaskDelay(2000/portTICK_PERIOD_MS);
  }
}

void loop() {
  // readButtons();
  reconnectWiFi();
  client = server.available();
  if (!client) { return; }
  while (!client.available()) { delay(1); }
  ClientRequest = (ReadIncomingRequest());
  Serial.println(ClientRequest);
  ClientRequest.remove(0, 5);
  ClientRequest.remove(ClientRequest.length() - 9, 9);

  Serial.println("Get:");
  Serial.println(ClientRequest);
  
  if (ClientRequest == "connect") {
    ClientRequest = "connected";
    Serial.println("Connected to a client");
  } 
  else if (ClientRequest.startsWith("getState")) {
    ClientRequest = getState();
    Serial.println("Send state to client");
  }
  else if (ClientRequest.startsWith("getR1State")) {
    ClientRequest = getSetupTimeMode(1, SetDateOfWeekLight1);
    Serial.println("Send state R1 to client");
  }
  else if (ClientRequest.startsWith("getR2State")) {
    ClientRequest = getSetupTimeMode(2, SetDateOfWeekLight2);
    Serial.println("Send state R2 to client");
  }
  else if (ClientRequest.startsWith("getR3State")) {
    ClientRequest = getSetupTimeMode(3, SetDateOfWeekLight3);
    Serial.println("Send state R3 to client");
  }
  else if (ClientRequest.startsWith("getR4State")) {
    ClientRequest = getSetupTimeMode(4, SetDateOfWeekLight4);
    Serial.println("Send state R4 to client");
  }
  else if (ClientRequest.startsWith("SMODE")) {
    state1.mode.setupTimeMode = 1;
    Serial.println("Specific mode on");
  }
  else if (ClientRequest.startsWith("GMODE")) {
    state1.mode.setupTimeMode = 0;
    Serial.println("General mode on");
  }
  else if (ClientRequest == "r1on")  // LIGHT 1
  {
    state1.relay1 = 1;
    Serial.println("Light 1 on");
    handleLightState(RELAY1_PIN, 1, "Den 1 bat");
  } 
  else if (ClientRequest == "r1off") {
    state1.relay1 = 0;
    Serial.println("Light 1 off");
    handleLightState(RELAY1_PIN, 0, "Den 1 bat");
  } 
  else if (ClientRequest == "r2on")  // LIGHT 2
  {
    state1.relay2 = 1;
    handleLightState(RELAY2_PIN, 1, "Den 2 bat");
    Serial.println("Light 2 on");
    // ringBell(RELAY2_PIN, "Chuong 3 bat");
  } 
  else if (ClientRequest == "r2off") {
    state1.relay2 = 0;
    handleLightState(RELAY2_PIN, 0, "Den 2 tat");
    Serial.println("Light 2 off");
  } 
  else if (ClientRequest == "r3on")  // LIGHT 2
  {
    state1.relay3 = 1;
    handleLightState(RELAY3_PIN, 1, "Den 3 bat");
    Serial.println("Light 3 on");
  } 
  else if (ClientRequest == "r3off") {
    state1.relay3 = 0;
    handleLightState(RELAY3_PIN, 0, "Den 3 tat");
    Serial.println("Light 3 off");
  } 
  else if (ClientRequest == "r4on")  // LIGHT 2
  {
    state1.relay4 = 1;
    handleLightState(RELAY4_PIN, 1, "Den 4 bat");
    Serial.println("Light 4 on");
  } 
  else if (ClientRequest == "r4off") {
    state1.relay4 = 0;
    handleLightState(RELAY4_PIN, 0, "Den 4 tat");
    Serial.println("Light 4 off");
  } 
  else if (ClientRequest == "MM") {
    state1.mode.operateMode = 0;
    Serial.println("Manual mode activated");
  } 
  else if (ClientRequest == "AM") {
    state1.mode.operateMode = 1;
    Serial.println("Auto mode activated");
  }
  else if (ClientRequest.startsWith("ST")) {
    handleRequestData(ClientRequest);
    Serial.println("Received time data");
    ClientRequest = "SetTimeComplete";
  }

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println("");
  client.print(ClientRequest);
  client.stop();
  delay(1);
  client.flush();
}

// nếu wifi không kết nối thì kết nối lại
void reconnectWiFi() {
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi Disconnected!");
    Serial.println("Reconnecting WiFi");
    Serial.print(".");
    WiFi.disconnect();
    WiFi.reconnect();
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
}

void getTime() {
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

String ReadIncomingRequest() {
  while (client.available()) {
    ClientRequest = (client.readStringUntil('\r'));

    if ((ClientRequest.indexOf("HTTP/1.1") > 0) && (ClientRequest.indexOf("/favicon.ico") < 0)) {
      myresultat = ClientRequest;
    }
  }
  return myresultat;
}

void serialFlush() {
  while (Serial.available() > 0) {
    char t = Serial.read();
  }
}

// FOR HANDLING DATA 

int compareTime(char timebaseHour, char timebaseMinute, char timeToCompareHour, char timeToCompareMinute, int type)
{
  if (type == FROM)
  {
    if (timebaseHour == timeToCompareHour && timebaseMinute == timeToCompareMinute)
    {
      return 1;
    }
  }
  else
  {
    if (timebaseHour == timeToCompareHour && timebaseMinute == timeToCompareMinute)
    {
      return 1;
    }
  }
  return false;
}

String getState()
{
  String getCurrentState = "getState";
  if (state1.mode.operateMode == 1)
    getCurrentState += "AM";
  else
    getCurrentState += "MM";  

  if (state1.relay1 == 1)
    getCurrentState += "r1on";
  else
    getCurrentState += "r1off";

  if (state1.relay2 == 1)
    getCurrentState += "r2on";
  else
    getCurrentState += "r2off";

  if (state1.relay3 == 1)
    getCurrentState += "r3on";
  else
    getCurrentState += "r3off";

  if (state1.relay4 == 1)
    getCurrentState += "r4on";
  else
    getCurrentState += "r4off";

  Serial.println(getCurrentState);
  return getCurrentState;
}

void handleLightState(unsigned char relay, char state, char* msg) {
  Serial.println(msg);
  if (state == 1) {
    digitalWrite(relay, HIGH);
  } else {
    digitalWrite(relay, LOW);
  }
}

String getSetupTimeMode(char relay, char arr[8][4])
{
  String setupTimeData = "";
  if (state1.mode.setupTimeMode == 0)
  {
    setupTimeData += "ST0R";
  }
  else if (state1.mode.setupTimeMode == 1)
  {
    setupTimeData += "ST1R";
  }
  setupTimeData += String(int(relay));
  
  setupTimeData += "?";

  for (int i = 0; i < 7; i++)
  {
    if (i < 6)
      setupTimeData += "T" + String(i+2) + "=";
    else
      setupTimeData += "CN=";

    if (state1.mode.setupTimeMode == 0)
    {
      setupTimeData += String(int(arr[7][0]));
      setupTimeData += ":";
      setupTimeData += String(int(arr[7][1]));
      setupTimeData += "-";
      setupTimeData += String(int(arr[7][2]));
      setupTimeData += ":";
      setupTimeData += String(int(arr[7][3]));
    }
    else
    {
      setupTimeData += String(int(arr[i][0]));
      setupTimeData += ":";
      setupTimeData += String(int(arr[i][1]));
      setupTimeData += "-";
      setupTimeData += String(int(arr[i][2]));
      setupTimeData += ":";
      setupTimeData += String(int(arr[i][3]));
    }
    if (i < 6)
      setupTimeData += "&";
  }
  Serial.println(setupTimeData);
  return setupTimeData;
}

void handleRequestData(String receivedData)
{
  if (receivedData.startsWith("ST0"))
  {
    state1.mode.setupTimeMode = 0;
  }
  else if (receivedData.startsWith("ST1"))
  {
    state1.mode.setupTimeMode = 1;
  }

  receivedData = receivedData.substring(3);

  if (receivedData.startsWith("R1"))
  {
    storeTimeSetup(SetDateOfWeekLight1, receivedData.substring(3));
  }
  else if (receivedData.startsWith("R2"))
  {
    storeTimeSetup(SetDateOfWeekLight2, receivedData.substring(3));
  }
  else if (receivedData.startsWith("R3"))
  {
    storeTimeSetup(SetDateOfWeekLight3, receivedData.substring(3));
  }
  else if (receivedData.startsWith("R4"))
  {
    storeTimeSetup(SetDateOfWeekLight4, receivedData.substring(3));
  }
}

void storeTimeSetup(char arr[8][4], String value)
{
  if (value.startsWith("ALL"))
  {
    String listTimeValue[2];
    splitString(listTimeValue, value.substring(4), "-");
    String temp[2];
    splitString(temp, listTimeValue[0], ":");
    arr[7][0] = temp[0].toInt();
    arr[7][1] = temp[1].toInt();
    splitString(temp, listTimeValue[1], ":");
    arr[7][2] = temp[0].toInt();
    arr[7][3] = temp[1].toInt();
  }
  else
  {
    String listTime[7];
    splitString(listTime, value, "&");
    for (int i = 0; i < 7; i++)
    {
      String listTimeValue[2];
      splitString(listTimeValue, listTime[i].substring(3), "-");
      String temp[2];
      splitString(temp, listTimeValue[0], ":");
      arr[i][0] = temp[0].toInt();
      arr[i][1] = temp[1].toInt();
      splitString(temp, listTimeValue[1], ":");
      arr[i][2] = temp[0].toInt();
      arr[i][3] = temp[1].toInt();
    }
  }
  for (int i = 0; i < 7; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      Serial.print(int(arr[i][j]));
      Serial.print(" ");
    }
      Serial.println();
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
