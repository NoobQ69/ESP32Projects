#include "main.h"

Water_Tank_t TankSystem[NUMBER_OF_TANK];
Water_consumtion_t Water_logger[8];
short relay_pins[8] = {P0, P2, P4, P6, P1, P3, P5, P7};
short Number_of_operate_tank = 0;

String Result = "";
String Msg_str = "";
String Ss_data_str = "ok,get_sensor,";
String Logging_sumary_str = "";
// String Request_data = "";
// tank1/setting/st1pr1m0c100u200l12e0
// tank2/setting/st1pr1m0c100u100l10e0
// tank2/setting/st1pr0
// tank2/setting/st1
// tank2/cmd/0 (on load)
// tank2/cmd/1 (off load)
// tank2/cmd/2 (on flush)
// tank2/cmd/3 (off flush)

// tank2/calib/up
// tank2/calib/low
// get_sensor/0/0
// register_tank/1,2,3,4
// #define DEBUG_ON

HX710B air_press_1(SCK1_PIN, SDI1_PIN);
HX710B air_press_2(SCK2_PIN, SDI2_PIN);
HX710B air_press_3(SCK3_PIN, SDI3_PIN);
HX710B air_press_4(SCK4_PIN, SDI4_PIN);
// HX710B air_press_5(SCK5_PIN, SDI5_PIN);
// HX710B air_press_6(SCK6_PIN, SDI6_PIN);
// HX710B air_press_7(SCK7_PIN, SDI7_PIN);
// HX710B air_press_8(SCK8_PIN, SDI8_PIN);

// I2C device found at address 0x20
// I2C device found at address 0x26
// I2C device found at address 0x3C
PCF8574 pcf8574(0x26);
PCF8574 pcf8574_input(0x20);

void water_tank_init(Water_Tank_t tank[])
{
  for (int i = 0; i < 8; i++)
  {
    tank[i].tank_id = i+1;
    tank[i].current_volume = 0;
    tank[i].upper_limit = 100+i;
    tank[i].lower_limit = 10+i;
    tank[i].capacity = 100+i;
    tank[i].protection = true;
    tank[i].state = WT_OFF_STATE;
    tank[i].mode = false;
    tank[i].leakage = false;
    tank[i].is_calibrated_flag = false;
    tank[i].overflow_flag = false;
  }
}

void water_tank_init_single(Water_Tank_t tank, int i)
{
  tank.tank_id = i;
  tank.current_volume = 0;
  tank.upper_limit = 100;
  tank.lower_limit = 10;
  tank.capacity = 100;
  tank.protection = true;
  tank.state = WT_OFF_STATE;
  tank.mode = false;
  tank.leakage = false;
  tank.is_calibrated_flag = false;
  tank.overflow_flag = false;
}


void water_tank_print_params(Water_Tank_t &tank)
{
  Serial.println();
  Serial.print("-TANK:");
  Serial.println(tank.tank_id);
  Serial.println();
  Serial.print("-curr:");
  Serial.println(tank.current_volume);
  Serial.print("-up:");
  Serial.println(tank.upper_limit );
  Serial.print("-up raw:");
  Serial.println(tank.upper_limit_raw);
  Serial.println();
  Serial.print("-low:");
  Serial.println(tank.lower_limit);
  Serial.print("-low raw:");
  Serial.println(tank.lower_limit_raw);
  Serial.print("-cap:");
  Serial.println(tank.capacity );
  Serial.print("-pro:");
  Serial.println(tank.protection);
  Serial.println();
  Serial.print("-state:");
  Serial.println(tank.state );
  Serial.print("-mode:");
  Serial.println(tank.mode);
  Serial.print("-leak:");
  Serial.println(tank.leakage );
  Serial.print("-calib:");
  Serial.println(tank.is_calibrated_flag );
}
// tank1/st1pr1m0c100u100l10e0
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

void handle_register_request()
{
  Result = "get_reg_tank,";
  for (int i = 1; i <= 8; i++)
  {
    if (register_tank[i] == 1)
    {
      Result += String(i);
      Result += ",";
    }
  }
}

void handle_setting_request(Water_Tank_t &tank)
{
  Result = "get_setting_"+String(tank.tank_id)+",st";
  Result += String(tank.state);
  Result += "pr";
  Result += String(tank.protection);
  Result += "m";
  Result += String(tank.mode);
  Result += "x";
  Result += String(tank.is_calibrated_flag);
  Result += "u";
  Result += String(tank.upper_limit);
  Result += "l";
  Result += String(tank.lower_limit);
  Result += "e";
  Result += String(tank.leakage);
}

// bool save_calib_data_to_file(Water_Tank_t tank[])
// {
//   for (int i = 0; i < 8; i++)
//   {
//     Result += String(tank[i].upper_limit_raw);
//     Result += ",";
//     Result += String(tank[i].lower_limit_raw);
//     Result += "&";
//   }

//   Result += "";
//   if (writeFile(SPIFFS, "/calib_tank.txt", Result.c_str()))
//   {
//     Serial.println("success");
//     return true;
//   }

//     Serial.println("failed");
//   return false;
// }

// bool save_setting_data_to_file(Water_Tank_t tank[])
// {
//   for (int i = 0; i < 8; i++)
//   {
//     Result += "st";
//     Result += String(tank[i].state);
//     Result += ",pr";
//     Result += String(tank[i].protection);
//     Result += ",m";
//     Result += String(tank[i].mode);
//     Result += ",u";
//     Result += String(tank[i].upper_limit);
//     Result += ",l";
//     Result += String(tank[i].lower_limit);
//     Result += ",e";
//     Result += String(tank[i].leakage);
//     Result += "&";
//   }
//   if (writeFile(SPIFFS, "/setting_tanks.txt", Result.c_str()))
//   {
//     Serial.println("Success");
//     return true;
//   }
//   Result = "";
//   Serial.println("Failed");
//   return false;
// }

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
  Number_of_operate_tank = 0;
  do
  {
    tank_number_str = split_value(params, ',', i);
    if (tank_number_str != "")
    {
      register_tank[tank_number_str.toInt()] = 1;
      Number_of_operate_tank += 1;
    }
    else
    {
      break;
    }
    // Serial.println(i);
    i++;
  }
  while(tank_number_str != "");
  return true;
}

bool handle_calibration(Water_Tank_t &tank, HX710B &pressure_sensor, String command)
{
  uint32_t data_raw = 0;
  int i = 0;
  while (pressure_sensor.read(&data_raw, 1000UL) != HX710B_OK && i <= 5)
  {
    i++;
    vTaskDelay(100);
  }
  if (i > 5)
  {
    return false;
  }
  if (command.startsWith("up"))
  {
    tank.upper_limit_raw = data_raw;
    if (tank.upper_limit_raw > tank.lower_limit_raw)
    {
      tank.is_calibrated_flag = true;
    }
    else
    {
      return false;
    }
  }
  else if (command.startsWith("low"))
  {
    tank.lower_limit_raw = data_raw;
  }
  return true;
}

void handle_get_sensor_data(bool type = true)
{
  if (type == false)
    Result = "get_sensor_raw,";
  else
    Result = "get_sensor,";


  for (int i = 1; i < NUMBER_OF_TANK+1; i++)
  {
    if (register_tank[i])
    {
      if (type == true)
      {
        Result += String(TankSystem[i-1].current_volume);
      }
      else
      {
        Result += String(TankSystem[i-1].current_volume_raw);
      }
      Result += ",";
    }
  }
  Serial2.println(Result);
}

void handle_get_log_data(Water_Tank_t tank[])
{
  Result = "get_log,";
  for (int i = 1; i <= 8; i++)
  {
    Result += String(Water_logger[i-1].total_value);
    Result += ",";
    Water_logger[i-1].total_value = 0;
  }
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
  
  Result = "ok_tank_all," + params;
}

bool handle_command_single_tank(Water_Tank_t &tank, HX710B &air_pressure_ss, String &command, String &params)
{
  int is_ready = WT_DATA_READY;
  params.trim();
  if (command.startsWith("setting"))
  {
    convert_received_data_setting(params, tank);
    if (tank.mode == true && !tank.is_calibrated_flag)
    {
      tank.state = WT_OFF_STATE;
      tank.mode = false;
      Result = "error_calib_0," + String(tank.tank_id);
      return false;
    }
    // save_setting_data_to_file(TankSystem);
    Result = "ok_setting," + String(tank.tank_id);
    return false;
  }
  else if (command.startsWith("calib"))
  {
    if (!handle_calibration(tank, air_pressure_ss, params))
    {
      Result = "error_calib_" + params + "," + String(tank.tank_id);
    }
    else
    {
      // save_calib_data_to_file(TankSystem);
      Result = "ok_calib_" + params + "," + String(tank.tank_id);
    }
    return false;
  }
  else if (command.startsWith("cmd"))
  {
    Result = command+"/"+params;

    switch (tank.tank_id)
    {
      case 1:
      {
        xQueueSend(tank_1_queue, (void *)&is_ready, 100);
        break; 
      }
      case 2:
      {
        xQueueSend(tank_2_queue, (void *)&is_ready, 100);
        break; 
      }
      case 3:
      {
        xQueueSend(tank_3_queue, (void *)&is_ready, 100);
        break; 
      }
      case 4:
      {
        xQueueSend(tank_4_queue, (void *)&is_ready, 100);
        break; 
      }
      default:
      {
        
      }
    }
  }
  return true;
}

int handle_commands(String data, int type)
{
  Result = "";
  // if (!data.startsWith("tank") && !data.startsWith("get_sensor") && !data.startsWith("get_log")) return 0;
  int is_ready = 0;
  String tank = split_value(data, '/', 0);
  String cmd = split_value(data, '/', 1);
  String params = split_value(data, '/', 2);
  bool is_get_from_queue = false;
#ifdef DEBUG_ON
  Serial.println(tank);
  Serial.println(params);
#endif

  if (tank.startsWith("get_sensor_raw")){ handle_get_sensor_data(false); }
  else if (tank.startsWith("get_sensor"))   { handle_get_sensor_data(); }
  else if (tank.startsWith("get_log"))      { handle_get_log_data(TankSystem); }
  else if (tank.startsWith("get_reg_tank")) { handle_register_request(); }
  else if (tank.startsWith("get_setting"))  { handle_setting_request(TankSystem[cmd.toInt()-1]); }
  else if (tank.startsWith("get_tank_all")) { handle_all_tank(TankSystem, cmd); }
  else if (tank.startsWith("is_ready"))     { Result = "ready";}
  else if (tank.startsWith("register_tank"))
  {
    handle_register_tank(cmd);
    writeFile(SPIFFS, register_tank_path, cmd.c_str());
    writeFile(SPIFFS, num_of_register_tank_path, String(Number_of_operate_tank).c_str());
    ESP.restart();
  }
  else if (tank.startsWith("tank1"))
  {
    if (register_tank[WT_TANK_1+1] == 1)
    { is_get_from_queue = handle_command_single_tank(TankSystem[WT_TANK_1], air_press_1, cmd, params); }
  }
  else if (tank.startsWith("tank2"))
  {
    if (register_tank[WT_TANK_2+1] == 1)
    { is_get_from_queue = handle_command_single_tank(TankSystem[WT_TANK_2], air_press_2, cmd, params); }
  }
  else if (tank.startsWith("tank3"))
  {
    if (register_tank[WT_TANK_3+1] == 1)
    { is_get_from_queue = handle_command_single_tank(TankSystem[WT_TANK_3], air_press_3, cmd, params); }
  }
  else if (tank.startsWith("tank4"))
  {
    if (register_tank[WT_TANK_4+1] == 1)
    { is_get_from_queue = handle_command_single_tank(TankSystem[WT_TANK_4], air_press_4, cmd, params); }
  }
  
  if (is_get_from_queue)
  {
    int timeout = 0;
    while ((xQueueReceive(transfer_data_queue, (void *)&is_ready, 100) != pdTRUE) && timeout < 5)
    {
      timeout++;
    }

    if (timeout >= 5)
      Result = "error_cmd";
  }

  if (type == WT_DASHBOARD_REQUEST)
    Serial2.println("ok,dashboard_req,"    + Result);
  else if (type == WT_DASHBOARD_SENSOR_REQUEST)
    Serial2.println("ok,dashboard_ss_req," + Result);
  else if (type == WT_CONTROLLER_REQUEST)
    Serial2.println("ok,controller_req,"   + Result);
  else if (type == WT_GLOBAL_REQUEST)
    Serial2.println("ok,global_req,"       + Result);
  else if (type == WT_PROMPT_REQUEST)
    Serial2.println("ok,prompt_req,"       + Result);

  return 1;
}

bool get_pressure_sensor_data(Water_Tank_t &tank, HX710B &pressure_sensor)
{
  int time_count = 0;
  while ((pressure_sensor.read(&tank.current_volume_raw, 1000UL) != HX710B_OK ) && time_count < 5)
  {
    time_count++;
  }

  if (time_count >= 5) // handle when sensors error
  {
    return false;
  }

  if (tank.is_calibrated_flag)
  {
    tank.current_volume = map(tank.current_volume_raw, tank.lower_limit_raw, tank.upper_limit_raw, tank.lower_limit, tank.upper_limit);
    if (tank.current_volume > tank.upper_limit) tank.current_volume = tank.upper_limit;
    else if (tank.current_volume < tank.lower_limit) tank.current_volume = tank.lower_limit;
  }
  return true;
}

void handle_manual_mode(Water_Tank_t &tank, HX710B &pressure_sensor, int command)
{
  if (tank.tank_id < 5)
  {
    switch(command)
    {
      case WP_LOAD:
      {
        pcf8574.digitalWrite(relay_pins[tank.tank_id-1], LOW);
        break;
      }
      case WP_OFF_LOAD:
      {
        pcf8574.digitalWrite(relay_pins[tank.tank_id-1], HIGH);
        break;
      }
      case WP_FLUSH:
      {
        if (tank.is_calibrated_flag)
        {
          Water_logger[tank.tank_id-1].previous_value = tank.current_volume;
        }
        pcf8574.digitalWrite(relay_pins[tank.tank_id+3], LOW);
        break;
      }
      case WP_OFF_FLUSH:
      {
        pcf8574.digitalWrite(relay_pins[tank.tank_id+3], HIGH);

        if (tank.is_calibrated_flag)
        {
          Water_logger[tank.tank_id-1].current_value = tank.current_volume;
          Water_logger[tank.tank_id-1].interval_value = Water_logger[tank.tank_id-1].previous_value - Water_logger[tank.tank_id-1].current_value;
          Water_logger[tank.tank_id-1].total_value += Water_logger[tank.tank_id-1].interval_value;
        }

          // writeFile(SPIFFS, "/tank_log.txt", handle_collect_logger(Water_logger).c_str());
        break;
      }
      default:
        for (int i = 0; i < 8; i++) pcf8574.digitalWrite(relay_pins[i], HIGH);
    }
  }
  else
  {

  }
}

bool handle_auto_mode(Water_Tank_t &tank, HX710B &pressure_sensor)
{
  uint32_t data_raw = 0;
  if (pressure_sensor.read(&data_raw, 1000UL) != HX710B_OK )
    return false;
  else
  {
    tank.current_volume = map(data_raw, tank.lower_limit_raw, tank.upper_limit_raw, tank.lower_limit, tank.upper_limit);
    if (tank.overflow_flag == false && tank.current_volume < tank.upper_limit)
    {
      if (tank.tank_id < 4)
      {
        pcf8574.digitalWrite(relay_pins[tank.tank_id-1], LOW);
      }
      else
      {
        // handle when full option
      }
    }
    else
    {
      if (tank.tank_id < 4)
      {
        pcf8574.digitalWrite(relay_pins[tank.tank_id-1], HIGH);
      }
      else
      {
        // handle when full option
      }
    }
  }
  return true;
}

void handle_tank_operation(Water_Tank_t &tank, QueueHandle_t &msg_queue, PCF8574 &float_sensor, PCF8574 &relay_pump, HX710B &air_pressure_sensor)
{
  String cmd, params, result;
  int msg_signal;

  if (xQueueReceive(msg_queue, (void *)&msg_signal, 0) == pdTRUE)
  {
    msg_signal = WT_DATA_READY;
    cmd = split_value(Result, '/', 0);
    params = split_value(Result, '/', 1);

    if (tank.state == WT_ON_STATE) // Tank 1 is on
    {
      if (cmd.startsWith("cmd"))
      {
        handle_manual_mode(tank, air_pressure_sensor, params.toInt());
        Result = "ok,cmd,tank" + String(tank.tank_id);
        xQueueSend(transfer_data_queue, (void *)&msg_signal, 100);
      }
    }
    else
    {
      Result = "error,st_0,tank" + String(tank.tank_id);
      xQueueSend(transfer_data_queue, (void *)&msg_signal, 100);
    }
  }

  if (tank.state == WT_ON_STATE) // Tank 1 is on
  {
    if (tank.mode == true) // Tank 1 is on auto mode
    {
      handle_auto_mode(tank, air_pressure_sensor);
    }

    if (float_sensor.digitalRead(tank.tank_id-1) == HIGH) // overflow water!
    {
      tank.overflow_flag = true;
      if (tank.protection == true) relay_pump.digitalWrite(relay_pins[tank.tank_id-1], HIGH);
    }
    else
    {
      tank.overflow_flag = false;
    }
  }
  else
  {
    relay_pump.digitalWrite(relay_pins[tank.tank_id-1], HIGH);
    relay_pump.digitalWrite(relay_pins[tank.tank_id+3], HIGH);
    vTaskDelay(100/portTICK_PERIOD_MS);
  }
}

void tank_1_task(void *parameters)
{
  while (1)
  {
    handle_tank_operation(TankSystem[WT_TANK_1], tank_1_queue, pcf8574_input, pcf8574, air_press_1);
  }
}

void tank_2_task(void *parameters)
{
  while (1)
  {
    handle_tank_operation(TankSystem[WT_TANK_2], tank_2_queue, pcf8574_input, pcf8574, air_press_2);
  }
}

void tank_3_task(void *parameters)
{
  while (1)
  {
    handle_tank_operation(TankSystem[WT_TANK_3], tank_3_queue, pcf8574_input, pcf8574, air_press_3);
  }
}

void tank_4_task(void *parameters)
{
  while (1)
  {
    handle_tank_operation(TankSystem[WT_TANK_4], tank_4_queue, pcf8574_input, pcf8574, air_press_4);
  }
}

void read_sensor_task(void *parameters)
{
  while (1)
  {
    if (register_tank[1])
    {
      get_pressure_sensor_data(TankSystem[0], air_press_1);
      Serial.print("Tank 1 sensor raw data:");
      Serial.println(TankSystem[0].current_volume_raw);
    }
    if (register_tank[2])
    {
      get_pressure_sensor_data(TankSystem[1], air_press_2);
      Serial.print("Tank 2 sensor raw data:");
      Serial.println(TankSystem[1].current_volume_raw);
    }
    if (register_tank[3])
    {
      get_pressure_sensor_data(TankSystem[2], air_press_3);
      Serial.print("Tank 3 sensor raw data:");
      Serial.println(TankSystem[2].current_volume_raw);
    }
    if (register_tank[4])
    {
      get_pressure_sensor_data(TankSystem[3], air_press_4);
      Serial.print("Tank 4 sensor raw data:");
      Serial.println(TankSystem[3].current_volume_raw);
    }
    vTaskDelay(500);
  }
}

void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
	delay(1000);
  
  String register_tank_str, num_of_reg_tank_str, file_name_str;

  if (initSPIFFS())
  {
    register_tank_str = readFile(SPIFFS, register_tank_path);
    num_of_reg_tank_str = readFile(SPIFFS, num_of_register_tank_path);
  }

  if (register_tank_str == "") handle_register_tank(DEFAULT_REGISTERED_TANK);
  else handle_register_tank(register_tank_str);

  if (num_of_reg_tank_str == "")  Number_of_operate_tank = DEFAULT_NUMBER_OF_REGISTERED_TANK;
  else Number_of_operate_tank = num_of_reg_tank_str.toInt();

  Serial.println("Number of registered tank:" + register_tank_str);
  Serial.println("Registered tank number:"+Number_of_operate_tank);
  // // Serial.println(num_of_reg_tank_str);
  
  // Result = readFile(SPIFFS,"/setting_tanks.txt");
  // Serial.println(Result);

  for (int i = 0; i < 8; i++)
  {
    TankSystem[i].tank_id = i+1;
  }
  // if (Result != "")
  // {
  //   for (int i = 0; i < 8; i++)
  //   {
  //     if (register_tank[i+1] == 1)
  //     {
  //       String single_setting_str = split_value(Result, '&', i);
  //       Serial.println(single_setting_str);
  //       convert_received_data_setting(single_setting_str, TankSystem[i]);
  //     }
  //   }
  // }

  // Result = readFile(SPIFFS,"/calib_tank.txt");
  // if (Result != "")
  // {
  //   for (int i = 0; i < 8; i++)
  //   {
  //     if (register_tank[i+1] == 1)
  //     {
  //       String single_calib_str = split_value(Result, '&', i);
  //       Serial.println(single_calib_str);
        
  //       TankSystem[i].upper_limit_raw = split_value(single_calib_str, ',', 0).toInt();
  //       TankSystem[i].lower_limit_raw = split_value(single_calib_str, ',', 1).toInt();
  //     }
  //   }
  // }

  // Result = "";

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) 
  {
    while (1)
    {
      Serial.println(F("SSD1306 allocation failed"));
      delay(500);
      ESP.restart();
    }
  }
  display.clearDisplay();

	// Set pinMode to OUTPUT
	pcf8574.pinMode(P0, OUTPUT);
	pcf8574.pinMode(P1, OUTPUT);
	pcf8574.pinMode(P2, OUTPUT);
	pcf8574.pinMode(P3, OUTPUT);
	pcf8574.pinMode(P4, OUTPUT);
	pcf8574.pinMode(P5, OUTPUT);
	pcf8574.pinMode(P6, OUTPUT);
	pcf8574.pinMode(P7, OUTPUT);

  for (int i = 0; i < 8; i++)
  {
	  pcf8574.digitalWrite(i, HIGH);
  }

	pcf8574_input.pinMode(P0, INPUT);
	pcf8574_input.pinMode(P1, INPUT);
	pcf8574_input.pinMode(P2, INPUT);
	pcf8574_input.pinMode(P3, INPUT);
	pcf8574_input.pinMode(P4, INPUT);
	pcf8574_input.pinMode(P5, INPUT);
	pcf8574_input.pinMode(P6, INPUT);
	pcf8574_input.pinMode(P7, INPUT);

	Serial.print("Init pcf8574...");
	if (pcf8574.begin())
  {
		Serial.println("Success to initiate external output");
	}
  else
  {
		Serial.println("Failed to set external output pin");
	}

	if (pcf8574_input.begin())
  {
		Serial.println("Success to initiate external input");
	}
  else
  {
		Serial.println("Failed to set external input pin");
	}

  // ******************************************* FOR RTOS INITIALIZATION *********************************************************


  transfer_data_queue  = xQueueCreate(transfer_data_queue_len, sizeof(int));
  
  if (register_tank[1])
  {
    tank_1_queue         = xQueueCreate(tank_1_queue_len, sizeof(int));
    if (!freeRTOS_task_create_pinned_to_core(tank_1_task, "Task 1", 4096, NULL, 2, &task_1_handler, app_cpu))
    {
      handle_when_task_creation_failed("Task tank 1 failed!");
    }
    if (!air_press_1.init())
    { 
      Serial.println(F("HX710B 1 not Found !"));
      print_notify_message("HX710B 1 not Found !", ERROR_TYPE_MSG);
      while(!air_press_1.init()) delay(500);
    }
  }
  if (register_tank[2])
  {
    tank_2_queue         = xQueueCreate(tank_2_queue_len, sizeof(String));
    if (!freeRTOS_task_create_pinned_to_core(tank_2_task, "Task 2", 4096, NULL, 2, &task_2_handler, app_cpu))
    {
      handle_when_task_creation_failed("Task tank 2 failed!");
    }
    if (!air_press_2.init())
    { 
      Serial.println(F("HX710B 2 not Found !"));
      print_notify_message("HX710B 2 not Found !", ERROR_TYPE_MSG);
      while(!air_press_2.init()) delay(500);
    }
  }
  if (register_tank[3])
  {
    tank_3_queue         = xQueueCreate(tank_3_queue_len, sizeof(String));
    if (!freeRTOS_task_create_pinned_to_core(tank_3_task, "Task 3", 4096, NULL, 2, &task_3_handler, app_cpu))
    {
      handle_when_task_creation_failed("Task tank 3 failed!");
    }
    if (!air_press_3.init())
    { 
      Serial.println(F("HX710B 3 not Found !"));
      oled128x32_printText("HX710B 3 not Found !", 20, 20, 1);
      while(!air_press_3.init()) delay(500);
    }
  }
  if (register_tank[4])
  {
    tank_4_queue         = xQueueCreate(tank_3_queue_len, sizeof(String));
    if (!freeRTOS_task_create_pinned_to_core(tank_4_task, "Task 4", 4096, NULL, 2, &task_4_handler, app_cpu))
    {
      handle_when_task_creation_failed("Task tank 4 failed!");
    }
    if (!air_press_4.init())
    { 
      Serial.println(F("HX710B 4 not Found !"));
      oled128x32_printText("HX710B 4 not Found !", 20, 20, 1);
      while(!air_press_4.init()) delay(500);
    }
  }

  if (!freeRTOS_task_create_pinned_to_core(read_sensor_task, "Task 10", 4096, NULL, 2, &sensor_task_handler, app_cpu))
  {
    handle_when_task_creation_failed("Task sensor failed!");
  }
  // ----------------------------------------------------------------------------------------------------------------------------
  // water_tank_init(TankSystem);
  
  // String read_logging_data = read_tank_logger();

  // if (read_logging_data != "")
  // {
  //   for (int i = 0; i < 8; i++)
  //   {
  //     if (register_tank[i] == 1)
  //     {
  //       Water_logger[i].total_value = split_value(read_logging_data, ',', i).toInt();
  //     }
  //   }
  // }
  // Serial.println("--Pass");
  update_tank_menu(Number_of_operate_tank);
  vTaskDelay(1000);
}

void loop() 
{
  String str = "";

  if (Serial2.available() > 0)
  {
    while (Serial2.available() > 0)
    {
      // Msg_str = Serial2.readStringUntil('\n');
      Msg_str = Serial2.readString();
    }

    if (Msg_str.startsWith("controller_req"))
    {
      int i = Msg_str.indexOf('/');
      str = Msg_str.substring(i+1, Msg_str.length());
      handle_commands(str, WT_CONTROLLER_REQUEST);
    }
    else if (Msg_str.startsWith("dashboard_req"))
    {
      int i = Msg_str.indexOf('/');
      str = Msg_str.substring(i+1, Msg_str.length());
      handle_commands(str, WT_DASHBOARD_REQUEST);
    }
    else if (Msg_str.startsWith("dashboard_ss_req"))
    {
      int i = Msg_str.indexOf('/');
      str = Msg_str.substring(i+1, Msg_str.length());
      handle_commands(str, WT_DASHBOARD_SENSOR_REQUEST);
    }
    else if (Msg_str.startsWith("global_req"))
    {
      int i = Msg_str.indexOf('/');
      str = Msg_str.substring(i+1, Msg_str.length());
      handle_commands(str, WT_GLOBAL_REQUEST);
    }
    else if (Msg_str.startsWith("prompt_req"))
    {
      int i = Msg_str.indexOf('/');
      str = Msg_str.substring(i+1, Msg_str.length());
      handle_commands(str, WT_PROMPT_REQUEST);
    }
    else if (Msg_str.startsWith("dump"))
    {
      for(int i = 0; i < 9; i++)
      {
        if (register_tank[i] == 1)
          water_tank_print_params(TankSystem[i-1]);
      }
    }
  }
  vTaskDelay(50/portTICK_PERIOD_MS);
}
