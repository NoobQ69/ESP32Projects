#include "RDM6300MainProject.h"

/// FOR HANDLING ERRORs
void handle_when_task_creation_failed(String failed_message = "")
{
  BuzzerMakesSound(BUZZER_ERROR_READ_WRITE);

#ifdef DEBUG_ON
  Serial.println(failed_message);
#endif
  oled128x32_printScreen(failed_message.c_str(), OLED_ERROR_STATE);
  vTaskDelay(2000/portTICK_PERIOD_MS);
  ESP.restart();
}

/// FOR WIFI SETUP
bool connect_to_WiFi() 
{
  //This line hides the viewing of ESP as wifi hotspot
  SetLED(LED_WIFI, LED_OFF);
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
/// ----------------------------------------------------------------------


/// FOR RDM6300
void handle_logging_data(String file_name, String content)
{
  // APPEND TO FILE WHEN RECEIVING A DATA FROM QUEUE
  // Code here
  file_name += String(time_attendance_system.day) + "_" + String(time_attendance_system.month) + "_" + String(time_attendance_system.year);
  file_name += ".txt";
  File file = SD.open(file_name.c_str(), FILE_WRITE);
  file.close();
  
  file = SD.open(file_name.c_str(), FILE_APPEND);

  if(!file) {
#ifdef DEBUG_ON
    Serial.println("Failed to open data log file");
#endif
  }
  int result;
    
  result = file.println(content);
    
  if(!result) {
#ifdef DEBUG_ON
    Serial.println("Append data log failed");
#endif
  }
  file.close();
  // End code
}

/// ----------------------------------------------------------------------

void my_timer_callback(TimerHandle_t xTimer) 
{ // Called when one of the timers expires

  // Print message if timer 0 expired
  if ((uint32_t)pvTimerGetTimerID(xTimer) == 0) {
    display.clearDisplay();
    display.display();  

    if (system1.system_flags.fault_URL_flag > 0) 
    {
      OLED_PACKAGE pkgCallbackTimer;
      pkgCallbackTimer.content = "Check URL!";
      pkgCallbackTimer.type = OLED_ERROR_STATE;
      xQueueSend(msg_queue, (void *)&pkgCallbackTimer, 0);
    }
    // Serial.println("One-shot timer expired");
  }
  else if ((uint32_t)pvTimerGetTimerID(xTimer) == 1) {
    handleSetTimeOutButton(true);
    // Serial.println("One-shot timer expired");
  }
}

/// FOR CUSTOM WEB PAGE
void handle_get_URL_callback()
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
/// ----------------------------------------------------------------------

// // FOR SENDING TO GOOGLE SHEET
// String byteArrayToString(byte array[], unsigned int length) {
//   char stringArr[length+1]; 
//   for (unsigned int i = 0; i < length; i++)
//   {
//     stringArr[i] = array[i];
//   }
//   stringArr[length] = '\0';
//   return String(stringArr);
// }

String get_value(String data, char separator, int index) 
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

int http_request_to_script_app(String userAttendance, int mode = ATTENDANCE_MODE) 
{
  if (WiFi.status() == WL_CONNECTED) 
  {
    String httpRequestUrl = "";

    httpRequestUrl = Web_App_URL + "?sts=";
    if (mode == ATTENDANCE_MODE) 
    {
      httpRequestUrl += "atc";
    }
    else 
    {
      httpRequestUrl += "reg";
    }
    httpRequestUrl += get_value(userAttendance, ',', 0);
    httpRequestUrl += get_value(userAttendance, ',', 1);
    httpRequestUrl += get_value(userAttendance, ',', 2);

#ifdef DEBUG_ON
    //----------------------------------------Sending HTTP requests to Google Sheets.
    Serial.println();
    Serial.println("-------------");
    Serial.println("Sending request to Google Sheets...");
    Serial.print("URL : ");
    Serial.println(httpRequestUrl);
#endif
    // HTTP GET Request.
    http.begin(httpRequestUrl.c_str());
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

    String sts_Res = get_value(payload, ',', 0);
    //----------------------------------------Conditions that are executed are based on the payload response from Google Sheets (the payload response is set in Google Apps Script).
    if (sts_Res.startsWith("OK")) 
    {
      //..................
      String atcInfo = get_value(payload, ',', 1);
      
      if (atcInfo.startsWith("TI_Successful")) {
        return 200;
      }
      else if (atcInfo.startsWith("TO_Successful")) {
        return 200;
      }
      else if (atcInfo.startsWith("atcErr01")) {
          //handle error
        return 203;
      }
      else if (atcInfo.startsWith("atcErr02") || atcInfo.startsWith("atcErr03")) {
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

/// ----------------------------------------------------------------------

// FOR HANDLE SENDING DATA REQUEST
void handle_sending_data_error()
{
  OLED_PACKAGE pkgFromMain;
  pkgFromMain.content = " Check URL!";
  pkgFromMain.type = OLED_ERROR_STATE;
  xQueueSend(msg_queue, (void *)&pkgFromMain, 0);

  if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
    system1.internet_state = OFFLINE_STATE;
  }
  xSemaphoreGive(mutex);
}

void handle_sending_data()
{
  int stateCode = 0;
  for (int i = 0; i < 3; i++) // loop three time
  {
    stateCode = http_request_to_script_ap);
    if (stateCode != -1) 
    { 
      system1.system_flags.fault_URL_flag = 0;   // reset Send data fault flag so it wouldn't appear
      break;
    }
    else 
    {
      system1.system_flags.fault_URL_flag++;
    }
  }

#ifdef DEBUG_ON
  Serial.print("State code:");
  Serial.println(stateCode);
#endif

  if (stateCode == 200) 
  {
    BuzzerMakesSound(BUZZER_SUCCESS_SEND_RECIEVE);
    QueueString.pop();
  }
  else if (stateCode == 203 || stateCode == 208) 
  {
    BuzzerMakesSound(BUZZER_ERROR_SEND_RECIEVE);
    QueueString.pop();
  }
  else if (stateCode == -1)
  {
    if (appendFile(SD, DataFileName.c_str().c_str(), true)) 
    {
      QueueString.pop();
      NumberOfLines++;
    }
    else 
    {
      system1.system_state = ERROR_STATE; // ERROR CASE 1: if internet is not available and there are some record have been on queue container but
                                          //               SD card is failed to communicate, then the system is on error state.
    }
  }
  else 
  {
    // Save to SD under the name exception.txt
    if (appendFile(SD, ExceptionalFileName.c_str().c_str(), true)) { // In some special case, the system will save the data to exceptional file at which the data
                                                                                      // will be handled by technicians.
      QueueString.pop();
      NumberOfLines++;
    }
  }

  if (system1.system_flags.fault_URL_flag > 12) 
  {
    handle_sending_data_error();
  }
}
/// ----------------------------------------------------------------------

/// MAIN
void second_loop_task(void *parameter)
{
  while (1)
  {
    if (rdm6300.get_new_tag_id())
    {
      uint32_t UID = rdm6300.get_tag_id();
#ifdef DEBUG_ON
		  Serial.println(rdm6300.get_tag_id(), HEX);
#endif
      char outputString[7];
      itoa(UID, outputString, 16);
      
      switch (system1.rfid_reader_mode)
      {
        case MFRC522_READ_MODE:
        {
          String getCurrentUserAttendance = "&uid=";
          getCurrentUserAttendance += String(outputString); //byteArrayToString(buffer, 4);
          if (system1.attendance_mode == ATTENDANCE_TIMEOUT)
          {
            getCurrentUserAttendance += ",&to=";
          } 
          else {
            getCurrentUserAttendance += ",&ti=";
          }
          getCurrentUserAttendance += get_current_time_String_format(time_attendance_system);
          getCurrentUserAttendance += ",&date=";
          getCurrentUserAttendance += get_current_time_String_format(time_attendance_system);

          if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)
          {
            system1.attendance_mode = ATTENDANCE_TIMEIN;

            QueueString.push(getCurrentUserAttendance);
          }
          xSemaphoreGive(mutex);

          handle_logging_data(DATA_LOG_FILENAME, getCurrentUserAttendance);

          OLED_PACKAGE pkgFromReader;
          pkgFromReader.content = "Success!";
          pkgFromReader.type = OLED_SUCCESS_STATE;
          xQueueSend(msg_queue, (void *)&pkgFromReader, 0);
          xTimerStart(one_shot_timer_oled_state, portMAX_DELAY);

          BuzzerMakesSound(BUZZER_SUCCESS_READ_WRITE);

          break;
        }
        case MFRC522_WRITE_MODE:
        {
		      // display.println(rdm6300.get_tag_id(), HEX);
          OLED_PACKAGE pkgFromReader;
          pkgFromReader.content = String(outputString);
          pkgFromReader.type = OLED_LOADING_STATE;
          xQueueSend(msg_queue, (void *)&pkgFromReader, 0);

//           getCurrentUserAttendance += ",&number=";
//           getCurrentUserAttendance += String(RDM6300RegNumber);

//           handle_logging_data(REGISTER_LOG_FILENAME, getCurrentUserAttendance);

//           BuzzerMakesSound(BUZZER_SUCCESS_READ_WRITE);
// #ifdef DEBUG_ON
// 		      Serial.println("Writing successfully");
// #endif

          break;
        }
        default:
        {
          system1.rfid_reader_mode = MFRC522_READ_MODE;
        }
      }

	    while (rdm6300.get_tag_id())
      {
        vTaskDelay(100/portTICK_PERIOD_MS);
      }
      // digitalWrite(READ_LED_PIN, rdm6300.get_tag_id());
    }
    else
    {
      vTaskDelay(100/portTICK_PERIOD_MS);
    }
  }
}

#define BTN_NUMER_OF_COUNT 150

bool read_button(int btn, int holdTimeOn = 0) {
  volatile int holdTimeCount = 0;
  if (digitalRead(btn) == HIGH) 
  {
    vTaskDelay(20/portTICK_PERIOD_MS);
    if (digitalRead(btn) == HIGH) 
    {
      while (digitalRead(btn) == HIGH) 
      {
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

volatile char PreChangeSetTimeoutBtn = 0, CurChangeSetTimeoutBtn = 0;

void handleSetTimeOutButton(bool isInvoked = false) 
{
  int btnState = digitalRead(SETTIMEOUT_BTN);
  if (btnState == HIGH)
  {
    CurChangeSetTimeoutBtn = 1;
    system1.attendance_mode = ATTENDANCE_TIMEOUT;
  }
  else if (btnState == LOW)
  {
    CurChangeSetTimeoutBtn = 0;
    system1.attendance_mode = ATTENDANCE_TIMEIN;
  }
  else 
  {
    CurChangeSetTimeoutBtn = 0;
    system1.attendance_mode = ATTENDANCE_TIMEIN;
  }

  if (isInvoked) {
    CurChangeSetTimeoutBtn = (CurChangeSetTimeoutBtn == 1) ? 0 : 1;
  }
  
  OLED_PACKAGE pkgFromBtn;
  if (PreChangeSetTimeoutBtn != CurChangeSetTimeoutBtn)
  {
    PreChangeSetTimeoutBtn = CurChangeSetTimeoutBtn;
    if (system1.attendance_mode == ATTENDANCE_TIMEIN)
    {
      pkgFromBtn.content = " SET IN";
    }
    else
    {
      pkgFromBtn.content = " SET OUT";
    }

    pkgFromBtn.type = OLED_OPERATE_STATE;
    BuzzerMakesSound(BUZZER_SHORT_TIME);

    xQueueSend(msg_queue, (void *)&pkgFromBtn, 0);    
  }
}

char SwitchTo = 0;
void buttons_loop_task(void *parameter)
{
  while (1) 
  {
    if (read_button(RESET_WIFI_BTN, 1))
    {
      OLED_PACKAGE pkgFromBtn;
      pkgFromBtn.content = " Deleting wifi...";
      pkgFromBtn.type = OLED_LOADING_STATE;
      xQueueSend(msg_queue, (void *)&pkgFromBtn, 0);

      wm.resetSettings();

      xTimerStart(one_shot_timer_oled_state, portMAX_DELAY);

      SetLED(LED_WIFI, LED_ON);
      BuzzerMakesSound(BUZZER_SHORT_TIME);
      SetLED(LED_WIFI, LED_OFF);
    }

    if (read_button(SETCARD_BTN))
    {
      SwitchTo = (SwitchTo + 1) % 2;
      if (SwitchTo == 1)
      {
        create_custom_webpage();
        system1.wifi_mode = SWITCH_AP_MODE_STATE;
        system1.rfid_reader_mode = MFRC522_WRITE_MODE;
        // wm.startConfigPortal(AP__NAME,AP__PASS);
      }
      else
      {
        // system1.wifi_mode = RESET_MODE_STATE;
        wm.setAbort(true);
        system1.wifi_mode = STA_MODE_STATE;
        system1.rfid_reader_mode = MFRC522_READ_MODE;
        // wm.resetSettings();
      }

      BuzzerMakesSound(BUZZER_SHORT_TIME);
    }
    handleSetTimeOutButton();
    vTaskDelay(100/portTICK_PERIOD_MS);
  }
}

/*
  CHECK INTERNET STATE TASK
  Description: this task will check internet state of the system
*/

void internet_state_loop_task(void *parameter)
{
  while (true)
  {
    if (system1.system_state == OPERATION_STATE)
    {
      if (WiFi.status() != WL_CONNECTED) 
      {
        system1.internet_state = OFFLINE_STATE;
        connect_to_WiFi();
        continue;
      }

      if (xQueueReceive(invoke_queue, (void *)&system1.system_flags.invoke_internet_flag, 0) == pdTRUE)
      {
        system1.system_flags.on_time_online_flag = 299;
      }

      if (system1.internet_state == ONLINE_STATE) 
      {
        system1.system_flags.on_time_online_flag++;
        if (system1.system_flags.on_time_online_flag > 299) // we sum system1.system_flags.on_time_online_flag each 1 second so 5*60 = 300 second = 5 minutes
        {
          if(Ping.ping("www.google.com")) 
          {
            system1.system_flags.on_time_online_flag = 0;
#ifdef DEBUG_ON
            Serial.println("Success! Internet is available.");
#endif
          }
          else
          {
            system1.internet_state = OFFLINE_STATE;
            SetLED(LED_WIFI, LED_OFF);
#ifdef DEBUG_ON
            Serial.println("Failed! Internet is not available.");
#endif
          }
        }
      }
      else 
      {
        if (WiFi.status() == WL_CONNECTED) 
        {
          if(Ping.ping("www.google.com")) 
          {
            system1.system_flags.on_time_online_flag = 0;
            system1.internet_state = ONLINE_STATE;
            SetLED(LED_WIFI, LED_ON);
#ifdef DEBUG_ON
            Serial.println("Success! Internet is available.");
#endif
          }
          else
          {
#ifdef DEBUG_ON
            Serial.println("Failed! Internet is not available.");
#endif
          }
        }
      }
    }
  //   else if (system1.system_state == ERROR_STATE) 
  //   {
  // #ifdef DEBUG_ON
  //     Serial.println("Error: there is something wrong with the system. Need to reconfigure or reset!");
  // #endif
  //     if (CheckIsOnlineInternet()) {
  //       system1.system_state = OPERATION_STATE;
  //       if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
  //         system1.internet_state = ONLINE_STATE;
  //       }
  //       xSemaphoreGive(mutex);
  //     }
  //   }
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
}

void oled_loop_task(void *parameter) 
{
  OLED_PACKAGE pkg;
  while (1) {
    if (xQueueReceive(msg_queue, (void *)&pkg, 0) == pdTRUE)
    {
      oled128x32_printScreen(pkg.content.c_str(), pkg.type);
    }
    // Wait for trying again
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void get_date_time_loop_task(void *parameter) 
{
  while (1)
  {
    if (system1.rtc_module_connection == RTC_MODULE_SUCCESS) 
    {
      if (!get_date_time(time_attendance_system, GET_RTC_TIME)) 
      {
        for (int i = 0; i < 3; i++)
        {
          vTaskDelay(500 / portTICK_PERIOD_MS);

          if (!get_date_time(time_attendance_system, GET_RTC_TIME))
          {
            system1.system_flags.rtc_flag += 1;
            if (system1.system_flags.rtc_flag > 2) system1.rtc_module_connection = RTC_MODULE_FAILED;
          }
          else
          {
            system1.system_flags.rtc_flag = 0;
          }
        }
      }
    }
    else 
    {
      if (!get_date_time(time_attendance_system, GET_NTP_TIME)) 
      {
        system1.system_flags.ntp_flag++;
        if (system1.system_flags.ntp_flag > 4)
        {
          system1.system_state = ERROR_STATE;  //ERROR CASE 2: if internet is not available and rtc module is failed, then the system is on error state.
          
          OLED_PACKAGE pkgFromDate;
          pkgFromDate.content = " Get time failed";
          pkgFromDate.type = OLED_ERROR_STATE;
          xQueueSend(msg_queue, (void *)&pkgFromDate, 0);
        }
      }
      else
      {
        system1.system_flags.ntp_flag = 0;
      }
    }

#ifdef DEBUG_ON
    print_date_time(time_attendance_system);
#endif
    vTaskDelay(2000/portTICK_PERIOD_MS);
  }
}

void setup() 
{
  // put your setup code here, to run once:
  char peripheralsState[2];
#ifdef DEBUG_ON
  Serial.begin(115200);  // Initialize serial communications with the PC
#endif
  system1.system_state           = OPERATION_STATE;
  system1.internet_state         = OFFLINE_STATE;
  system1.rfid_reader_mode        = MFRC522_READ_MODE;
  system1.attendance_mode        = ATTENDANCE_TIMEIN;
  system1.internet_connection    = INTERNET_NOT_AVAILABLE;
  system1.sdcard_connection      = SD_CARD_FAILED;
  system1.rtc_module_connection   = RTC_MODULE_FAILED;
  system1.card_reader_connection  = CARD_READER_FAILED;
  system1.system_flags.fault_URL_flag        = 0;
  system1.system_flags.fault_URL_flag = 0;
  system1.system_flags.ntp_flag = 0;
  system1.system_flags.rtc_flag = 0;
  system1.system_flags.on_time_online_flag = 0;
  system1.system_flags.invoke_internet_flag = 0;

  pinMode(RESET_WIFI_BTN, INPUT);
  pinMode(SETCARD_BTN, INPUT);
  pinMode(SETTIMEOUT_BTN, INPUT);
  pinMode(SD_CS_PIN, OUTPUT);  // SS
  pinMode(LED_WIFI, OUTPUT);  
  // pinMode(LED_CARD, OUTPUT); 
  pinMode(BUZZER, OUTPUT);
  
  SetLED(LED_WIFI, LED_OFF);
    // Create a one-shot timer
  one_shot_timer = xTimerCreate(
                      "One-shot timer oled",           // Name of timer
                      30000 / portTICK_PERIOD_MS,  // Period of timer (in ticks)
                      pdFALSE,                    // Auto-reload
                      (void *)0,                  // Timer ID
                      my_timer_callback);           // Callback function

  if (one_shot_timer == NULL)
  {
    oled128x32_printScreen(" Soft timer err!", OLED_ERROR_STATE);
    vTaskDelay(2000/portTICK_PERIOD_MS);
#ifdef DEBUG_ON
    Serial.println(F("Failed to create timer"));
#endif
  }

  one_shot_timer_oled_state = xTimerCreate(
                      "One-shot timer oled state",           // Name of timer
                      1000 / portTICK_PERIOD_MS,  // Period of timer (in ticks)
                      pdFALSE,                    // Auto-reload
                      (void *)1,                  // Timer ID
                      my_timer_callback);           // Callback function

  if (one_shot_timer_oled_state == NULL)
  {
    oled128x32_printScreen(" Timer oled state err!", OLED_ERROR_STATE);
    vTaskDelay(2000/portTICK_PERIOD_MS);
#ifdef DEBUG_ON
    Serial.println(F("Failed to create oled timer state"));
#endif
  }
  // SetLED(LED_CARD, LED_OFF);

  /// OLED 128x32 INITIALIZATION
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {     // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
#ifdef DEBUG_ON
    Serial.println(F("SSD1306 allocation failed"));
#endif
  }
  // vTaskDelay(2000/portTICK_PERIOD_MS);
  display.clearDisplay();
  oled128x32_printScreen(" STRAWBERRY", OLED_LOGO_STATE);
  vTaskDelay(2000/portTICK_PERIOD_MS);
  oled128x32_printScreen(" loading...", OLED_LOADING_STATE);


  BuzzerMakesSound(BUZZER_POWER_ON);
  Serial.println(F("Pass"));

  /// EEPROM INITIALIZATION
  if (!EEPROM.begin(EEPROM_SIZE)) {
#ifdef DEBUG_ON
    Serial.println("Failed to initialise EEPROM");
    Serial.println("Restarting...");
#endif
    BuzzerMakesSound(BUZZER_ERROR_READ_WRITE);
    oled128x32_printScreen(" EEPROM err!", OLED_ERROR_STATE);
    vTaskDelay(2000/portTICK_PERIOD_MS);
    ESP.restart();
  }

  Web_App_URL = EEPROM.readString(0); // read saved Web App URL at address 0
  wm.setWebAppURL(Web_App_URL);
#ifdef DEBUG_ON
  Serial.print("URL:"); 
  Serial.println(Web_App_URL); 
#endif
  /// ------------------------------------------------

  /// RD6300 INITIALIZATION
  rdm6300.begin(RDM6300_RX_PIN);
  /// ------------------------------------------------

  /// SD INITIALIZATION
  sdSPI2.begin(SD_CLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN); // Initiate SPI2 bus
  if (SD.begin(SD_CS_PIN, sdSPI2)) 
  {
    // deleteFile(SD, "/data.txt");
    system1.sdcard_connection = SD_CARD_SUCCESS;
    File file = SD.open("/data.txt");
    peripheralsState[2] = 1;

    if(!file) 
    {
#ifdef DEBUG_ON
      Serial.println("File doesn't exist");
      Serial.println("Creating file...");
#endif
    }
    else 
    {
#ifdef DEBUG_ON
      Serial.println("File already exists");  
#endif
    }
    file.close();
    getSDCardInformations();
    readFile(SD, DataFileName.c_str(), NumberOfLines);
  }
  else 
  {
#ifdef DEBUG_ON
    Serial.println("Card Mount Failed");
#endif
    // oled128x32_printScreen(" SD err!", OLED_ERROR_STATE);
    vTaskDelay(2000/portTICK_PERIOD_MS);
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
  mutex_internet_state = xSemaphoreCreateMutex();

  /// SECOND LOOP INITIALIZATION
  if (!freeRTOS_task_create_pinned_to_core(second_loop_task, "Task 1", 8192, NULL, 1, &task_1_handler, app_cpu))
  {
    handle_when_task_creation_failed("Task reader failed!");
  }
  /// ------------------------------------------------

  /// WIFI INITIALIZATION
  bool res = false;
  create_custom_webpage();

  while (!res) 
  {
    if(!wm.autoConnect(AP__NAME, AP__PASS))
    { // password protected ap
      BlinkLED(LED_WIFI, 1, 500);
      // for (int i = 0; i < 5; i++) 
      // {
      //   if (check_is_online_internet())
      //   {
      //     res = true;
      //     break;
      //   }
      // }
      system1.wifi_mode = AP_MODE_STATE;
#ifdef DEBUG_ON
      Serial.println("Internet is not available!");
#endif
    }
  }
  SetLED(LED_WIFI, LED_ON);
  system1.wifi_mode = STA_MODE_STATE;
  system1.internet_state = ONLINE_STATE;
  /// ------------------------------------------------

  /// RTC INITIALIZATION
  int remainNumber = 5;
  while (get_ntp_date_time(time_attendance_system) == false)
  {
    remainNumber--;
    vTaskDelay(2000/portTICK_PERIOD_MS);
    
    if (remainNumber == 0)
    {
      oled128x32_printScreen(" RTC err!", OLED_ERROR_STATE);
      vTaskDelay(2000/portTICK_PERIOD_MS);

#ifdef DEBUG_ON
      Serial.println("RTC connection failed!");
#endif
      ESP.restart();
    }
  }
  
  remainNumber = 3;
  String dateSet = convert_to_rtc_date_init_format(time_attendance_system.month, time_attendance_system.day, time_attendance_system.year);
  String timeSet = get_current_time_String_format(time_attendance_system);

  while(!rtc_init(dateSet,timeSet) && remainNumber--)
  {
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }

  if (remainNumber > 0) 
  {
    peripheralsState[0] = 1;

#ifdef DEBUG_ON
      Serial.print("Remain number:");
      Serial.println(remainNumber);
      Serial.println("RTC connects successfully!");
#endif

    system1.rtc_module_connection = RTC_MODULE_SUCCESS;
  }
  else 
  {
    oled128x32_printScreen(" RTC err!", OLED_ERROR_STATE);
    vTaskDelay(2000/portTICK_PERIOD_MS);

#ifdef DEBUG_ON
      Serial.println("RTC connection failed!");
#endif
  }
  /// ------------------------------------------------

  /// THIRD LOOP INITIALIZATION
  if (!freeRTOS_task_create_pinned_to_core(buttons_loop_task, "Task 2", 4096, NULL, 2, &task_2_handler, app_cpu))
  {
    handle_when_task_creation_failed("Task reader failed!");
  }
  /// ------------------------------------------------

  /// FOURTH LOOP INITIALIZATION
  if (!freeRTOS_task_create_pinned_to_core(internet_state_loop_task, "Task 3", 4096, NULL, 2, &task_3_handler, app_cpu))
  {
    handle_when_task_creation_failed("Task itn failed!");
  }
  // Create queue
  msg_queue = xQueueCreate(msg_queue_len, sizeof(OLED_PACKAGE));
  invoke_queue = xQueueCreate(invoke_queue_len, sizeof(char));
  /// FIFTH LOOP INITIALIZATION
  if (!freeRTOS_task_create_pinned_to_core(oled_loop_task, "Task 4", 4096, NULL, 2, &task_4_handler, app_cpu))
  {
    handle_when_task_creation_failed("Task oled failed!");
  }
  /// ------------------------------------------------

  /// SIXTH LOOP TASK INITIALIZATION
  if (!freeRTOS_task_create_pinned_to_core(get_date_time_loop_task, "Task 5", 2048, NULL, 2, &task_5_handler, app_cpu))
  {
    handle_when_task_creation_failed("Task time failed!");
  }
  /// ------------------------------------------------
  for (int i = 0; i < 2; i++) 
  {
    if (peripheralsState[i] == 1)
    {
      buzzerOscillates(100, i+1);
    }
    vTaskDelay(500/portTICK_PERIOD_MS);
  }

  oled128x32_printScreen(" Complete!", OLED_SUCCESS_STATE);
  vTaskDelay(2000/portTICK_PERIOD_MS);
  handleSetTimeOutButton();
}

void loop() 
{
  // put your main code here, to run repeatedly:
  int currentTime = millis();
  if (currentTime - PreviousTime > TIME_INTERVAL) 
  {
    PreviousTime = millis();
    
    if (system1.wifi_mode == SWITCH_AP_MODE_STATE) 
    {
      OLED_PACKAGE pkgFromMain;
      pkgFromMain.content = "AP Mode";
      pkgFromMain.type = OLED_CONFIG_STATE;
      xQueueSend(msg_queue, (void *)&pkgFromMain, 0);

      wm.startConfigPortal(AP__NAME, AP__PASS);

      system1.system_flags.fault_URL_flag = 0;  // After exit to AP mode (the users probably changed the URL link so we reset the fault flag variable)
      system1.rfid_reader_mode = MFRC522_READ_MODE;
      handleSetTimeOutButton(true);

      SwitchTo = 0;                               // Need to put here to reset flag for btn
      system1.wifi_mode = STA_MODE_STATE;
      // --------------------------------------
    }
  }

  if (currentTime - PreviousTime2 >= HALF_SECOND) 
  {
    PreviousTime2 = millis();
    int count = 0;
    if (system1.system_state == OPERATION_STATE)
    {
      if (xSemaphoreTake(mutex, 0) == pdTRUE)
      {
        count = QueueString.count();
      }
      xSemaphoreGive(mutex);
      // Serial.println("end here 1");
      
      if (count > 0) 
      {
     = "empty";

        if (xSemaphoreTake(mutex, 0) == pdTRUE) 
        {
       = QueueString.peek();
      .trim();
        }
        xSemaphoreGive(mutex);

#ifdef DEBUG_ON
        Serial.print("Get from queue:");
        Serial.printl);
#endif

        if != "empty" | != "") 
        {
          if (system1.internet_state == ONLINE_STATE) 
          {
            handle_sending_data();
          }
          else 
          {
            if (appendFile(SD, DataFileName.c_str().c_str(), true)) 
            {
              QueueString.pop();
              NumberOfLines++;
              BuzzerMakesSound(BUZZER_SUCCESS_SEND_RECIEVE);
            }
          }
        }
        else 
        {
          QueueString.pop();
        }
      }

      if (system1.internet_state == ONLINE_STATE) 
      {
        if (NumberOfLines > 0) 
        {
          // read the first line in file
          if (system1.sdcard_connection == SD_CARD_SUCCESS) {
            if (readFile(SD, DataFileName.c_str(), NumberOfLines)) { // update NumberOfLine if something error occurs
              if (readALineFile(SD, DataFileName.c_str(), 0)) {      // get the first line to push into queue
            .trim();
                
                if != "")
                  QueueString.pus);
                
                if (deleteALineFile(SD,  DataFileName.c_str(), 0)) {                 // delete the first line after reading  
                  NumberOfLines--;                                                  // update number of line remained in the file
                }
              }
            }
            else 
            {
              system1.sdcard_connection = SD_CARD_FAILED;
            }
          } // system1.sdcard_connection == SD_CARD_SUCCESS
        }
      }
    }
    else
    {
      vTaskDelay(500/portTICK_PERIOD_MS);
    }
  }
}
