#ifndef WATER_TANK_MAIN_PROJECT
#define WATER_TANK_MAIN_PROJECT
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <DNSServer.h>
#include <WebServer.h>
#include "WiFiManager.h"
#include <PubSubClient.h>

#include <NTPtimeESP.h>
#include <RtcDS1302.h>

#include <ESP32Ping.h>

#include "SPIFFS.h"

#include "rtc_utilities.h"
#include "RTOS_utilities.h"
#include "spiffs_utilities.h"
#include "water_tank_obj.h"

#define RXD2 16
#define TXD2 17

#define NUMBER_OF_TANK 8
short register_tank[NUMBER_OF_TANK+1];
#define MQTT_SERVER_CONNECTING_TIMEOUT 5000


typedef struct
{
  bool mqtt_connection_flag;
  bool rtc_confidence_flag;
  bool internet_state_flag;

} system_handle_t;

Water_Tank_t TankSystem[NUMBER_OF_TANK];
int Tank_position = 0;
int Number_of_operate_tank = 2;
rtc_time_t Time1;
system_handle_t System1;
String Result = "";
String Gui_str = "";
String Dashboard_str = "";
String Get_sensor = "";
String Get_sensor_raw = "";
unsigned int Time_count = 0;
volatile bool switch_type_data = false;

#include "hmi_utilities.h"

WiFiClient client;
PubSubClient mqtt_client(client);
HTTPClient http;

const char *ssid = "Sussy_hotspot";
const char *password = "nguyenqu@ng10";
const char *mqtt_server = "192.168.43.96";
const int   mqtt_port = 1883;
const char *mqtt_id = "esp32";

#define AP__NAME "Water_tank_system"
#define AP__PASS "24112411"

#define WT_RELAY_PIN 22
#define WT_BUZZER_PIN 33

#define WT_BUTTON_1 13
#define WT_BUTTON_2 14
#define WT_BUTTON_3 15
#define WT_BUTTON_4 19

String Web_App_URL = "";
// Set your Static IP address
IPAddress local_IP(192, 168, 60, 3);
// Set your Gateway IP address
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);
WiFiManager wm;

#define DEBUG_ON

#endif