#ifndef MAIN_H
#define MAIN_H

#include "SPIFFS.h"
//#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <ESP_Mail_Client.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <NTPtimeESP.h>
#include <RtcDS1302.h>
#include <ESP32Ping.h>

#include "spiffs_utilities.h"
#include "rtos_utilities.h"
#include "rtc_time_utilities.h"
#include "bitmap_frame_utilities.h"
#include "oled_utilities.h"
#include "email_utilities.h"

#define DEBUG_ON

#define ENTER_BUTTON 17
#define BACK_BUTTON 4
#define UP_BUTTON 18
#define DOWN_BUTTON 19

#define BUZZER_PIN 23
#define LED_STATE_PIN 2 

#define PROXIMITY_SENSOR 15 

enum system_state
{
  SYSTEM_OFF,
  SYSTEM_ON,
};

enum system_security_ontime
{
  SYSTEM_OFF_SECURITY,
  SYSTEM_ON_SECURITY,
  SYSTEM_HALT_SECURITY,
};

AsyncWebServer server(80);

// Search for parameter in HTTP POST request
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "ip";
const char* PARAM_INPUT_4 = "gateway";

const char* PARAM_INPUT_SWITCH_ADDRESS = "output";
const char* PARAM_INPUT_SWITCH_STATE = "state";

const char* PARAM_INPUT_TIME_ON = "time_on";
const char* PARAM_INPUT_TIME_OFF = "time_off";

const char* PARAM_INPUT_EMAIL_NAME = "name_email";
const char* PARAM_INPUT_EMAIL_PASS = "pass_email";
const char* PARAM_INPUT_RECIPIENT_EMAIL = "recipient_email";

// File paths to save input values permanently
const char* ssid_path     = "/ssid.txt";
const char* pass_path     = "/pass.txt";
const char* ip_path       = "/ip.txt";
const char* gateway_path  = "/gateway.txt";
const char* time_on_path  = "/time_on.txt";
const char* time_off_path  = "/time_off.txt";
const char* email_name_path  = "/email_name.txt";
const char* email_pass_path  = "/email_pass.txt";
const char* email_recipient_path  = "/email_recipient.txt";

typedef struct
{
  int state_message;
  int sensor_reading_message;
  int ontime_message;

} queue_message_storage_t;

typedef struct
{
  long int previous_time;
  long int time_interval;

} time_invoke_t;

typedef struct
{
  String ssid;
  String pass;
  String ip;
  String gateway;

} wifi_system_parameters_t;

typedef struct
{
  int rtc_time_state;
  int rtc_module_flag_error;
  int ntp_flag_error;
  int rtc_module_connection;

} rtc_time_flags;

typedef struct
{
  int wifi_state_flag;
  int system_state_flag;
  int led_on_state_flag;
  int distance_sensor_flag;
  int is_sent_email_flag;
  rtc_time_flags rtc_flags;

} system_flags_t;

typedef struct
{
  wifi_system_parameters_t  wifi_params;
  time_invoke_t             time_inv;
  system_flags_t            system_flags;
  time_attendance_t         rtc_time;
  time_attendance_t         time_on_security;
  time_attendance_t         time_off_security;
  queue_message_storage_t   queue_messages;

} device_system_t;

#endif
