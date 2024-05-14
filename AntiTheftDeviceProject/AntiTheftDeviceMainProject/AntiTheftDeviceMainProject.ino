#include "main.h"

device_system_t Device1;
email_data_t Email_container_1;

IPAddress localIP(192, 168, 1, 200); // hardcoded
IPAddress localGateway(192, 168, 1, 1); //hardcoded
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(8, 8, 8, 8);
IPAddress second_dns(8, 8, 4, 4);

void my_timer_callback(TimerHandle_t xTimer) 
{ 
  if ((uint32_t)pvTimerGetTimerID(xTimer) == 0) 
  {
    digitalWrite(BUZZER_PIN, LOW);
  }
}

/// FOR WIFI SETUP
bool init_WiFi() 
{
  WiFi.mode(WIFI_AP_STA);

  WiFi.softAP("Door_open_detector", "24112411");
  if (!WiFi.softAPConfig(localIP, localGateway, subnet))
  {
#ifdef DEBUG_ON
    Serial.println("STA Failed to configure");
#endif
    return false;
  }

#ifdef DEBUG_ON
  Serial.println("STA successful to config");
#endif

  if(Device1.wifi_params.ssid=="")
  {
#ifdef DEBUG_ON
    Serial.println("Undefined SSID or IP address.");
#endif
    return false;
  }
  
  WiFi.begin(Device1.wifi_params.ssid.c_str(), Device1.wifi_params.pass.c_str());

#ifdef DEBUG_ON
  Serial.println("Connecting to WiFi...");
#endif

  unsigned long current_millis = millis();
  Device1.time_inv.previous_time = current_millis;

  while(WiFi.status() != WL_CONNECTED) 
  {
    current_millis = millis();
    if (current_millis - Device1.time_inv.previous_time >= Device1.time_inv.time_interval) 
    {
#ifdef DEBUG_ON
      Serial.println("Failed to connect.");
#endif
      return false;
    }
  }
  Device1.system_flags.wifi_state_flag = 1;

#ifdef DEBUG_ON
  Serial.println(WiFi.localIP());
#endif
  return true;
}

String split_value(String data, char separator, int index) 
{
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

bool convert_time_string(String value_to_split, int &hour, int &minute)
{
  if (value_to_split != "")
  {
    hour = split_value(value_to_split, ':', 0).toInt();
    minute = split_value(value_to_split, ':', 1).toInt();
    return true;
  }
  return false;
}

void set_time_security(time_attendance_t &time, int hours = 6, int minutes = 0, int seconds = 0)
{
  time.hour = hours;
  time.minute = minutes;
  time.second = seconds;
}

void handle_when_task_creation_failed(String failed_message = "")
{
#ifdef DEBUG_ON
  Serial.println(failed_message);
#endif
  vTaskDelay(2000/portTICK_PERIOD_MS);
  ESP.restart();
}

String get_system_state(String message_on = "checked", String message_off = "")
{
  if(Device1.system_flags.system_state_flag == SYSTEM_ON)
  {
    return message_on;
  }
  else 
  {
    return message_off;
  }
}

// Replaces placeholder with LED state value
String sending_callback_func(const String& var) 
{
  //Serial.println(var);
  if(var == "BUTTONPLACEHOLDER")
  {
    String buttons = "";
    buttons += "<h4>System - " + get_system_state("On", "Off") + "</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"2\" ";
    buttons += get_system_state() + "><span class=\"slider\"></span></label>";
    return buttons;
  }
  return String();
}

bool read_button(int btn, void (*callback_func)() = NULL) 
{
  if (digitalRead(btn) == LOW) 
  {
    vTaskDelay(20/portTICK_PERIOD_MS);
    if (digitalRead(btn) == LOW) 
    {
      while (digitalRead(btn) == LOW) 
      {
        vTaskDelay(20/portTICK_PERIOD_MS);
      }
      if (callback_func != NULL)
      {
        callback_func();
      }
      return true;
    }
  }
  return false;
}

// ------------------------------------ FOR OLED PRINT SCREEN -------------------------------------------
void update_menu_mode()
{
  if (Buttons_1.back_btn == 0)
  {
    oled128x32_drawBitmap(bitmap_thick_rec, 0, 0, 128, 20, false);
    oled128x32_drawBitmap(bitmap_border_rec, 0, 21, 128, 20, false);
    oled128x32_drawBitmap(bitmap_border_rec, 0, 42, 128, 20, false);
  }
  else if (Buttons_1.back_btn == 1)
  {
    oled128x32_drawBitmap(bitmap_border_rec, 0, 0, 128, 20, false);
    oled128x32_drawBitmap(bitmap_thick_rec, 0, 21, 128, 20, false);
    oled128x32_drawBitmap(bitmap_border_rec, 0, 42, 128, 20, false);
  }
  else
  {
    oled128x32_drawBitmap(bitmap_border_rec, 0, 0, 128, 20, false);
    oled128x32_drawBitmap(bitmap_border_rec, 0, 21, 128, 20, false);
    oled128x32_drawBitmap(bitmap_thick_rec, 0, 42, 128, 20, false);
  }

  oled128x32_printText("Monitor", 5, 5, 1);
  oled128x32_printText("Set time", 5, 26, 1);
  oled128x32_printText("Info", 5, 47, 1);
  display.clearDisplay();
}

void update_monitor_mode()
{
  if (Device1.system_flags.system_state_flag == SYSTEM_OFF)
  {
    oled128x32_printText("System OFF", 0, 20, 2);
  }
  else
  {
    oled128x32_printText("System ON", 0, 20, 2);
  }
  String time = String(Device1.rtc_time.hour) + ":" + String(Device1.rtc_time.minute);
  oled128x32_printText(time.c_str(), 10, 38, 1);

  oled128x32_printText("WiFi", 10, 50, 1);
  if (Device1.system_flags.wifi_state_flag == 1)
  {
    oled128x32_printText("ON", 50, 50, 1);
  }
  else
  {
    oled128x32_printText("OFF", 50, 50, 1);
  }
  display.clearDisplay();
}

void update_setting_mode()
{
  if (Buttons_1.enter_btn == 0)
  {
    oled128x32_drawBitmap(bitmap_thick_box, 10, 10, 40, 20, false);
    oled128x32_drawBitmap(bitmap_box_rec, 60, 10, 40, 20, false);
    oled128x32_drawBitmap(bitmap_box_rec, 10, 44, 40, 20, false);
    oled128x32_drawBitmap(bitmap_box_rec, 60, 44, 40, 20, false);
  }
  else if (Buttons_1.enter_btn == 1)
  {
    oled128x32_drawBitmap(bitmap_box_rec, 10, 10, 40, 20, false);
    oled128x32_drawBitmap(bitmap_thick_box, 60, 10, 40, 20, false);
    oled128x32_drawBitmap(bitmap_box_rec, 10, 44, 40, 20, false);
    oled128x32_drawBitmap(bitmap_box_rec, 60, 44, 40, 20, false);
  }
  else if (Buttons_1.enter_btn == 2)
  {
    oled128x32_drawBitmap(bitmap_box_rec, 10, 10, 40, 20, false);
    oled128x32_drawBitmap(bitmap_box_rec, 60, 10, 40, 20, false);
    oled128x32_drawBitmap(bitmap_thick_box, 10, 44, 40, 20, false);
    oled128x32_drawBitmap(bitmap_box_rec, 60, 44, 40, 20, false);
  }
  else
  {
    oled128x32_drawBitmap(bitmap_box_rec, 10, 10, 40, 20, false);
    oled128x32_drawBitmap(bitmap_box_rec, 60, 10, 40, 20, false);
    oled128x32_drawBitmap(bitmap_box_rec, 10, 44, 40, 20, false);
    oled128x32_drawBitmap(bitmap_thick_box, 60, 44, 40, 20, false);
  }
  // *******************************************
  oled128x32_printText(String(Device1.time_on_security.hour).c_str(), 14, 14, 1);
  oled128x32_printText(String(Device1.time_on_security.minute).c_str(), 64, 14, 1);
  oled128x32_printText(String(Device1.time_off_security.hour).c_str(), 14, 48, 1);
  oled128x32_printText(String(Device1.time_off_security.minute).c_str(), 64, 48, 1);
  // *******************************************
  oled128x32_printText("Time on", 0, 0, 1);
  oled128x32_printText("Time off", 0, 32, 1);
  display.clearDisplay();
}

void update_info_mode()
{
  oled128x32_printText("Email:", 0, 0, 1);
  oled128x32_printText(Email_container_1.author_email.c_str(), 0, 20, 1);
  oled128x32_printText("Recipient:", 0, 40, 1);
  oled128x32_printText(Email_container_1.recipient_email.c_str(), 0, 50, 1);
  display.clearDisplay();
}

void handle_back_menu()
{
  Buttons_1.back_btn = (Buttons_1.back_btn + 1) % 3;
  update_menu_mode();
}

void handle_enter_menu()
{
  if (Buttons_1.back_btn == 0)
  {
    Buttons_1.enter_btn = 0;
    Buttons_1.oled_mode = OLED_MONITOR;
    update_monitor_mode();
  }
  else if (Buttons_1.back_btn == 1)
  {
    Buttons_1.enter_btn = 0;
    Buttons_1.oled_mode = OLED_SETTING;
    update_setting_mode();
  }
  else if (Buttons_1.back_btn == 2)
  {
    Buttons_1.oled_mode = OLED_INFO;
    update_info_mode();
  }
}

void handle_enter_monitor()
{
  Buttons_1.enter_btn = (Buttons_1.enter_btn + 1) % 2;

  if (Buttons_1.enter_btn == 0)
  {
    //*****************************
    Device1.system_flags.system_state_flag = SYSTEM_OFF;
  }
  else if (Buttons_1.enter_btn == 1)
  {
    //*****************************
    Device1.system_flags.system_state_flag = SYSTEM_ON;
  }
  xQueueSend(handle_state_msg_queue, (void *)&Device1.queue_messages.state_message, 0);
  update_monitor_mode();
}

void handle_back_monitor()
{
  String save_time = String(Device1.time_on_security.hour) + ":" + String(Device1.time_on_security.minute);
  writeFile(SPIFFS, time_on_path, save_time.c_str());
  save_time = String(Device1.time_off_security.hour) + ":" + String(Device1.time_off_security.minute);
  writeFile(SPIFFS, time_off_path, save_time.c_str());

  Buttons_1.enter_btn = 0;
  Buttons_1.oled_mode = OLED_MENU;
  update_menu_mode();
}

void handle_enter_setting()
{
  Buttons_1.enter_btn = (Buttons_1.enter_btn + 1) % 4;
  update_setting_mode();
}

void handle_up_setting()
{
  if (Buttons_1.enter_btn == 0)
  {
    Device1.time_on_security.hour++;
    if (Device1.time_on_security.hour > 23) Device1.time_on_security.hour = 0;
  }
  else if (Buttons_1.enter_btn == 1)
  {
    Device1.time_on_security.minute++;
    if (Device1.time_on_security.minute > 59) Device1.time_on_security.minute = 0;
  }
  else if (Buttons_1.enter_btn == 2)
  {
    Device1.time_off_security.hour++;
    if (Device1.time_off_security.hour > 23) Device1.time_off_security.hour = 0;
  }
  else if (Buttons_1.enter_btn == 3)
  {
    Device1.time_off_security.minute++;
    if (Device1.time_off_security.minute > 59) Device1.time_off_security.minute = 0;
  }
  update_setting_mode();
}

void handle_down_setting()
{
  if (Buttons_1.enter_btn == 0)
  {
    Device1.time_on_security.hour--;
    if (Device1.time_on_security.hour < 0) Device1.time_on_security.hour = 23;
  }
  else if (Buttons_1.enter_btn == 1)
  {
    Device1.time_on_security.minute--;
    if (Device1.time_on_security.minute < 0) Device1.time_on_security.minute = 59;
  }
  if (Buttons_1.enter_btn == 2)
  {
    Device1.time_off_security.hour--;
    if (Device1.time_off_security.hour < 0) Device1.time_off_security.hour = 23;
  }
  else if (Buttons_1.enter_btn == 3)
  {
    Device1.time_off_security.minute--;
    if (Device1.time_off_security.minute < 0) Device1.time_off_security.minute = 59;
  }
  update_setting_mode();
}

void handle_modes_oled()
{
  switch (Buttons_1.oled_mode)
  {
    case OLED_MONITOR:
    {
      read_button(BACK_BUTTON, handle_back_monitor);
      read_button(ENTER_BUTTON, handle_enter_monitor);
      break;
    }
    case OLED_MENU:
    {
      read_button(BACK_BUTTON, handle_back_menu);
      read_button(ENTER_BUTTON, handle_enter_menu);
      break;
    }
    case OLED_SETTING:
    {
      read_button(ENTER_BUTTON, handle_enter_setting);
      read_button(BACK_BUTTON, handle_back_monitor);
      read_button(UP_BUTTON, handle_up_setting);
      read_button(DOWN_BUTTON, handle_down_setting);
      break;
    }
    case OLED_INFO:
    {
      read_button(BACK_BUTTON, handle_back_monitor);
      break;
    }
    default:
    {
      Buttons_1.oled_mode = OLED_MENU;
    }
  }
}
// --------------------------------------------------------------------------------------------------

void buttons_loop_task(void *parameters)
{
  while (1)
  {
    handle_modes_oled();
    vTaskDelay(100/portTICK_PERIOD_MS);
  }
}

void ontime_security_loop_task(void *parameters)
{
  while (1)
  {
    Serial.println("ontime secur is on");
    if (xQueueReceive(ontime_secu_msg_queue, (void *)&Device1.queue_messages.ontime_message, 0) == pdTRUE)
    {
      if (Device1.queue_messages.ontime_message == 0)
      {
        if (Device1.system_flags.rtc_flags.rtc_time_state == SYSTEM_ON_SECURITY)
          xQueueSend(distance_sensor_msg_queue, (void *)&Device1.queue_messages.sensor_reading_message, 100);
        
        vTaskSuspend(NULL);
      }
    }

    if (Device1.system_flags.rtc_flags.rtc_time_state == SYSTEM_ON_SECURITY)
    {
      Serial.println("Go off");
      // Device1.system_flags.rtc_flags.rtc_time_state = SYSTEM_HALT_SECURITY;
      
      if ((Device1.rtc_time.hour > Device1.time_off_security.hour)||(Device1.rtc_time.hour == Device1.time_off_security.hour && 
              Device1.rtc_time.minute >= Device1.time_off_security.minute))
      {
//        vTaskSuspend(task_4_handler);
        xQueueSend(distance_sensor_msg_queue, (void *)&Device1.queue_messages.sensor_reading_message, 100);
        Device1.system_flags.rtc_flags.rtc_time_state = SYSTEM_OFF_SECURITY;
      }
    }
    else if (Device1.system_flags.rtc_flags.rtc_time_state == SYSTEM_OFF_SECURITY)
    {
      // Device1.system_flags.rtc_flags.rtc_time_state = SYSTEM_HALT_SECURITY;
      Serial.println("Go on");
      if ((Device1.rtc_time.hour > Device1.time_on_security.hour) && (Device1.rtc_time.hour < Device1.time_off_security.hour))
      {
        vTaskResume(task_4_handler);
        Device1.system_flags.rtc_flags.rtc_time_state = SYSTEM_ON_SECURITY;
      }
      else if ((Device1.rtc_time.hour >= Device1.time_on_security.hour) &&
              (Device1.rtc_time.minute >= Device1.time_on_security.minute) && 
              ((Device1.rtc_time.hour <= Device1.time_off_security.hour) && 
               (Device1.rtc_time.minute <= Device1.time_off_security.minute)))
       {
        vTaskResume(task_4_handler);
        Device1.system_flags.rtc_flags.rtc_time_state = SYSTEM_ON_SECURITY;
       }
    }
    else
    {
      Device1.system_flags.rtc_flags.rtc_time_state = SYSTEM_OFF_SECURITY;
    }
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
}

void rtc_time_loop_task(void *parameters)
{
  while (1)
  {
    if (Device1.system_flags.rtc_flags.rtc_module_connection == RTC_MODULE_SUCCESS) 
    {
      if (!get_date_time(Device1.rtc_time, GET_RTC_TIME)) 
      {
        for (int i = 0; i < 3; i++)
        {
          vTaskDelay(500 / portTICK_PERIOD_MS);

          if (!get_date_time(Device1.rtc_time, GET_RTC_TIME))
          {
            Device1.system_flags.rtc_flags.rtc_module_flag_error += 1;
            if (Device1.system_flags.rtc_flags.rtc_module_flag_error > 2) Device1.system_flags.rtc_flags.rtc_module_connection = RTC_MODULE_FAILED;
          }
          else
          {
            Device1.system_flags.rtc_flags.rtc_module_flag_error = 0;
          }
        }
      }
    }
    else 
    {
      if (!get_date_time(Device1.rtc_time, GET_NTP_TIME)) 
      {
        Device1.system_flags.rtc_flags.ntp_flag_error++;
        if (Device1.system_flags.rtc_flags.ntp_flag_error > 4)
        {
          // system1.system_state = ERROR_STATE;  //ERROR CASE 2: if internet is not available and rtc module is failed, then the system is on error state.
          
          // OLED_PACKAGE pkgFromDate;
          // pkgFromDate.content = " Get time failed";
          // pkgFromDate.type = OLED_ERROR_STATE;
          // xQueueSend(msg_queue, (void *)&pkgFromDate, 0);
        }
      }
      else
      {
        Device1.system_flags.rtc_flags.ntp_flag_error = 0;
      }
    }

#ifdef DEBUG_ON
    print_date_time(Device1.rtc_time);
#endif
//     if(Ping.ping("www.google.com")) 
//     {
// #ifdef DEBUG_ON
//       Serial.println("Success! Internet is available.");
// #endif
//     }
//     else
//     {
// #ifdef DEBUG_ON
//       Serial.println("Failed! Internet is not available.");
// #endif
//     }
    vTaskDelay(2000/portTICK_PERIOD_MS);
  }
}

void distance_sensor_loop_task(void *parameters)
{
  while (1)
  {
    if (xQueueReceive(distance_sensor_msg_queue, (void *)&Device1.queue_messages.sensor_reading_message, 100) == pdTRUE)
    {
      if (Device1.queue_messages.sensor_reading_message == 0)
        vTaskSuspend(NULL);
    }

    int get_sensor_state = digitalRead(PROXIMITY_SENSOR);
    
    if (get_sensor_state == HIGH)
    {
#ifdef DEBUG_ON
      Serial.println("HIGH");
#endif
      digitalWrite(BUZZER_PIN, HIGH);
      if (Device1.system_flags.wifi_state_flag == 1)
      {
        if (Device1.system_flags.is_sent_email_flag == 0)
        {
          smtp_init(Email_container_1);
          smtp_send_email(smtp, message);
          Device1.system_flags.is_sent_email_flag = 1;
        } 
      }
      xTimerStart(one_shot_timer, portMAX_DELAY);
    }
    else
    {
#ifdef DEBUG_ON
      Serial.println("LOW");
#endif
    }
    vTaskDelay(500/portTICK_PERIOD_MS);
  }
}

void system_handle_state_loop_task(void *parameters)
{
  while (1)
  {
    if (xQueueReceive(handle_state_msg_queue, (void *)&Device1.queue_messages.state_message, 0) == pdTRUE)
    { 
      if (Device1.system_flags.system_state_flag == SYSTEM_OFF)
      {
        xQueueSend(ontime_secu_msg_queue, (void *)&Device1.queue_messages.ontime_message, 0); 
        // vTaskSuspend(task_1_handler);
      }
      else if (Device1.system_flags.system_state_flag == SYSTEM_ON)
      {
        Device1.system_flags.rtc_flags.rtc_time_state = SYSTEM_OFF_SECURITY;
        vTaskResume(task_1_handler);
        Device1.system_flags.is_sent_email_flag = 0;
      }
      else
      {
        Device1.system_flags.rtc_flags.rtc_time_state = SYSTEM_OFF_SECURITY;
      }
    }
    vTaskDelay(200/portTICK_PERIOD_MS);
  }
}

void setup() 
{
  // Serial port for debugging purposes
  Serial.begin(115200);

  initSPIFFS();

  // Set GPIO 2 as an OUTPUT
  // pinMode(ledPin, OUTPUT);
  // digitalWrite(ledPin, LOW);
  pinMode(PROXIMITY_SENSOR, INPUT);
  
  pinMode(ENTER_BUTTON, INPUT_PULLUP);
  pinMode(BACK_BUTTON, INPUT_PULLUP);
  pinMode(UP_BUTTON, INPUT_PULLUP);
  pinMode(DOWN_BUTTON, INPUT_PULLUP);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_STATE_PIN, OUTPUT); 

  Device1.system_flags.system_state_flag = SYSTEM_OFF;
  Device1.system_flags.led_on_state_flag = LOW;
  Device1.system_flags.distance_sensor_flag = LOW;
  Device1.system_flags.is_sent_email_flag = 0;
  Device1.system_flags.wifi_state_flag = 0;

  Device1.system_flags.rtc_flags.rtc_time_state = SYSTEM_OFF_SECURITY;
  Device1.system_flags.rtc_flags.rtc_module_flag_error = 0;
  Device1.system_flags.rtc_flags.ntp_flag_error = 0;
  Device1.system_flags.rtc_flags.rtc_module_connection = RTC_MODULE_FAILED;


  // Load values saved in SPIFFS
  Device1.wifi_params.ssid = readFile(SPIFFS, ssid_path);
  Device1.wifi_params.pass = readFile(SPIFFS, pass_path);
  Device1.wifi_params.ip = readFile(SPIFFS, ip_path);
  Device1.wifi_params.gateway = readFile(SPIFFS, gateway_path);
  
  Email_container_1.author_email = readFile(SPIFFS, email_name_path);
  Email_container_1.author_pass = readFile(SPIFFS, email_pass_path);
  Email_container_1.recipient_email = readFile(SPIFFS, email_recipient_path);

  String read_time_on = readFile(SPIFFS, time_on_path);
  String read_time_off = readFile(SPIFFS, time_off_path);

  if (!(convert_time_string(read_time_on, Device1.time_on_security.hour, Device1.time_on_security.minute) 
      && convert_time_string(read_time_off, Device1.time_off_security.hour, Device1.time_off_security.minute)))
  {
    set_time_security(Device1.time_on_security, 18, 0, 0);
    set_time_security(Device1.time_off_security, 21, 0, 0);
  }

  Device1.time_inv.previous_time = 0;
  Device1.time_inv.time_interval = 10000;

  Device1.queue_messages.state_message = 0;
  Device1.queue_messages.sensor_reading_message = 0;
  Device1.queue_messages.ontime_message = 0;

  // ---------------------------FOR CREATING QUEUE-------------------------------
  handle_state_msg_queue = xQueueCreate(handle_state_msg_queue_len, sizeof(int));
  distance_sensor_msg_queue = xQueueCreate(distance_sensor_msg_queue_len, sizeof(int));
  ontime_secu_msg_queue = xQueueCreate(ontime_secu_msg_queue_len, sizeof(int));
  // ----------------------------------------------------------------------------
  
  int remain_number = 5;
  if (init_WiFi())
  {
    while (get_ntp_date_time(Device1.rtc_time) == false && remain_number > 0)
    {
      remain_number--;
      vTaskDelay(2000/portTICK_PERIOD_MS);  
    }

    if (remain_number == 0)
    {
  #ifdef DEBUG_ON
      Serial.println("Get ntp failed!");
  #endif
    }

      /// ------------------------------------RTC INITIALIZATION------------------------------------------

    remain_number = 3;
    String date_set = convert_to_rtc_date_init_format(Device1.rtc_time.month, Device1.rtc_time.day, Device1.rtc_time.year);
    String time_set = get_current_time_String_format(Device1.rtc_time);
  
    while(!rtc_init(date_set, time_set) && remain_number--)
    {
      vTaskDelay(1000/portTICK_PERIOD_MS);
    }
  
    if (remain_number > 0) 
    {
  
  #ifdef DEBUG_ON
        Serial.print("Remain number:");
        Serial.println(remain_number);
        Serial.println("RTC connects successfully!");
  #endif
  
      Device1.system_flags.rtc_flags.rtc_module_connection = RTC_MODULE_SUCCESS;
    }
    else 
    {
  #ifdef DEBUG_ON
        Serial.println("RTC connects failed!");
  #endif
    }
  }
  // -------------------------------------------------------------------------------------

#ifdef DEBUG_ON
  Serial.println(Device1.wifi_params.ssid);
  Serial.println(Device1.wifi_params.pass);
  Serial.println(Device1.wifi_params.ip);
  Serial.println(Device1.wifi_params.gateway);
#endif


  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) 
  {
    Serial.println(F("SSD1306 allocation failed"));
    while (1)
    {
      delay(500);
    }
  }
  Buttons_1.oled_mode = 0;
  display.clearDisplay();
  
  one_shot_timer = xTimerCreate(
                    "One-shot timer buzz",           // Name of timer
                    30000 / portTICK_PERIOD_MS,  // Period of timer (in ticks)
                    pdFALSE,                    // Auto-reload
                    (void *)0,                  // Timer ID
                    my_timer_callback);           // Callback function

  if (!freeRTOS_task_create_pinned_to_core(ontime_security_loop_task, "Task 1", 4096, NULL, 2, &task_1_handler, app_cpu))
  {
    handle_when_task_creation_failed("Task on-time security failed!");
  }
  vTaskSuspend(task_1_handler);
  if (!freeRTOS_task_create_pinned_to_core(buttons_loop_task, "Task 2", 32768, NULL, 2, &task_2_handler, app_cpu))
  {
   handle_when_task_creation_failed("Task buttons failed!");
  }
  if (!freeRTOS_task_create_pinned_to_core(rtc_time_loop_task, "Task 3", 4096, NULL, 2, &task_3_handler, app_cpu))
  {
    handle_when_task_creation_failed("Task rtc time failed!");
  }
  if (!freeRTOS_task_create_pinned_to_core(distance_sensor_loop_task, "Task 4", 16384, NULL, 2, &task_4_handler, app_cpu))
  {
    handle_when_task_creation_failed("Task sensor time failed!");
  }
  vTaskSuspend(task_4_handler);

  if (!freeRTOS_task_create_pinned_to_core(system_handle_state_loop_task, "Task 5", 4096, NULL, 2, &task_5_handler, app_cpu))
  {
    handle_when_task_creation_failed("Task handle state time failed!");
  }
  
  update_menu_mode();
  
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) 
  {
    request->send(SPIFFS, "/index.html", "text/html", false, sending_callback_func);
  });
  server.serveStatic("/", SPIFFS, "/");
  
  // Route to set GPIO state to HIGH
  server.on("/wifi_config", HTTP_GET, [](AsyncWebServerRequest *request) {
    // digitalWrite(ledPin, HIGH);
    request->send(SPIFFS, "/wifi_config.html", "text/html", false, sending_callback_func);
  });

  // Route to set GPIO state to LOW
  server.on("/set_time", HTTP_GET, [](AsyncWebServerRequest *request) {
    // digitalWrite(ledPin, LOW);
    request->send(SPIFFS, "/set_time.html", "text/html", false, sending_callback_func);
  });

  server.on("/register_email", HTTP_GET, [](AsyncWebServerRequest *request) {
    // digitalWrite(ledPin, LOW);
    request->send(SPIFFS, "/register_email.html", "text/html", false, sending_callback_func);
  });

  server.on("/wifi_submit", HTTP_POST, [](AsyncWebServerRequest *request) 
  {
    request->send(SPIFFS, "/submit_result.html", "text/html", false, sending_callback_func);
    int params = request->params();
    for(int i=0;i<params;i++)
    {
      AsyncWebParameter* p = request->getParam(i);
      if(p->isPost())
      {
        // HTTP POST ssid value
        if (p->name() == PARAM_INPUT_1) 
        {
          Device1.wifi_params.ssid = p->value().c_str();
#ifdef DEBUG_ON
          Serial.print("SSID set to: ");
          Serial.println(Device1.wifi_params.ssid);
#endif
          // Write file to save value
          writeFile(SPIFFS, ssid_path, Device1.wifi_params.ssid.c_str());
        }
        // HTTP POST pass value
        if (p->name() == PARAM_INPUT_2) 
        {
          Device1.wifi_params.pass = p->value().c_str();
#ifdef DEBUG_ON
          Serial.print("Password set to: ");
          Serial.println(Device1.wifi_params.pass);
#endif
          // Write file to save value
          writeFile(SPIFFS, pass_path, Device1.wifi_params.pass.c_str());
        }
        // HTTP POST ip value
        if (p->name() == PARAM_INPUT_3) 
        {
          Device1.wifi_params.ip = p->value().c_str();
#ifdef DEBUG_ON
          Serial.print("IP Address set to: ");
          Serial.println(Device1.wifi_params.ip);
#endif
          // Write file to save value
          writeFile(SPIFFS, ip_path, Device1.wifi_params.ip.c_str());
        }
        // HTTP POST gateway value
        if (p->name() == PARAM_INPUT_4) 
        {
          Device1.wifi_params.gateway = p->value().c_str();
#ifdef DEBUG_ON
          Serial.print("Gateway set to: ");
          Serial.println(Device1.wifi_params.gateway);
#endif
          // Write file to save value
          writeFile(SPIFFS, gateway_path, Device1.wifi_params.gateway.c_str());
        }
        //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }
    request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + Device1.wifi_params.ip);
    delay(3000);
    ESP.restart();
  });

  server.on("/settime_submit", HTTP_POST, [](AsyncWebServerRequest *request) 
  {
    request->send(SPIFFS, "/submit_result.html", "text/html", false, sending_callback_func);
    int params = request->params();
    String get_time_str = "";

    for(int i=0;i<params;i++)
    {
      AsyncWebParameter* p = request->getParam(i);
      if(p->isPost())
      {
        // HTTP POST ssid value
        if (p->name() == PARAM_INPUT_TIME_ON) 
        {
          get_time_str = p->value().c_str();
          writeFile(SPIFFS, time_on_path, get_time_str.c_str());
          Device1.time_on_security.hour = split_value(get_time_str, ':', 0).toInt();
          Device1.time_on_security.minute = split_value(get_time_str, ':', 1).toInt();
#ifdef DEBUG_ON
          Serial.print("time on set to: ");
          Serial.print(Device1.time_on_security.hour);
          Serial.print(":");
          Serial.println(Device1.time_on_security.minute);
#endif
        }
        // HTTP POST pass value
        if (p->name() == PARAM_INPUT_TIME_OFF) 
        {
          get_time_str = p->value().c_str();
          writeFile(SPIFFS, time_off_path, get_time_str.c_str());
          Device1.time_off_security.hour = split_value(get_time_str, ':', 0).toInt();
          Device1.time_off_security.minute = split_value(get_time_str, ':', 1).toInt();
#ifdef DEBUG_ON
          Serial.print("time off set to: ");
          Serial.print(Device1.time_off_security.hour);
          Serial.print(":");
          Serial.print(Device1.time_off_security.minute);
#endif
        }
      }
    }
    request->send(200, "text/plain", "Done. Times have been saved!");
  });

  server.on("/reg_email_submit", HTTP_POST, [](AsyncWebServerRequest *request) 
  {
    request->send(SPIFFS, "/submit_result.html", "text/html", false, sending_callback_func);
    int params = request->params();
    String get_email_str = "";

    for(int i=0;i<params;i++)
    {
      AsyncWebParameter* p = request->getParam(i);
      if(p->isPost())
      {
        // HTTP POST ssid value
        if (p->name() == PARAM_INPUT_EMAIL_NAME) 
        {
          get_email_str = p->value().c_str();
          writeFile(SPIFFS, email_name_path, get_email_str.c_str());
          Email_container_1.author_email = get_email_str;
#ifdef DEBUG_ON
          Serial.println(Email_container_1.author_email);
#endif
        }
        // HTTP POST pass value
        if (p->name() == PARAM_INPUT_EMAIL_PASS) 
        {
          get_email_str = p->value().c_str();
          writeFile(SPIFFS, email_pass_path, get_email_str.c_str());
          Email_container_1.author_pass = get_email_str;
#ifdef DEBUG_ON
          Serial.println(Email_container_1.author_pass);
#endif
        }

        if (p->name() == PARAM_INPUT_RECIPIENT_EMAIL) 
        {
          get_email_str = p->value().c_str();
          writeFile(SPIFFS, email_recipient_path, get_email_str.c_str());
          Email_container_1.recipient_email = get_email_str;
#ifdef DEBUG_ON
          Serial.println(Email_container_1.recipient_email);
#endif
        }
      }
    }
    request->send(200, "text/plain", "Done. Times have been saved!");
  });

  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) 
  {
    String input_message1;
    String input_message2;
    // GET input1 value on <ESP_IP>/update?output=<input_message1>&state=<input_message2>
    if (request->hasParam(PARAM_INPUT_SWITCH_ADDRESS) && request->hasParam(PARAM_INPUT_SWITCH_STATE)) 
    {
      input_message1 = request->getParam(PARAM_INPUT_SWITCH_ADDRESS)->value();
      input_message2 = request->getParam(PARAM_INPUT_SWITCH_STATE)->value();
      if (input_message2.startsWith("1"))
      {
        Device1.system_flags.system_state_flag = SYSTEM_ON;
      }
      else if (input_message2.startsWith("0"))
      {
        Device1.system_flags.system_state_flag = SYSTEM_OFF;
      }
      xQueueSend(handle_state_msg_queue, (void *)&Device1.queue_messages.state_message, 0);
    }
    else 
    {
      input_message1 = "No message sent";
      input_message2 = "No message sent";
    }
#ifdef DEBUG_ON
    Serial.print("from: ");
    Serial.print(input_message1);
    Serial.print(" - value: ");
    Serial.println(input_message2);
#endif
    request->send(200, "text/plain", "OK");
  });
  server.begin();
  // if(init_WiFi()) 
  // {
  // }
  // else 
  // {
  //   // Connect to Wi-Fi network with SSID and password
  //   Serial.println("Setting AP (Access Point)");
  //   // NULL sets an open Access Point
  //   WiFi.softAP("ESP-WIFI-MANAGER", NULL);

  //   IPAddress IP = WiFi.softAPIP();
  //   Serial.print("AP IP address: ");
  //   Serial.println(IP); 

  //   // Web Server Root URL
  //   server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
  //     request->send(SPIFFS, "/wifimanager.html", "text/html");
  //   });
    
  //   server.serveStatic("/", SPIFFS, "/");
    
  //   server.begin();
  // }
}

void loop() 
{

}
