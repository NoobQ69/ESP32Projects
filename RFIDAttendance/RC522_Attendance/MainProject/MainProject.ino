#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <NTPtimeESP.h>
#include <RtcDS1302.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>  //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFiManager.h>  //https://github.com/tzapu/WiFiManager
#include <EEPROM.h>
#include "Queue.h"
// #define DEBUG_ON
#include "SD_Utilities.h"


#define QUEUE_LENGTH 50
#define EEPROM_SIZE 118

#define LED_WIFI 33
#define LED_CARD 32
#define BUZZER    4

#define AP__NAME "RFID-Attendance-Device"
#define AP__PASS "24112411"

#define SD_CS_PIN        15
#define SD_CLK_PIN       14
#define SD_MOSI_PIN      13
#define SD_MISO_PIN      21
SPIClass sdSPI2(HSPI);
String DataFileName = "/data.txt";
String ExceptionalFileName = "/exception.txt";
int NumberOfLines = 0;

#define DS1302_CLK 27
#define DS1302_DAT 26
#define DS1302_RST 25

#define SS_PIN 16
#define RST_PIN 17

#define RESET_WIFI_BTN  34
#define SETCARD_BTN     35
#define SETTIMEOUT_BTN   5
#define countof(a) (sizeof(a) / sizeof(a[0]))

String Web_App_URL = "";

static const BaseType_t app_cpu = 0;
static SemaphoreHandle_t mutex;
Queue<String> QueueString = Queue<String>(10);
// Task handler
static TaskHandle_t task_1_handler = NULL;
static TaskHandle_t task_2_handler = NULL;

// Set your Static IP address
IPAddress local_IP(192, 168, 60, 3);
// Set your Gateway IP address
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);

enum TimeInterval {
  TIME_INTERVAL = 400,
  ONE_SECOND = 1000,
  TWO_SECOND = 2000,
  THREE_SECOND = 3000,
  TEN_SECOND = 10000,
  ONE_MINUTE = 60000,
};

enum SoundType {
  BUZZER_SUCCESS_READ_WRITE = 100,
  BUZZER_SUCCESS_SEND_RECIEVE = 101,
  BUZZER_ERROR_READ_WRITE = 110,
  BUZZER_ERROR_SEND_RECIEVE = 111,
  BUZZER_POWER_ON = 200
};

enum buzzerSoundTime {
  BUZZER_SHORT_TIME = 100,
  BUZZER_MEDIUM_TIME = 200,
  BUZZER_LONG_TIME = 800,
};

enum ledState {
  LED_OFF = LOW,
  LED_ON = HIGH
};

NTPtime NTPch("ch.pool.ntp.org");  // connect to Server NTP
int Hour;
int Minute;
int Second;
int Week;
int Day;
int Month;
int Year;

String MonthsOfTheYear[] = {"","Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

WiFiManager wm;

strDateTime dateTime;
ThreeWire myWire(DS1302_DAT, DS1302_CLK, DS1302_RST);  // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);
uint8_t IsRtcConnected = 0;
 
uint8_t IsInternetOn = 0;
int PreviousTime = 0, PreviousTime2 = 0;

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
byte sector         = 0;
byte blockAddr      = 1;
byte trailerBlock   = 3;
MFRC522::StatusCode status;
byte buffer[18];
byte dataBlock[16];
byte size = sizeof(buffer);
MFRC522::MIFARE_Key key;

String ReceivedString = "";

enum {
  OPERATION_STATE,
  ERROR_STATE,

  OFFLINE_STATE,
  ONLINE_STATE,

  AP_MODE_STATE,
  SWITCH_AP_MODE_STATE,
  STA_MODE_STATE,
  RESET_MODE_STATE,

  MFRC522_READ_MODE,
  MFRC522_WRITE_MODE,
  MFRC522_ERROR_MODE,

  ATTENDANCE_TIMEIN,
  ATTENDANCE_TIMEOUT,

  GET_NTP_TIME,
  GET_RTC_TIME,

  INTERNET_AVAILABLE,
  INTERNET_NOT_AVAILABLE,

  SD_CARD_FAILED,
  SD_CARD_SUCCESS,

  RTC_MODULE_FAILED,
  RTC_MODULE_SUCCESS,

  CARD_READER_FAILED,
  CARD_READER_SUCCCESS
};

struct SystemConfiguration {
  int systemState;
  int internetState;
  int wifiMode;
  int rfidReaderMode;
  int attendanceMode;
  int internetConnection;
  int sdCardConnection;
  int rtcModuleConnection;
  int cardReaderConnection;
};

SystemConfiguration system1;

/// FOR NTP AND RTC TIME 
bool getNTPDateTime() {
  dateTime = NTPch.getNTPtime(7.0, 0);  // GMT+7

  if (dateTime.valid) {

    Hour = dateTime.hour;      
    Minute = dateTime.minute;  
    Second = dateTime.second; 
    Week = dateTime.dayofWeek;
    Year = dateTime.year;
    Month = dateTime.month;
    Day = dateTime.day;

    return true;
  }
  return false;
}

String getCurrentTimeStringFormat() {
  String timeStr = "";
  timeStr = String(Hour);
  timeStr += ":";
  timeStr += String(Minute);
  timeStr += ":";
  timeStr += String(Second);
  return timeStr;
}

String getCurrentDateStringFormat() {
  String dateStr = String(Day);
   dateStr += "/";
   dateStr += String(Month);
   dateStr += "/";
   dateStr += String(Year);
  return dateStr;
}

bool RTCInit(String dateSet = "", String timeSet = "")
{
  Rtc.Begin();

  RtcDateTime compiled = RtcDateTime(dateSet.c_str(),timeSet.c_str());
  printDateTime();
  Serial.println();

  if (!Rtc.IsDateTimeValid()) {
#ifdef DEBUG_ON
    Serial.println("RTC lost confidence in the DateTime!");
#endif
    Rtc.SetDateTime(compiled);
    return false;
  }

  if (Rtc.GetIsWriteProtected()) {
#ifdef DEBUG_ON
    Serial.println("RTC was write protected, enabling writing now");
#endif
    Rtc.SetIsWriteProtected(false);
    return false;
  }

  if (!Rtc.GetIsRunning()) {
#ifdef DEBUG_ON
    Serial.println("RTC was not actively running, starting now");
#endif
    Rtc.SetIsRunning(true);
    return false;
  }

  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled) {
#ifdef DEBUG_ON
    Serial.println("RTC is older than compile time!  (Updating DateTime)");
#endif
    Rtc.SetDateTime(compiled);
  } else if (now > compiled) {
#ifdef DEBUG_ON
    Serial.println("RTC is newer than compile time. (this is expected)");
#endif
  } else if (now == compiled) {
#ifdef DEBUG_ON
    Serial.println("RTC is the same as compile time! (not expected but all is fine)");
#endif
  }
  return true;
}

bool getRTCDateTime() {
  RtcDateTime now = Rtc.GetDateTime();

  if (now.IsValid())
  {
    Day = now.Day();
    Month = now.Month();
    Year = now.Year();
    Hour = now.Hour();
    Minute = now.Minute();
    Second = now.Second();

    return true;
  }
  return false;
}

void printDateTime() {
  char datestring[26];

  snprintf_P(datestring,
             countof(datestring),
             PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
             Day,
             Month,
             Year,
             Hour,
             Minute,
             Second);

  Serial.println(datestring);
}

String ConvertToRTCDateInitFormat(int month, int day, int year) {
  String dateStr = "";
  dateStr += MonthsOfTheYear[month];
  dateStr += " ";
  dateStr += String(day);
  dateStr += " ";
  dateStr += String(year);
  return dateStr;
}

bool GetDateTime(int type)
{
  if (type == GET_NTP_TIME)
  {
// #ifdef DEBUG_ON
//     Serial.print("NTP:");
// #endif
    return getNTPDateTime();
  }
#ifdef DEBUG_ON
  Serial.print("RTC:");
#endif
  return getRTCDateTime();
}
/// ----------------------------------------------------------------------

/// FOR WIFI SETUP
bool ConnectToWiFi() {
  //This line hides the viewing of ESP as wifi hotspot
  SetLED(LED_WIFI, LED_OFF);
#ifdef DEBUG_ON
  Serial.println(wm.getSSID());
  Serial.println(wm.getPass());
  Serial.println("Connecting to WiFi");
#endif
  WiFi.begin(wm.getSSID(), wm.getPass());

  WiFi.mode(WIFI_STA);

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect();
    WiFi.reconnect();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
#ifdef DEBUG_ON
    Serial.print(".");
#endif
    return false;
  }
  return true;
}
/// ----------------------------------------------------------------------

/// FOR MRC522
bool AuthenticationTag()
{
  // Authenticate using key A
#ifdef DEBUG_ON
  Serial.println(F("Authenticating using key A..."));
#endif
  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
#ifdef DEBUG_ON
      Serial.print(F("PCD_Authenticate() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
#endif
      return false;
  }
  return true;
}

bool WriteToTag(uint8_t pageAddress, byte* buff, byte sizeBuff) {
#ifdef DEBUG_ON
  Serial.print(F("Writing data into block ")); Serial.print(blockAddr);
  Serial.println(F(" ..."));
  dumpInfo(buff, 16); Serial.println();
#endif
  status = (MFRC522::StatusCode)mfrc522.MIFARE_Write(pageAddress, buff, sizeBuff);
  if (status != MFRC522::STATUS_OK) {
#ifdef DEBUG_ON
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
#endif
    return false;
  }
  return true;
}

bool ReadFromTag(uint8_t pageAddress, byte* buff, byte size) {
#ifdef DEBUG_ON
  Serial.print(F("Reading data from block ")); Serial.print(blockAddr);
  Serial.println(F(" ..."));
#endif
  //data in 4 block is readed at once.
  status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(pageAddress, buff, &size);
  if (status != MFRC522::STATUS_OK) {
#ifdef DEBUG_ON
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
#endif
    return false;
  }
  return true;
}

void DumpSectorInfo()
{
    // Dump the sector data
  Serial.println(F("Current data in sector:"));
  mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
  Serial.println();
}

void dumpInfo(byte* ar, int len) {
  for (int i = 0; i < len; i++) {
    if (ar[i] < 0x10)
      Serial.print(F("0"));
    Serial.print(ar[i], HEX);
    Serial.print(F(" "));
  }
  Serial.println();
}

bool cutString(String* arrayString, String text, int lengthToCut)
{
  int index = 0, i = 0;
  int length = text.length();
  if (length < lengthToCut) return false;
  while (index < length)
  {
    arrayString[i] = String(text.substring(index, index+lengthToCut));
    index = index + lengthToCut;
    i++;
  }
  return true;
}

bool ConvertToIDByteArray(byte arrID[], String stringToConvert)
{
  char length = stringToConvert.length();
  for (int i = 0; i < length; i++)
  {
    arrID[i] = stringToConvert[i];
  }
  return true;
}
/// ----------------------------------------------------------------------

// FOR SD CARD
void getSDCardInformations() {
  uint8_t cardType = SD.cardType();
  Serial.print("SD Card Type: ");

  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } 
  else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } 
  else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } 
  else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
}
/// ----------------------------------------------------------------------

/// FOR CUSTOM WEB PAGE
void handleGetURLCallback()
{
  Web_App_URL = wm.getWebAppURL();
  EEPROM.writeString(0, Web_App_URL);
  EEPROM.commit();
#ifdef DEBUG_ON
  Serial.print("Web URL:");
  // Serial.println(Web_App_URL);
  Serial.println(EEPROM.readString(0));
#endif
  BuzzerMakesSound(BUZZER_SUCCESS_SEND_RECIEVE);
}

void createCustomWebpage()
{
  wm.setAPStaticIPConfig(IPAddress(192,168,60,3), IPAddress(192,168,1,1), IPAddress(255,255,255,0));
  std::vector<const char *> menu = {"wifi","info","custom","webAppURL", "exit","sep","update"};
  wm.setMenu(menu); // custom menu, pass vector

  // set custom html menu content, inside menu item custom
  const char* menuhtml = "<form action='/custom' method='get'><button>Sign up ID</button></form><br/>\n";
  wm.setCustomMenuHTML(menuhtml);
  wm.setWebApppUrlFunctionCallback(handleGetURLCallback);
}
/// ----------------------------------------------------------------------

// FOR SENDING TO GOOGLE SHEET
String byteArrayToString(byte array[], unsigned int length) {
  char stringArr[length+1]; 
  for (unsigned int i = 0; i < length; i++)
  {
    stringArr[i] = array[i];
  }
  stringArr[length] = '\0';
  return String(stringArr);
}

String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;
  
  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

int httpRequestToScriptApp(String userAttendance) {
  if (WiFi.status() == WL_CONNECTED) {
    String httpRequestUrl = "";

    httpRequestUrl  = Web_App_URL + "?sts=atc";
    httpRequestUrl += getValue(userAttendance, ',', 0);
    httpRequestUrl += getValue(userAttendance, ',', 1);
    httpRequestUrl += getValue(userAttendance, ',', 2);

#ifdef DEBUG_ON
    //----------------------------------------Sending HTTP requests to Google Sheets.
    Serial.println();
    Serial.println("-------------");
    Serial.println("Sending request to Google Sheets...");
    Serial.print("URL : ");
    Serial.println(httpRequestUrl);
#endif
    // Create an HTTPClient object as "http".
    HTTPClient http;

    // HTTP GET Request.
    http.begin(httpRequestUrl.c_str());
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    // Gets the HTTP status code.
    int httpCode = http.GET(); 
    Serial.print("HTTP Status Code : ");
    Serial.println(httpCode);

    // Getting response from google sheet.
    String payload;
    if (httpCode > 0) {
        payload = http.getString();
#ifdef DEBUG_ON
        Serial.println("Payload : " + payload);  
        Serial.println("-------------");
#endif
    }
    http.end();

    String sts_Res = getValue(payload, ',', 0);
    //----------------------------------------Conditions that are executed are based on the payload response from Google Sheets (the payload response is set in Google Apps Script).
    if (sts_Res == "OK") {
      //..................
      String atcInfo = getValue(payload, ',', 1);
      
      if (atcInfo == "TI_Successful") {
        return 200;
      }
      else if (atcInfo == "TO_Successful") {
        return 200;
      }
      else if (atcInfo == "atcErr01") {
          //handle error
        return 203;
      }
      else if (atcInfo == "atcErr02" || atcInfo == "atcErr03") {
          //handle error
        return 208;
      }
      else 
      {
        return 404;
      }
    }
  }
  return -1;
}

bool CheckIsOnlineInternet()
{
  if (WiFi.status() == WL_CONNECTED) {
    if (getNTPDateTime()) {
      return true;
    }
  }
  return false;
}
/// ----------------------------------------------------------------------

// FOR LED IDICATORS AND BUZZER
void SetLED(int type, int state)
{
  if (type == LED_WIFI) digitalWrite(LED_WIFI, state);
  else if (type == LED_CARD) digitalWrite(LED_CARD, state);
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

void buzzerOscillates(int time, int numberOfTimes = 1) {
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
/// ----------------------------------------------------------------------

/// MAIN
void secondLoopTask(void *parameter)
{
  while (1)
  {
    if (wm.getIDAcknowledge())
    {
      system1.rfidReaderMode = MFRC522_WRITE_MODE;
      ConvertToIDByteArray(dataBlock, wm.getIDString());
      wm.setIDAcknowledge(false);
      
      BuzzerMakesSound(BUZZER_SUCCESS_SEND_RECIEVE);
      BlinkLED(LED_CARD, 2, 100);
    }

    if ( ! mfrc522.PICC_IsNewCardPresent()){
      vTaskDelay(20/portTICK_PERIOD_MS);
      continue;
    }

      // Select one of the cards
    if ( ! mfrc522.PICC_ReadCardSerial()){
      vTaskDelay(20/portTICK_PERIOD_MS);
      continue;
    }

    if (system1.systemState == ERROR_STATE) {
      vTaskDelay(100/portTICK_PERIOD_MS);
      continue;
    }
    // Show some details of the PICC (that is: the tag/card)
#ifdef DEBUG_ON
    Serial.print(F("Card UID:"));
    dumpInfo(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.println();
    Serial.print(F("PICC type: "));
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    Serial.println(mfrc522.PICC_GetTypeName(piccType));

    // Check for compatibility
    if (    piccType != MFRC522::PICC_TYPE_MIFARE_MINI
        &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
        &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
        Serial.println(F("This sample only works with MIFARE Classic cards."));
        continue;
    }
#endif
    
    if (AuthenticationTag()) {
      Serial.println(F("Current data in sector:"));
      Serial.println();
      DumpSectorInfo();
    }
    else {
      system1.rfidReaderMode = MFRC522_ERROR_MODE;
    }

    switch (system1.rfidReaderMode)
    {
      case MFRC522_READ_MODE:
      {
        if (ReadFromTag(blockAddr, buffer, size)) {
#ifdef DEBUG_ON
          Serial.print(F("Data in block ")); Serial.print(blockAddr); Serial.println(F(":"));
          dumpInfo(buffer, 16);
#endif

          String getCurrentUserAttendance = "&uid=";
          getCurrentUserAttendance += byteArrayToString(buffer, 4);
          if (system1.attendanceMode == ATTENDANCE_TIMEOUT)
          {
            getCurrentUserAttendance += ",&to=";
          } 
          else {
            getCurrentUserAttendance += ",&ti=";
          }
          getCurrentUserAttendance += getCurrentTimeStringFormat();
          getCurrentUserAttendance += ",&date=";
          getCurrentUserAttendance += getCurrentDateStringFormat();

          BuzzerMakesSound(BUZZER_SUCCESS_READ_WRITE);
          BlinkLED(LED_CARD, 1, 200);
          
          if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)
          {
            system1.attendanceMode = ATTENDANCE_TIMEIN;
            SetLED(LED_CARD, LED_OFF);
            
            QueueString.push(getCurrentUserAttendance);
          }
          xSemaphoreGive(mutex);
        }
        else {
          system1.rfidReaderMode = MFRC522_ERROR_MODE;
          BuzzerMakesSound(BUZZER_ERROR_READ_WRITE);
        }
        break;
      }
      case MFRC522_WRITE_MODE:
      {
        if (WriteToTag(blockAddr, dataBlock, 16)) {
          system1.rfidReaderMode = MFRC522_READ_MODE;
          
          BuzzerMakesSound(BUZZER_SUCCESS_READ_WRITE);
          BlinkLED(LED_CARD, 1, 200);

#ifdef DEBUG_ON
          Serial.println(F("Checking result:"));
          ReadFromTag(blockAddr, buffer, size);
          Serial.print(F("Data in block ")); Serial.print(blockAddr); Serial.println(F(":"));
          dumpInfo(buffer, 16);
#endif
        } 
        else {
          system1.rfidReaderMode = MFRC522_ERROR_MODE;
          BuzzerMakesSound(BUZZER_ERROR_SEND_RECIEVE);
        }
        break;
      }
      case MFRC522_ERROR_MODE:
      {
        // handleError
        system1.rfidReaderMode = MFRC522_READ_MODE;
        break;
      }
      default:
      {
        system1.rfidReaderMode = MFRC522_READ_MODE;
      }
    }
    // Halt PICC
    mfrc522.PICC_HaltA();
    // Stop encryption on PCD
    mfrc522.PCD_StopCrypto1();
  }
}

#define BTN_NUMER_OF_COUNT 150

bool readButton(int btn, int holdTimeOn = 0) {
  volatile int holdTimeCount = 0;
  if (digitalRead(btn) == HIGH) {
    vTaskDelay(20/portTICK_PERIOD_MS);
    if (digitalRead(btn) == HIGH) {
      while (digitalRead(btn) == HIGH) {
        if (holdTimeOn)
        {
          holdTimeCount++;
          if (holdTimeCount > BTN_NUMER_OF_COUNT)
          {
            holdTimeCount = BTN_NUMER_OF_COUNT;
          }
        }
        vTaskDelay(20/portTICK_PERIOD_MS);
      }
      if (holdTimeOn)
      {
        if (holdTimeCount > BTN_NUMER_OF_COUNT-1) return true;
        else return false;
      }
      else
      {
        return true;
      }
    }
  }
  return false;
}

void handleSetTimeOutButton() {
  if (system1.attendanceMode == ATTENDANCE_TIMEIN) {
    system1.attendanceMode = ATTENDANCE_TIMEOUT;
    BuzzerMakesSound(BUZZER_SHORT_TIME);
    SetLED(LED_CARD, LED_ON);
  }
  else if (system1.attendanceMode == ATTENDANCE_TIMEOUT) {
    system1.attendanceMode = ATTENDANCE_TIMEIN;
    BuzzerMakesSound(BUZZER_SHORT_TIME);
    SetLED(LED_CARD, LED_OFF);
  }
  else {
    system1.attendanceMode = ATTENDANCE_TIMEIN;
    BuzzerMakesSound(BUZZER_SHORT_TIME);
    SetLED(LED_CARD, LED_OFF);
  }
}

  
char SwitchTo = 0;
void thirdLoopTask(void *parameter)
{
  while (1) {
      if (readButton(RESET_WIFI_BTN, 1))
      {
        wm.resetSettings();

        SetLED(LED_WIFI, LED_ON);
        BuzzerMakesSound(BUZZER_SHORT_TIME);
        SetLED(LED_WIFI, LED_OFF);
      }
      if (readButton(SETCARD_BTN))
      {
        SwitchTo = (SwitchTo + 1) % 2;
        if (SwitchTo == 1)
        {
          createCustomWebpage();
          system1.wifiMode = SWITCH_AP_MODE_STATE;
          // wm.startConfigPortal(AP__NAME,AP__PASS);
        }
        else
        {
          // system1.wifiMode = RESET_MODE_STATE;
          wm.setAbort(true);
          system1.wifiMode = STA_MODE_STATE;
          // wm.resetSettings();
        }

        SetLED(LED_CARD, LED_ON);
        BuzzerMakesSound(BUZZER_SHORT_TIME);
        SetLED(LED_CARD, LED_OFF);
      }
      if (readButton(SETTIMEOUT_BTN))
      {
        handleSetTimeOutButton();
      }
    vTaskDelay(100/portTICK_PERIOD_MS);
  }
}

void setup() {
  // put your setup code here, to run once:
  char peripheralsState[3];
#ifdef DEBUG_ON
  Serial.begin(115200);  // Initialize serial communications with the PC
#endif
  system1.systemState           = OPERATION_STATE;
  system1.internetState         = OFFLINE_STATE;
  system1.rfidReaderMode        = MFRC522_READ_MODE;
  system1.attendanceMode        = ATTENDANCE_TIMEIN;
  system1.internetConnection    = INTERNET_NOT_AVAILABLE;
  system1.sdCardConnection      = SD_CARD_FAILED;
  system1.rtcModuleConnection   = RTC_MODULE_FAILED;
  system1.cardReaderConnection  = CARD_READER_FAILED;

  pinMode(RESET_WIFI_BTN, INPUT_PULLDOWN);
  pinMode(SETCARD_BTN, INPUT_PULLDOWN);
  pinMode(SETTIMEOUT_BTN, INPUT_PULLDOWN);
  pinMode(SD_CS_PIN, OUTPUT);  // SS
  pinMode(LED_WIFI, OUTPUT);  
  pinMode(LED_CARD, OUTPUT); 
  pinMode(BUZZER, OUTPUT); 
  
  SetLED(LED_WIFI, LED_OFF);
  SetLED(LED_CARD, LED_OFF);

  BuzzerMakesSound(BUZZER_POWER_ON);

  /// EEPROM INITIALIZATION
  if (!EEPROM.begin(EEPROM_SIZE)) {
#ifdef DEBUG_ON
    Serial.println("Failed to initialise EEPROM");
    Serial.println("Restarting...");
#endif
    BuzzerMakesSound(BUZZER_ERROR_READ_WRITE);
    delay(1000);
    ESP.restart();
  }

  Web_App_URL = EEPROM.readString(0); // read saved Web App URL at address 0
  wm.setWebAppURL(Web_App_URL);
#ifdef DEBUG_ON
  Serial.print("URL:"); 
  Serial.println(Web_App_URL); 
#endif
  /// ------------------------------------------------

  /// MFRC522 INITIALIZATION
  SPI.begin();         // Init SPI bus
  sdSPI2.begin(SD_CLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN); // Initiate SPI2 bus

  mfrc522.PCD_Init();  // Init MFRC522 card
  // mfrc522.PCD_SetAntennaGain(0x07<<4);
  if (mfrc522.PCD_DumpVersionToSerial()) { // Show details of PCD - MFRC522 Card Reader details
    system1.cardReaderConnection = CARD_READER_SUCCCESS;
    peripheralsState[1] = 1;
  }
  else {
    int i = 300;
    while (i--) {
      SetLED(LED_WIFI, LED_ON);
      SetLED(LED_CARD, LED_ON);
      vTaskDelay(500);
      SetLED(LED_WIFI, LED_OFF);
      SetLED(LED_CARD, LED_OFF);
      vTaskDelay(500);
#ifdef DEBUG_ON
      Serial.println("Card reader failed. Need to repair or replace a new module!");
#endif  
    }
    ESP.restart();
  }

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  /// ------------------------------------------------
  
  /// SD INITIALIZATION
  if (SD.begin(SD_CS_PIN, sdSPI2)) {
    // deleteFile(SD, "/data.txt");
    system1.sdCardConnection = SD_CARD_SUCCESS;
    File file = SD.open("/data.txt");
    peripheralsState[2] = 1;

    if(!file) {
#ifdef DEBUG_ON
      Serial.println("File doesn't exist");
      Serial.println("Creating file...");
#endif
    }
    else {
#ifdef DEBUG_ON
      Serial.println("File already exists");  
#endif
    }
    file.close();
    getSDCardInformations();
    readFile(SD, DataFileName.c_str(), ReceivedString, NumberOfLines);
    ReceivedString = "";
  }
  else {
#ifdef DEBUG_ON
    Serial.println("Card Mount Failed");
#endif
  }
  /// ------------------------------------------------

#ifdef DEBUG_ON
  Serial.println("------FreeRTOS Task Start------");
  Serial.print("Setup and loop task running on core ");
  Serial.print(xPortGetCoreID());
  Serial.print(" With priority ");
  Serial.println(uxTaskPriorityGet(NULL));
  Serial.println();
#endif

  mutex = xSemaphoreCreateMutex();

  /// SECOND LOOP INITIALIZATION
  BaseType_t taskCreationResult = xTaskCreatePinnedToCore(  //
                                  secondLoopTask,              // Function to be called
                                  "Task 1",           // Name of task
                                  8192,                   // Stack size (bytes in ESP32, word in FreeRTOS)
                                  NULL,                   // parameter to pass function
                                  1,                      // Task priority ( 0 to configMAX_PRIORITIES - 1)
                                  &task_1_handler,                   // Task handle
                                  app_cpu);               // Run on one core for demo purposes (ESP32 only)
  // Check if task creation was successful
  if (taskCreationResult != pdPASS) {
    BuzzerMakesSound(BUZZER_ERROR_READ_WRITE);
    vTaskDelay(3000/portTICK_PERIOD_MS);

#ifdef DEBUG_ON
    Serial.println("Task creation failed!");
#endif
    ESP.restart();
  }
  /// ------------------------------------------------

  /// WIFI INITIALIZATION
  bool res = false;
  createCustomWebpage();

  while (!res) {
    if(wm.autoConnect(AP__NAME, AP__PASS)) { // password protected ap
      BlinkLED(LED_WIFI, 1, 500);
      for (int i = 0; i < 5; i++) {
        if (CheckIsOnlineInternet()) {
          res = true;
          break;
        }
      }
      system1.wifiMode = AP_MODE_STATE;
#ifdef DEBUG_ON
      Serial.println("Internet is not available!");
#endif
    }
  }
  SetLED(LED_WIFI, LED_ON);
  system1.wifiMode = STA_MODE_STATE;
  system1.internetState = ONLINE_STATE;
  /// ------------------------------------------------

  /// RTC INITIALIZATION
  int remainNumber = 3;
  String dateSet = ConvertToRTCDateInitFormat(Month, Day, Year);
  String timeSet = getCurrentTimeStringFormat();

  while(!RTCInit(dateSet,timeSet) && remainNumber--) {
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }

  if (remainNumber > 0) {
    peripheralsState[0] = 1;
#ifdef DEBUG_ON
      Serial.print("Remain number:");
      Serial.println(remainNumber);
      Serial.println("RTC connects successfully!");
#endif
    system1.rtcModuleConnection = RTC_MODULE_SUCCESS;
  }
  else {
#ifdef DEBUG_ON
      Serial.println("RTC connects failed!");
#endif
  }
  /// ------------------------------------------------

  /// THIRD LOOP INITIALIZATION
  taskCreationResult = xTaskCreatePinnedToCore(  //
                          thirdLoopTask,              // Function to be called
                          "Task 2",           // Name of task
                          4096,                   // Stack size (bytes in ESP32, word in FreeRTOS)
                          NULL,                   // parameter to pass function
                          2,                      // Task priority ( 0 to configMAX_PRIORITIES - 1)
                          &task_2_handler,                   // Task handle
                          app_cpu);               // Run on one core for demo purposes (ESP32 only)
  if (taskCreationResult != pdPASS) {
    BuzzerMakesSound(BUZZER_ERROR_READ_WRITE);
    vTaskDelay(3000/portTICK_PERIOD_MS);

#ifdef DEBUG_ON
    Serial.println("Task creation failed!");
#endif
    ESP.restart();
  }
  /// ------------------------------------------------

  for (int i = 0; i < 3; i++) {
    if (peripheralsState[i] == 1)
      buzzerOscillates(100, i+1);

    vTaskDelay(500/portTICK_PERIOD_MS);
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  int currentTime = millis();
  if (currentTime - PreviousTime > TIME_INTERVAL) {
    PreviousTime = millis();
    if (WiFi.status() != WL_CONNECTED) {
      system1.internetState = OFFLINE_STATE;

      ConnectToWiFi();
        // system1.internetState = ONLINE_STATE;
    }
  
    int count = 0;

    if (system1.wifiMode == SWITCH_AP_MODE_STATE) {
      wm.startConfigPortal(AP__NAME,AP__PASS);
      // Need to put here to reset flag for butn
      SwitchTo = 0;
      system1.wifiMode = STA_MODE_STATE;
      // --------------------------------------
    }

    if (xSemaphoreTake(mutex, 0) == pdTRUE)
    {
      count = QueueString.count();
    }
    xSemaphoreGive(mutex);
    
    if (count > 0) {
      ReceivedString = "empty";
      if (xSemaphoreTake(mutex, 0) == pdTRUE) {
        ReceivedString = QueueString.peek();
      }
      xSemaphoreGive(mutex);
#ifdef DEBUG_ON
      Serial.print("Get from queue:");
      Serial.println(ReceivedString);
#endif
      if (ReceivedString != "empty") {
        if (system1.internetState == ONLINE_STATE) {
          int stateCode = httpRequestToScriptApp(ReceivedString);

#ifdef DEBUG_ON
        Serial.print("State code:");
        Serial.println(stateCode);
#endif
          if (stateCode == 200) {
            BuzzerMakesSound(BUZZER_SUCCESS_SEND_RECIEVE);
            QueueString.pop();
          }
          else if (stateCode == 203 || stateCode == 208) {
            BuzzerMakesSound(BUZZER_ERROR_SEND_RECIEVE);
            QueueString.pop();
          }
          else if (stateCode == -1) {   // internet is offline when sending request
            BuzzerMakesSound(BUZZER_ERROR_SEND_RECIEVE);
            system1.internetState = OFFLINE_STATE;
            if (appendFile(SD, DataFileName.c_str(), ReceivedString.c_str(), true)) {
              QueueString.pop();
              NumberOfLines++;
            }
            else {
              system1.systemState = ERROR_STATE; // ERROR CASE 1: if internet is not available and there are some record have been on queue container but
                                                  //               SD card is failed to communicate, then the system is on error state.
            }
          }
          else {
            BuzzerMakesSound(BUZZER_ERROR_SEND_RECIEVE);
            // Save to SD under the name exception.txt
            if (appendFile(SD, ExceptionalFileName.c_str(), ReceivedString.c_str(), true)) { // In some special case, the system will save the data to exceptional file at which the data
                                                                                              // will be handled by technicians.
              QueueString.pop();
              NumberOfLines++;
            }
          }
        }
        else {
          // Save to SD card until system's online;
          if (appendFile(SD, DataFileName.c_str(), ReceivedString.c_str(), true)) {
            QueueString.pop();
            NumberOfLines++;
            BuzzerMakesSound(BUZZER_SUCCESS_SEND_RECIEVE);
          }
        }

      }
      else {
        QueueString.pop();
      }
    }
  }
  if (currentTime - PreviousTime2 >= TWO_SECOND) {
    PreviousTime2 = millis();
    if (system1.systemState == OPERATION_STATE) 
    {
      if (system1.internetState == ONLINE_STATE) {
        if(!GetDateTime(GET_NTP_TIME)) {
          IsInternetOn += 1;
          if (IsInternetOn > 3) 
          {
#ifdef DEBUG_ON
          Serial.println("Internet is not available now!");
#endif
            system1.internetState = OFFLINE_STATE;
            SetLED(LED_WIFI, LED_OFF);
          }
        }
        else {
          IsInternetOn = 0;
        }

        if (NumberOfLines > 0) {
          // read the first line of file
          if (system1.sdCardConnection == SD_CARD_SUCCESS) {
            if (readFile(SD, DataFileName.c_str(), ReceivedString, NumberOfLines)) { // update NumberOfLine if something error occurs
              if (readALineFile(SD, DataFileName.c_str(), ReceivedString, 0)) {      // get the first line to push into queue
                QueueString.push(ReceivedString);
                if (deleteALineFile(SD,  DataFileName.c_str(), 0)) {                 // delete the first line after reading  
                  NumberOfLines--;                                                   // update number of line remained in the file
                }
              }
            }
            else {
              system1.sdCardConnection = SD_CARD_FAILED;
            }
          } // system1.sdCardConnection == SD_CARD_SUCCESS
        }
      }
      else {
        int count = 0;
        
        if (xSemaphoreTake(mutex, 0) == pdTRUE)
        {
          count = QueueString.count();
        }
        if (count > 0) {
          if (xSemaphoreTake(mutex, 0) == pdTRUE) {
            ReceivedString = QueueString.pop();
          }
          xSemaphoreGive(mutex);

          if (appendFile(SD, DataFileName.c_str(), ReceivedString.c_str(), true)) {
            QueueString.pop();
            NumberOfLines++;
          }
          else {
            system1.systemState = ERROR_STATE;      // ERROR CASE 1: if internet is not available and there are some record have been on queue container but
                                                    //               SD card is failed to communicate, then the system is on error state.
          }
        }

        if (WiFi.status() == WL_CONNECTED) {
          
          if (GetDateTime(GET_NTP_TIME)) {            // Check is internet available again
  #ifdef DEBUG_ON
            Serial.println("Internet is available now!");
  #endif
            system1.internetState = ONLINE_STATE;
            IsInternetOn = 0;
            SetLED(LED_WIFI, LED_ON);
          }
        }
        if (system1.rtcModuleConnection == RTC_MODULE_SUCCESS) {
          if (!GetDateTime(GET_RTC_TIME)) {
            IsRtcConnected += 1;
            if (IsRtcConnected >= 3) system1.rtcModuleConnection = RTC_MODULE_FAILED;
          }
        }
        else {
          system1.systemState = ERROR_STATE;  //ERROR CASE 2: if internet is not available and rtc module is failed, then the system is on error state.
        }
      }
#ifdef DEBUG_ON
        printDateTime();
#endif
    }
    else if (system1.systemState == ERROR_STATE) {
  #ifdef DEBUG_ON
    Serial.println("Error: there is something wrong with the system. Need to reconfigure or reset!");
  #endif
      SetLED(LED_WIFI, LED_ON);
      SetLED(LED_CARD, LED_OFF);
      vTaskDelay(1000);
      SetLED(LED_WIFI, LED_OFF);
      SetLED(LED_CARD, LED_ON);
      vTaskDelay(1000);
      if (CheckIsOnlineInternet()) {
        system1.systemState = OPERATION_STATE;
        system1.internetState = ONLINE_STATE;
        SetLED(LED_WIFI, LED_ON);
        SetLED(LED_CARD, LED_OFF);
      }
    }
  }
}
