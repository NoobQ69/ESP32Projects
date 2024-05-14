#ifndef MAIN_H
#define MAIN_H
#include "SPIFFS.h"
#include "spiffs_utilities.h"
#include "Arduino.h"
#include <Wire.h>
#include "PCF8574.h"
#include "HX710B.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "oled_utilities.h"
#include "bitmap.h"
#include "RTOS_utilities.h"

#define NUMBER_OF_TANK 8
short register_tank[NUMBER_OF_TANK+1];

#include "gui_utilities.h"

#define DEFAULT_REGISTERED_TANK "1,2"
#define DEFAULT_NUMBER_OF_REGISTERED_TANK 2

#define SCK1_PIN 32
#define SDI1_PIN 33

#define SCK2_PIN 25
#define SDI2_PIN 26

#define SCK3_PIN 27
#define SDI3_PIN 14

#define SCK4_PIN 23
#define SDI4_PIN 13

#define SCK5_PIN 19
#define SDI5_PIN 18

// #define SCK6_PIN 5
// #define SDI6_PIN 17

// #define SCK7_PIN 16
// #define SDI7_PIN 4

#define SCK8_PIN 2
#define SDI8_PIN 15

#define RXD2 16
#define TXD2 17

#define FLOAT_SENSOR_TANK_1 P0
#define FLOAT_SENSOR_TANK_2 P1
#define FLOAT_SENSOR_TANK_3 P2
#define FLOAT_SENSOR_TANK_4 P3
#define FLOAT_SENSOR_TANK_5 P4
#define FLOAT_SENSOR_TANK_6 P5
#define FLOAT_SENSOR_TANK_7 P6
#define FLOAT_SENSOR_TANK_8 P7

enum water_tank_parameters
{
  WT_TANK_1 = 0, 
  WT_TANK_2 = 1, 
  WT_TANK_3 = 2, 
  WT_TANK_4 = 3, 
  WT_TANK_5 = 4, 
  WT_TANK_6 = 5, 
  WT_TANK_7 = 6, 
  WT_TANK_8 = 7, 

  WT_OFF_STATE = 0,
  WT_ON_STATE = 1,
  WT_IDLE_STATE = 2,
  WT_DATA_READY = 200,

  WT_SENSOR_READ_RAW,
  WT_SENSOR_READ_WATER_LEVER,

  WT_DASHBOARD_REQUEST,
  WT_DASHBOARD_SENSOR_REQUEST,
  WT_CONTROLLER_REQUEST,
  WT_PROMPT_REQUEST,
  WT_GLOBAL_REQUEST,
};

enum water_pump_commands
{
  WP_LOAD,
  WP_OFF_LOAD,
  WP_FLUSH,
  WP_OFF_FLUSH
};

const char* register_tank_path  = "/register_tank.txt";
const char* num_of_register_tank_path  = "/number_register_tank.txt";
const char* logging_consumed_water_path  = "/number_register_tank.txt";
// const char* email_recipient_path  = "/email_recipient.txt";

typedef struct
{
  unsigned int previous_value;
  unsigned int current_value;
  unsigned int interval_value;
  unsigned int total_value;

} Water_consumtion_t;

typedef struct
{
  int tank_id;
  unsigned int current_volume;
  unsigned int upper_limit;
  unsigned int lower_limit;
  unsigned int capacity;
  int state;
  uint32_t current_volume_raw;
  uint32_t upper_limit_raw;
  uint32_t lower_limit_raw;
  bool mode;
  bool protection;
  bool leakage;

  bool is_calibrated_flag;
  bool overflow_flag;

} Water_Tank_t;

#endif
