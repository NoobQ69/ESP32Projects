#ifndef RD6300_H_
#define RD6300_H_

#include <SPI.h>
#include <rdm6300.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// #include <MFRC522.h>
#include <WiFi.h>
#include <ESP32Ping.h>
#include <HTTPClient.h>
#include <NTPtimeESP.h>
#include <RtcDS1302.h>
// #include <SPI.h>
#include <SD.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFiManager.h>  //https://github.com/tzapu/WiFiManager
#include <EEPROM.h>

#define DEBUG_ON

#include "handles_errors_utilities.h"
#include "Queue.h"
#include "SD_Utilities.h"
#include "Icons.h"
#include "rtc_utilities.h"
#include "rtos_define_vars.h"
#include "oled_utilities.h"
#include "led_buzzer_utilities.h"


// #define LED_CARD 32

#define RESET_WIFI_BTN  34
#define SETCARD_BTN     35
#define SETTIMEOUT_BTN  5
// SPIClass sdSPI2(HSPI);

#define EEPROM_SIZE 118
#define DATA_LOG_FILENAME "/data_log"

String DataFileName = "/data.txt";
String ExceptionalFileName = "/exception.txt";
int NumberOfLines = 0;


#define AP__NAME "RFID-Attendance-Device"
#define AP__PASS "24112411"
String Web_App_URL = "";
// Set your Static IP address
IPAddress local_IP(192, 168, 60, 3);
// Set your Gateway IP address
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);
WiFiManager wm;

enum time_interval 
{
  TIME_INTERVAL = 400,
  HALF_SECOND = 500,
  ONE_SECOND = 1000,
  TWO_SECOND = 2000,
  THREE_SECOND = 3000,
  TEN_SECOND = 10000,
  ONE_MINUTE = 60000,
};

uint8_t IsInternetOn = 0;
int PreviousTime = 0, PreviousTime2 = 0, PreviousTime3 = 0;

#define RDM6300_RX_PIN 16 // read the SoftwareSerial doc above! may need to change this pin to 10...
// #define READ_LED_PIN 13
Rdm6300 rdm6300;
// #define SS_PIN 16
// #define RST_PIN 17
// MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
// byte sector         = 0;
// byte blockAddr      = 1;
// byte trailerBlock   = 3;
// MFRC522::StatusCode status;
// byte buffer[18];
byte dataBlock[16];
// byte size = sizeof(buffer);
// MFRC522::MIFARE_Key key;

String ReceivedString = "";
SPIClass sdSPI2(HSPI);

HTTPClient http;
// int IsFaultedLinkURL = 0;

enum {
  OPERATION_STATE,
  SUSPEND_STATE,
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

  INTERNET_AVAILABLE,
  INTERNET_NOT_AVAILABLE,

  SD_CARD_FAILED,
  SD_CARD_SUCCESS,

  RTC_MODULE_FAILED,
  RTC_MODULE_SUCCESS,

  CARD_READER_FAILED,
  CARD_READER_SUCCCESS,

  ATTENDANCE_MODE,
  REGISTER_MODE
};

typedef struct 
{
  String file_name;
  uint32_t number_of_lines;

} file_system_t;

typedef struct 
{
  int on_time_online_flag;
  int fault_URL_flag;
  int rtc_flag;
  int ntp_flag;
  char invoke_internet_flag;
  bool is_internet_on_flag;

} system_flags_t;

typedef struct 
{
  int system_state;
  int internet_state;
  int wifi_mode;
  int rfid_reader_mode;
  int attendance_mode;
  int internet_connection;
  int sdcard_connection;
  int rtc_module_connection;
  int card_reader_connection;
  system_flags_t system_flags;

} attendance_system_config_t;

attendance_system_config_t system1;
time_attendance_t time_attendance_system;

void handleSetTimeOutButton(bool isInvoked);

#endif