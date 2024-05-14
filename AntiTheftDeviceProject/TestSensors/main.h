#ifndef MAIN_H
#define MAIN_H

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "bit_map_frame_utilities.h"
#include "oled_utilities.h"

#define ENTER_BUTTON 5
#define BACK_BUTTON 4
#define UP_BUTTON 18
#define DOWN_BUTTON 19

typedef struct 
{
  int hour;
  int minute;
  int second;
  int week;
  int day;
  int month;
  int year;

} time_attendance_t;

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