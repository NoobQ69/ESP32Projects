#include "main.h"
// Scanning...
// I2C device found at address 0x20
// I2C device found at address 0x26
// I2C device found at address 0x3C

bool connect_to_WiFi() 
{
  //This line hides the viewing of ESP as wifi hotspot
  // SetLED(LED_WIFI, LED_OFF);
  hmi_update_wifi_state();
#ifdef DEBUG_ON
  Serial.println(wm.getSSID());
  Serial.println(wm.getPass());
  Serial.println("Connecting to WiFi");
#endif
  WiFi.begin(wm.getSSID(), wm.getPass());

  WiFi.mode(WIFI_STA);

  if (WiFi.status() != WL_CONNECTED) 
  {
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

void buzzer_makes_sound(int delay_time = 20)
{
  digitalWrite(WT_BUZZER_PIN, HIGH);
  vTaskDelay(delay_time);
  digitalWrite(WT_BUZZER_PIN, LOW);
}

int read_button(int button_pin, bool is_holding = true, void (*callback_func)() = NULL)
{
  long int counting_time = 0; 
  if (digitalRead(button_pin) == LOW)
  {
    vTaskDelay(20);
    if (digitalRead(button_pin) == LOW)
    {
      if (is_holding)
      {
        while (digitalRead(button_pin) == LOW)
        {
          counting_time++;
          vTaskDelay(20);
          if (counting_time > 200) wm.resetSettings();
        }
      }

      if (callback_func != NULL) callback_func();
      return 1;
    }
  }
  return 0;
}

void handle_get_URL_callback()
{
  Web_App_URL = wm.getWebAppURL();
  // EEPROM.writeString(0, Web_App_URL);
  // EEPROM.commit();
#ifdef DEBUG_ON
  Serial.print("Web URL:");
  Serial.println(Web_App_URL);
  // Serial.println(EEPROM.readString(0));
#endif
  buzzer_makes_sound(200);
  // BuzzerMakesSound(BUZZER_SUCCESS_SEND_RECIEVE);
}

void create_custom_webpage()
{
  wm.setAPStaticIPConfig(IPAddress(192,168,60,3), IPAddress(192,168,1,1), IPAddress(255,255,255,0));
  std::vector<const char *> menu = {"wifi","info","webAppURL", "exit","sep"};
  wm.setMenu(menu); // custom menu, pass vector

  // set custom html menu content, inside menu item custom
  const char* menuhtml = "<form action='/custom' method='get'><button>Sign up ID</button></form><br/>\n";
  wm.setCustomMenuHTML(menuhtml);
  wm.setWebApppUrlFunctionCallback(handle_get_URL_callback);
}

String split_string(String data, char separator, int index) 
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

int http_request_to_script_app(String data_to_send) 
{
  if (WiFi.status() == WL_CONNECTED) 
  {
    String httpRequestUrl = "";
    httpRequestUrl = Web_App_URL + "?tank=";

#ifdef DEBUG_ON
    //----------------------------------------Sending HTTP requests to Google Sheets.
    Serial.println();
    Serial.println("-------------");
    Serial.println("Sending request to Google Sheets...");
    Serial.print("URL : ");
    Serial.println(httpRequestUrl);
#endif
    // HTTP GET Request.
    http.begin(data_to_send.c_str());
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    // Gets the HTTP status code.
    int httpCode = http.GET(); 

#ifdef DEBUG_ON
    Serial.print("HTTP Status Code : ");
    Serial.println(httpCode);
#endif

    // Getting response from google sheet.
    String payload;
    if (httpCode > 0) 
    {
        payload = http.getString();

#ifdef DEBUG_ON
        Serial.println("Payload : " + payload);  
        Serial.println("-------------");
#endif
    }
    http.end();

    String sts_Res = split_string(payload, ',', 0);
    //----------------------------------------Conditions that are executed are based on the payload response from Google Sheets (the payload response is set in Google Apps Script).
    if (sts_Res.startsWith("OK")) 
    {
      //..................
      return 200;
    }
    else
    {
      return 400;
    }
  }
  return -1;
}

void water_tank_init(Water_Tank_t tank[])
{
  for (int i = 0; i < 8; i++)
  {
    tank[i].tank_id = i+1;
    tank[i].current_volume = 50+i;
    tank[i].upper_limit = 100+i;
    tank[i].lower_limit = 10+i;
    tank[i].capacity = 100+i;
    tank[i].protection = true;
    tank[i].state = false;
    tank[i].mode = true;
    tank[i].leakage = false;
    tank[i].is_calibrated_flag = false;
    tank[i].load_state_flag = false;
    tank[i].flush_state_flag = false;
  }
}

void water_tank_init_single(Water_Tank_t tank, int i)
{
  tank.tank_id = i;
  tank.current_volume = 50;
  tank.upper_limit = 100;
  tank.lower_limit = 10;
  tank.capacity = 100;
  tank.protection = true;
  tank.state = false;
  tank.mode = true;
  tank.leakage = false;
  tank.is_calibrated_flag = false;
  tank.load_state_flag = false;
  tank.flush_state_flag = false;
}

// ***********************************  FUNCTION HANDLES MQTT CONNECTION ****************************************

void convert_received_data_setting(String params, Water_Tank_t &tank)
{
  int length = params.length();
  for (int i = 0; i < length; i++)
  {
    if (params.startsWith("st"))
    {
      tank.state = params.substring(2, 3).toInt();
      params = params.substring(3, length);
    }
    else if (params.startsWith("pr"))
    {
      tank.protection = params.substring(2, 3).toInt();
      params = params.substring(3, length);
    }
    else if (params.startsWith("m"))
    {
      tank.mode = params.substring(1, 2).toInt();
      params = params.substring(2, length);
    }
    else if (params.startsWith("x"))
    {
      tank.is_calibrated_flag = params.substring(1, 2).toInt();
      params = params.substring(2, length);
    }
    else if (params.startsWith("u"))
    {
      String upper_val="";
      int j=1;
      while(j < length && (params[j]>='0' && params[j]<='9'))
      {
        upper_val += params[j];
        j++;
      }
      params = params.substring(j, length);
      tank.upper_limit = upper_val.toInt();
    }
    else if (params.startsWith("l"))
    {
      String lower_val="";
      int j=1;
      while(j < length && (params[j]>='0' && params[j]<='9'))
      {
        lower_val += params[j];
        j++;
      }
      params = params.substring(j, length);
      tank.lower_limit = lower_val.toInt();
    }
    else if (params.startsWith("e"))
    {
      tank.leakage = params.substring(1, 2).toInt();
      params = params.substring(2, length);
    }
    else if (params == "")
    {
      break;
    }
    else
    {
      params = params.substring(1, length);
    }
  }
}

bool handle_register_tank(String params)
{
  int i = 0;
  String tank_number_str;
  do
  {
    tank_number_str = split_string(params, ',', i);
    if (tank_number_str != "")
    {
      register_tank[tank_number_str.toInt()] = 1;
    }
    // Serial.println(i);
    i++;
  }
  while(tank_number_str != "");
  Number_of_operate_tank = i-1;
  return true;
}

void handle_all_tank(Water_Tank_t tank[], String params)
{
  if (params.startsWith("1"))
  {
    for (int i = 0; i < NUMBER_OF_TANK+1; i++)
    {
      if (register_tank[i])
      {
        tank[i-1].state = WT_ON_STATE;
      }
    }
  }
  else if (params.startsWith("0"))
  {
    for (int i = 0; i < NUMBER_OF_TANK+1; i++)
    {
      if (register_tank[i])
      {
        tank[i-1].state = WT_OFF_STATE;
      }
    }
  }
}

void handle_get_sensor_controller(String data)
{
  for (int i = 1; i < NUMBER_OF_TANK+1; i++)
  {
    if (register_tank[i] == 1)
    {
      TankSystem[i-1].current_volume = split_string(data, ',', i-1).toInt();
    }
  }
}

void water_tank_print_params(Water_Tank_t &tank)
{
  Serial.println();
  Serial.print("-TANK:");
  Serial.println(tank.tank_id);
  Serial.print(" curr:");
  Serial.print(tank.current_volume);
  Serial.print(" up:");
  Serial.print(tank.upper_limit );
  Serial.print(" up raw:");
  // Serial.print(tank.upper_limit_raw);
  Serial.print(" low:");
  Serial.print(tank.lower_limit);
  Serial.print(" low raw:");
  // Serial.print(tank.lower_limit_raw);
  Serial.print(" cap:");
  Serial.print(tank.capacity );
  Serial.print(" pro:");
  Serial.print(tank.protection);
  Serial.print(" state:");
  Serial.print(tank.state );
  Serial.print(" mode:");
  Serial.print(tank.mode);
  Serial.print(" leak:");
  Serial.print(tank.leakage );
  Serial.print(" calib:");
  Serial.println(tank.is_calibrated_flag);
}

int handle_commands(String data)
{
  int i = 0;

  if (data.startsWith("get_sensor"))
  {
    i = data.indexOf(',');
    handle_get_sensor_controller(data.substring(i+1, data.length()));
  }
  else if (data.startsWith("get_log"))
  {
    i = data.indexOf(',');
    http_request_to_script_app(data.substring(i+1, data.length()));
  }
  else if (data.startsWith("get_setting"))
  {
    // int is_ready = 2;
    // Result = data;
    convert_received_data_setting(split_string(data, ',' , 1), TankSystem[Tank_position-1]);
    hmi_main_menu(TankSystem);
    // xQueueSend(gui_msg_queue, (void *)&is_ready, 100);
    // mqtt_client.publish("esp32_tank_system", data.c_str());
  }
  else if (data.startsWith("ok_tank_all")) 
  { 
    handle_all_tank(TankSystem, split_string(data, ',', 1)); 
  }
  else if (data.startsWith("ok_setting"))
  {

  }
  else if (data.startsWith("ok_calib"))
  {

  }
  else if (data.startsWith("error_calib_0"))
  {
    i = split_string(data, ',', 1).toInt();
    TankSystem[i].state = WT_OFF_STATE;
    TankSystem[i].mode = false;
  }
  else if (data.startsWith("error_calib_up")) {}
  else if (data.startsWith("error_calib_low")){}
  else if (data.startsWith("error_cmd")) {}
  else if (data.startsWith("error,st_0")){}

  return 1;
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Dashboard_str = "";
  for (int i = 0; i < length; i++)
  {
    Dashboard_str += char(payload[i]);
  }
  // handle_commands(Dashboard_str);
  // hmi_main_menu(TankSystem);
  int is_ready = 1;
  xQueueSend(transfer_msg_queue, (void *)&is_ready, 100);

  // mqtt_client.publish("esp32_system_tank_1", get_payload.c_str());
}
// --------------------------------------------------------------------------------------------------------------

bool check_internet_available()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    if (Ping.ping("www.google.com")) return true;
  }
  return false;
}

bool is_sent_log_flag = false;
void rtc_time_loop_task(void *parameters)
{
  while (1)
  {
    if (Time1.rtc_flags.rtc_module_connection == RTC_MODULE_SUCCESS) 
    {
      if (!get_date_time(Time1.time, GET_RTC_TIME))
      {
        for (int i = 0; i < 3; i++)
        {
          vTaskDelay(500 / portTICK_PERIOD_MS);

          if (!get_date_time(Time1.time, GET_RTC_TIME))
          {
            Time1.rtc_flags.rtc_module_flag_error += 1;
            if (Time1.rtc_flags.rtc_module_flag_error > 2) Time1.rtc_flags.rtc_module_connection = RTC_MODULE_FAILED;
          }
          else
          {
            Time1.rtc_flags.rtc_module_flag_error = 0;
          }
        }
      }
    }
    else 
    {
      if (!get_date_time(Time1.time, GET_NTP_TIME)) 
      {
        Time1.rtc_flags.ntp_flag_error++;
        if (Time1.rtc_flags.ntp_flag_error > 4)
        {
          if (!check_internet_available())
          {
            System1.rtc_confidence_flag = false;
          }
        }
      }
      else
      {
        Time1.rtc_flags.ntp_flag_error = 0;
      }
    }
    if (System1.rtc_confidence_flag = true)
    {
      if (Time1.time.hour == 23 && Time1.time.minute == 0)
      {
        if (is_sent_log_flag == false)
        {
          // String get_log_request = "get_log";
          // xQueueSend(transfer_msg_queue, (void *)&get_log_request, 100);
        }
        is_sent_log_flag = true;
      }
      else
      {
        is_sent_log_flag = false;
      }
    }
#ifdef DEBUG_ON
    print_date_time(Time1.time);
#endif
    vTaskDelay(2000/portTICK_PERIOD_MS);
  }
}

void gui_loop_task(void *parameter)
{
  while (1)
  {
        // put your main code here, to run repeatedly:
    if (read_button(WT_BUTTON_1))
    {
      buzzer_makes_sound();
      while (1)
      {
        Tank_position = (Tank_position + 1) % 9;
        if (Tank_position == 0) Tank_position = 1;
        if (register_tank[Tank_position] == 1) break;
      }
      // digitalWrite(WT_RELAY_PIN, HIGH);
      // Serial.println(Tank_position);
      Gui_str = "controller_req/get_setting/"+String(Tank_position);
      int is_ready = 2;
      xQueueSend(transfer_msg_queue, (void *)&is_ready, 100);
 
    }
    if (read_button(WT_BUTTON_2))
    {
      buzzer_makes_sound();
      // digitalWrite(WT_RELAY_PIN, LOW);
      int is_ready = 2;
      if (TankSystem[Tank_position-1].state == WT_ON_STATE)
      {
        hmi_fill_circle(90, 30, 4, ST7735_RED);
        TankSystem[Tank_position-1].state = WT_OFF_STATE;
        Gui_str = "controller_req/tank"; 
        Gui_str += String(Tank_position); 
        Gui_str += "/setting/st0";
      }
      else
      {
        hmi_fill_circle(90, 30, 4, ST7735_GREEN);
        TankSystem[Tank_position-1].state = WT_ON_STATE;
        Gui_str = "controller_req/tank"; 
        Gui_str += String(Tank_position); 
        Gui_str += "/setting/st1";
        // Serial2.println(setting_str);
      }
      xQueueSend(transfer_msg_queue, (void *)&is_ready, 100);
    }
    if (read_button(WT_BUTTON_3))
    {
      int is_ready = 2;
      buzzer_makes_sound();
      if (TankSystem[Tank_position-1].load_state_flag == false)
      {
        hmi_fill_circle(140, 80, 4, ST7735_GREEN);
        TankSystem[Tank_position-1].load_state_flag = true;
        Gui_str = "controller_req/tank"; 
        Gui_str += String(Tank_position); 
        Gui_str += "/cmd/0";
      }
      else
      {
        hmi_fill_circle(140, 80, 4, ST7735_RED);
        TankSystem[Tank_position-1].load_state_flag = false;
        Gui_str = "controller_req/tank"; 
        Gui_str += String(Tank_position); 
        Gui_str += "/cmd/1";
        // Serial2.println(cmd);
      }
      xQueueSend(transfer_msg_queue, (void *)&is_ready, 100);
      // digitalWrite(WT_RELAY_PIN, HIGH);
    }
    if (read_button(WT_BUTTON_4))
    {
      int is_ready = 2;
      buzzer_makes_sound();
      if (TankSystem[Tank_position-1].flush_state_flag == false)
      {
        hmi_fill_circle(140, 105, 4, ST7735_GREEN);
        TankSystem[Tank_position-1].flush_state_flag = true;
        Gui_str = "controller_req/tank"; 
        Gui_str += String(Tank_position); 
        Gui_str += "/cmd/2";
        // Serial2.println(cmd);
      }
      else
      {
        hmi_fill_circle(140, 105, 4, ST7735_RED);
        TankSystem[Tank_position-1].flush_state_flag = false;
        Gui_str = "controller_req/tank"; 
        Gui_str += String(Tank_position); 
        Gui_str += "/cmd/3";
        // Serial2.println(cmd);
      }
      xQueueSend(transfer_msg_queue, (void *)&is_ready, 100);
      // digitalWrite(WT_RELAY_PIN, LOW);
    }
    vTaskDelay(50);
  }
}

void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  
  pinMode(WT_BUTTON_1, INPUT_PULLUP);
  pinMode(WT_BUTTON_2, INPUT_PULLUP);
  pinMode(WT_BUTTON_3, INPUT_PULLUP);
  pinMode(WT_BUTTON_4, INPUT_PULLUP);

  // pinMode(LED_BUILTIN, OUTPUT);
  pinMode(WT_RELAY_PIN, OUTPUT);
  pinMode(WT_BUZZER_PIN, OUTPUT);
  
  hmi_init();
  water_tank_init(TankSystem);
  create_custom_webpage();

  tft.fillScreen(ST7735_WHITE);
  hmi_draw_text("Connect to WiFi...", 5, 50, 1, ST7735_BLACK);
  while (1) 
  {
    if(!wm.autoConnect(AP__NAME, AP__PASS))
    { // password protected ap
#ifdef DEBUG_ON
      Serial.println("WiFi is not available!");
#endif
    }
    else
      break;
  }
  System1.internet_state_flag = true;
  initSPIFFS();

    /// RTC INITIALIZATION
  int remain_number = 5;
  while (get_ntp_date_time(Time1.time) == false)
  {
    remain_number--;
    vTaskDelay(2000/portTICK_PERIOD_MS);
    
    if (remain_number == 0)
    {
      // oled128x32_printScreen(" RTC err!", OLED_ERROR_STATE);
      vTaskDelay(2000/portTICK_PERIOD_MS);

#ifdef DEBUG_ON
      Serial.println("RTC connection failed!");
#endif
      ESP.restart();
    }
  }

  transfer_msg_queue  = xQueueCreate(transfer_msg_queue_len, sizeof(int));
  rtc_msg_queue  = xQueueCreate(rtc_msg_queue_len, sizeof(int));
  gui_msg_queue  = xQueueCreate(gui_msg_queue_len, sizeof(int));

  if (!freeRTOS_task_create_pinned_to_core(rtc_time_loop_task, "Task 1", 4096, NULL, 2, &task_1_handler, app_cpu))
  {
    handle_when_task_creation_failed("Task rtc time failed!");
  }
  if (!freeRTOS_task_create_pinned_to_core(gui_loop_task, "Task 2", 16000, NULL, 2, &task_2_handler, app_cpu))
  {
    handle_when_task_creation_failed("Task gui failed!");
  }

  mqtt_client.setServer(mqtt_server, mqtt_port);
  mqtt_client.setCallback(callback);
  Serial.println("Connect to mqtt... ");
  tft.fillScreen(ST7735_WHITE);
  hmi_draw_text("Connect to MQTT...", 5, 50, 1, ST7735_BLACK);
  
  uint32_t time_start = millis(); 
  System1.mqtt_connection_flag = true;
  while (!mqtt_client.connect(mqtt_id))
  {
    delay(500);
    if (millis() - time_start > MQTT_SERVER_CONNECTING_TIMEOUT) 
    {
      System1.mqtt_connection_flag = false;
      break;
    }
  }

  if (System1.mqtt_connection_flag == true)
  {
    tft.fillScreen(ST7735_WHITE);
    hmi_draw_text("SUCCESS", 5, 20, 3, ST7735_GREEN);
    hmi_draw_text("Connected to", 5, 60, 1, ST7735_BLACK);
    hmi_draw_text("MQTT server!", 5, 70, 1, ST7735_BLACK);
    mqtt_client.subscribe("tank_dashboard");
    Serial.println("Connected to MQTT");
  }
  else
  {
    tft.fillScreen(ST7735_WHITE);
    hmi_draw_text("ERROR", 10, 20, 3, ST7735_RED);
    hmi_draw_text("Failed to connect", 5, 60, 1, ST7735_BLACK);
    hmi_draw_text("MQTT server", 5, 70, 1, ST7735_BLACK);
    Serial.println("Failed to connect MQTT");
  }
  vTaskDelay(3000/portTICK_PERIOD_MS);

  tft.fillScreen(ST7735_WHITE);
  hmi_draw_text("Communicate with", 10, 60, 1, ST7735_BLACK);
  hmi_draw_text("actuator unit ...", 10, 70, 1, ST7735_BLACK);

  // Serial.readString();
  Serial2.println("controller_req/get_reg_tank");
  int time_count = 0;
  while (!Serial2.available() && (time_count < 75))
  {
    vTaskDelay(50);
    time_count++;
  }

  if (time_count >= 50)
  {
    tft.fillScreen(ST7735_WHITE);
    hmi_draw_text("ERROR", 10, 20, 3, ST7735_RED);
    hmi_draw_text("Failed to communicate", 10, 60, 1, ST7735_BLACK);
    hmi_draw_text("actuator unit", 10, 70, 1, ST7735_BLACK);
    vTaskDelay(2000/portTICK_PERIOD_MS);
    digitalWrite(WT_RELAY_PIN, LOW);
    digitalWrite(WT_RELAY_PIN, HIGH);
  }
  else
  {
    String result = Serial2.readString();
    int i = result.indexOf(',', 20);
    result = result.substring(i+1, result.length());
    Serial.println(result);
    handle_register_tank(result);
    Serial.println("Number of register tank:");
    for (int i = 1; i <= 8; i++) Serial.print(register_tank[i]);
    Serial.println();
  }
  for (int i = 1; i <= 8; i++)
  {
    if (register_tank[i] == 1)
    {
      Tank_position = i;
      Serial2.println("controller_req/get_setting/"+String(i));
      break;
    } 
  }
  hmi_main_menu(TankSystem);
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqtt_client.connected()) 
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt_client.connect("arduinoClient")) 
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
      // ... and resubscribe
      mqtt_client.subscribe("tank_dashboard");
    }
    else 
    {
      Serial.print("failed, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      vTaskDelay(5000);
    }
  }
}

void handle_when_timeout()
{
  tft.fillScreen(ST7735_WHITE);
  hmi_draw_text("ERROR", 10, 20, 3, ST7735_RED);
  hmi_draw_text("Failed to communicate", 10, 60, 1, ST7735_BLACK);
  hmi_draw_text("actuator unit", 10, 70, 1, ST7735_BLACK);
  hmi_draw_text(Gui_str.c_str(), 10, 80, 1, ST7735_BLACK);
}

void loop() 
{
  // put your main code here, to run repeatedly:
  if (Serial.available())
  {
    Serial.readString();
    for (int j = 1; j <= 8; j++)
    {
      if (register_tank[j] == 1)
      {
        water_tank_print_params(TankSystem[j-1]);
      }
    }
  }
  String get_str;
  int is_ready;

  Time_count++;
  if (!mqtt_client.connected()) {
    reconnect();
  }
  mqtt_client.loop();
  
  if (WiFi.status() != WL_CONNECTED)
  {
    connect_to_WiFi();
  }
  
  if (xQueueReceive(transfer_msg_queue, (void *)&is_ready, 0) == pdTRUE)
  {
    int time_out = 0;
    Serial.print("Sending: ");
    if (is_ready == 1)
    {
      Serial.println(Dashboard_str);
      Serial2.println(Dashboard_str);
    }
    else if (is_ready == 2)
    {
      Serial.println(Gui_str);
      Serial2.println(Gui_str);
    }
    while (!Serial2.available() && (time_out < 310))
    {
      vTaskDelay(10);
      time_out++;
    }
    Serial.println(time_out);
    if (time_out > 300)
    {
      handle_when_timeout();
    }

  }

  if (Serial2.available())
  {
    get_str = Serial2.readStringUntil('\n');

    Serial.print("Recieved: ");
    Serial.println(get_str);
    
    if (get_str.startsWith("ok,controller_req"))
    {
      int i = get_str.indexOf(',',3);
      handle_commands(get_str.substring(i+1, get_str.length()));
    }
    else if (get_str.startsWith("ok,dashboard_req"))
    {
      mqtt_client.publish("esp32_tank_system", get_str.c_str());
    }
    else if (get_str.startsWith("ok,dashboard_ss_req"))
    {
      mqtt_client.publish("esp32_tank_system", get_str.c_str());
    }
    else if (get_str.startsWith("ok,global_req"))
    {
      int i = get_str.indexOf(',',3);
      handle_commands(get_str.substring(i+1, get_str.length()));
      mqtt_client.publish("esp32_tank_system", get_str.c_str());
    }
    else if (get_str.startsWith("ok,prompt_req"))
    {
      mqtt_client.publish("esp32_tank_system", get_str.c_str());
    }
  }
  else if (Time_count > 300)
  {
    Time_count = 0;
    if (switch_type_data == false)
    {
      Serial2.println("global_req/get_sensor");
      switch_type_data = true;
    }
    else
    {
      Serial2.println("global_req/get_sensor_raw");
      switch_type_data = false;
    }
    
    while (!Serial2.available() && Time_count < 50)
    { 
      Time_count++;
      vTaskDelay(20);
    }

    if (Time_count < 50)
    {
      get_str = Serial2.readStringUntil('\n');
      int i = get_str.indexOf(',',3);
      handle_commands(get_str.substring(i+1, get_str.length()));
      mqtt_client.publish("esp32_tank_system", get_str.c_str());
    }
    Time_count = 0;
  }
  // Serial.println("main loop is running...");
  vTaskDelay(10);
}
