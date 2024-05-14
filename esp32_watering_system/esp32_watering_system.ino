#include <LiquidCrystal_I2C.h>
#include "SPIFFS.h"
LiquidCrystal_I2C lcd(0x3F,16,2);

const char* upper_path     = "/upper_limit.txt";
const char* lower_path     = "/lower_limit.txt";
const char* current_path     = "/current_limit.txt";

#define SET_BTN_PIN 17
#define INC_BTN_PIN 5
#define DEC_BTN_PIN 18

#define MOISTURE_SENSOR_PIN 33

#define RELAY_PIN 4

enum system_state
{
  S_RUN_STATE,
  S_STOP_STATE,
  S_SET_UPPER_RANGE_STATE,
  S_SET_LOWER_RANGE_STATE,
  S_SET_CURRENT_RANGE_STATE,

};

typedef struct
{
  int upper_limit;
  int lower_limit;
  int current_limit;

} soil_moisture_range_t;

typedef struct 
{
  int state;
  soil_moisture_range_t moisture_range;

} watering_system_t;

watering_system_t System1;


int Previous_percentage_val = 0;
int Is_no_water_flag = 0;
// Initialize SPIFFS
void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

// Read File from SPIFFS
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return String();
  }
  
  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;     
  }
  return fileContent;
}

// Write file to SPIFFS
void writeFile(fs::FS &fs, const char * path, const char * message)
{
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
}

void lcd_display_override(LiquidCrystal_I2C &lcd_i2c, String str, int row, int col, int occupy_range)
{
  String override_string;
  for (int i = 0; i < occupy_range; i++)
  {
    override_string += " ";
  }

  lcd_i2c.setCursor(col, row);
  lcd_i2c.print(override_string.c_str());
  lcd_i2c.setCursor(col, row);
  lcd_i2c.print(str.c_str());
}

bool read_button(int button, void(*callback_func)() = NULL)
{
  if (digitalRead(button) == LOW)
  {
    delay(20);
    if (digitalRead(button) == LOW)
    {
      while (digitalRead(button) == LOW) 
      {
        delay(20);
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

void handle_stop_state()
{
  System1.state = S_RUN_STATE;
  lcd_display_override(lcd, "Trang thai:bat", 0, 0, 16);
}

void handle_states()
{
  if (System1.state == S_RUN_STATE)
  {
    digitalWrite(RELAY_PIN, LOW);
    lcd.clear();
    lcd_display_override(lcd, "Cai dat", 0, 0, 16);
    lcd_display_override(lcd, "Do am duoi:", 1, 0, 10);
    lcd_display_override(lcd, String(System1.moisture_range.lower_limit).c_str(), 1, 12, 10);
    System1.state = S_SET_LOWER_RANGE_STATE;
  }
  else if (System1.state == S_SET_UPPER_RANGE_STATE)
  {
    lcd.clear();
    lcd_display_override(lcd, "Setting", 0, 0, 10);
    lcd_display_override(lcd, "lower limit:", 1, 0, 10);
    lcd_display_override(lcd, String(System1.moisture_range.lower_limit).c_str(), 1, 12, 10);
    System1.state = S_SET_LOWER_RANGE_STATE;
  }
  else if (System1.state == S_SET_LOWER_RANGE_STATE)
  {
    lcd.clear();
    lcd_display_override(lcd, "Cai dat", 0, 0, 10);
    lcd_display_override(lcd, "Do am tren:", 1, 0, 10);
    lcd_display_override(lcd, String(System1.moisture_range.current_limit).c_str(), 1, 14, 10);
    System1.state = S_SET_CURRENT_RANGE_STATE;
  }
  else if (System1.state == S_SET_CURRENT_RANGE_STATE)
  {
    writeFile(SPIFFS, upper_path, String(System1.moisture_range.upper_limit).c_str());
    writeFile(SPIFFS, lower_path, String(System1.moisture_range.lower_limit).c_str());
    writeFile(SPIFFS, current_path, String(System1.moisture_range.current_limit).c_str());
    lcd.clear();
    lcd_display_override(lcd, "Trang thai:tat", 0, 0, 16);
    lcd_display_override(lcd, " ", 1, 0, 16);
    digitalWrite(RELAY_PIN, LOW);
    System1.state = S_STOP_STATE;
  }
  else if (System1.state == S_STOP_STATE)
  {
    lcd.clear();
    lcd_display_override(lcd, "Trang thai:bat", 0, 0, 16);
    lcd_display_override(lcd, " ", 1, 0, 16);
    System1.state = S_RUN_STATE;
  }
}

void handle_upper_mode_inc_btn()
{
  System1.moisture_range.upper_limit++;
  lcd_display_override(lcd, String(System1.moisture_range.upper_limit).c_str(), 1, 12, 4);
}

void handle_upper_mode_dec_btn()
{
  System1.moisture_range.upper_limit--;
  lcd_display_override(lcd, String(System1.moisture_range.upper_limit).c_str(), 1, 12, 4);
}

void handle_lower_mode_inc_btn()
{
  System1.moisture_range.lower_limit++;
  lcd_display_override(lcd, String(System1.moisture_range.lower_limit).c_str(), 1, 12, 4);
}

void handle_lower_mode_dec_btn()
{
  System1.moisture_range.lower_limit--;
  lcd_display_override(lcd, String(System1.moisture_range.lower_limit).c_str(), 1, 12, 4);
}

void handle_current_inc_btn()
{
  System1.moisture_range.current_limit++;
  if (System1.moisture_range.current_limit > System1.moisture_range.upper_limit)
  {
    System1.moisture_range.current_limit = System1.moisture_range.upper_limit;
  }
  lcd_display_override(lcd, String(System1.moisture_range.current_limit).c_str(), 1, 14, 2);
}

void handle_current_dec_btn()
{
  System1.moisture_range.current_limit--;
  if (System1.moisture_range.current_limit < System1.moisture_range.lower_limit)
  {
    System1.moisture_range.current_limit = System1.moisture_range.lower_limit;  
  }
  lcd_display_override(lcd, String(System1.moisture_range.current_limit).c_str(), 1, 14, 2);
}

int time_count = 0;
void control_moisture_lever()
{
  if (System1.state == S_SET_UPPER_RANGE_STATE)
    return;
  int adc_value = analogRead(MOISTURE_SENSOR_PIN);
  int percentage_value = map(adc_value, 0, 4095, 100, 0);
  Previous_percentage_val = percentage_value;  
  
  Serial.print("adc value:");
  Serial.println(adc_value);
  Serial.print("percentage value:");
  Serial.println(percentage_value);

  if (percentage_value < System1.moisture_range.current_limit-2)
  {
    digitalWrite(RELAY_PIN, HIGH);
    adc_value = analogRead(MOISTURE_SENSOR_PIN);
    percentage_value = map(adc_value, 0, 4095, 100, 0);
  }
  else
  {
    digitalWrite(RELAY_PIN, LOW);
  }

  if (time_count > 10)
  {
    // lcd_display_override(lcd, "moisture:", 1, 0, 0);
    lcd_display_override(lcd, "Do am dat:", 1, 0, 0);
    lcd_display_override(lcd, String(percentage_value).c_str(), 1, 11, 3);
    lcd_display_override(lcd, "%", 1, 16, 1);
    time_count = 0;
//    if (percentage_value == Previous_percentage_val)
//    {
//      Is_no_water_flag++;
//      if (Is_no_water_flag > 3)
//      {
//        Is_no_water_flag = 0;
//        digitalWrite(RELAY_PIN, LOW);
//        lcd_display_override(lcd, "Error: no water", 1, 0, 16);
//        delay(5000);
//        lcd.clear();
//        lcd_display_override(lcd, "System is off", 0, 0, 16);
//        lcd_display_override(lcd, " ", 1, 0, 16);
//        System1.state = S_STOP_STATE;
//      }
//    }
  }
  else
  {
    time_count++;
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Hello, ESP32!");

  pinMode(SET_BTN_PIN, INPUT_PULLUP);
  pinMode(INC_BTN_PIN, INPUT_PULLUP);
  pinMode(DEC_BTN_PIN, INPUT_PULLUP);
  digitalWrite(SET_BTN_PIN, HIGH);
  digitalWrite(INC_BTN_PIN, HIGH);
  digitalWrite(DEC_BTN_PIN, HIGH);

  pinMode(MOISTURE_SENSOR_PIN, INPUT);

  pinMode(RELAY_PIN, OUTPUT);

  initSPIFFS();

  lcd.begin();                      // initialize the lcd 
  lcd.backlight();
  // lcd_display_override(lcd, "Hi", 0, 1, 4);
  // lcd.setCursor(0,0);
  // lcd.print("Hello, world!");
  lcd.clear();
  // lcd_display_override(lcd, "System is off", 0, 0, 16);
  lcd_display_override(lcd, "Trang thai:tat", 0, 0, 16);
  lcd_display_override(lcd, " ", 1, 0, 16);
  digitalWrite(RELAY_PIN, LOW);
  System1.state = S_STOP_STATE;
  String get_upper = readFile(SPIFFS, upper_path);
  String get_lower = readFile(SPIFFS, lower_path);
  String get_current = readFile(SPIFFS, current_path);

  Serial.println(get_upper);
  Serial.println(get_lower);
  Serial.println(get_current);
  if (get_upper == "")
  {
   System1.moisture_range.upper_limit = 80; 
  }
  else
  {
    System1.moisture_range.upper_limit = get_upper.toInt();
  }

  if (get_lower == "")
  {
    System1.moisture_range.lower_limit = 20;
  }
  else
  {
    System1.moisture_range.lower_limit = get_lower.toInt();
  }
  
  if (get_current == "")
  {
    System1.moisture_range.current_limit = 50;
  }
  else
  {
    System1.moisture_range.current_limit = get_current.toInt();
  }
}

void loop() 
{
  // put your main code here, to run repeatedly:
  switch (System1.state)
  {
    case S_STOP_STATE:
    {
      read_button(SET_BTN_PIN, handle_stop_state);
      break;
    }
    case S_RUN_STATE:
    {
      read_button(SET_BTN_PIN, handle_states);
      control_moisture_lever();
      break;
    }
    case S_SET_UPPER_RANGE_STATE:
    {
      read_button(SET_BTN_PIN, handle_states);
      read_button(INC_BTN_PIN, handle_upper_mode_inc_btn);
      read_button(DEC_BTN_PIN, handle_upper_mode_dec_btn);
      break;
    }
    case S_SET_LOWER_RANGE_STATE:
    {
      read_button(SET_BTN_PIN, handle_states);
      read_button(INC_BTN_PIN, handle_lower_mode_inc_btn);
      read_button(DEC_BTN_PIN, handle_lower_mode_dec_btn);
      break;
    }
    case S_SET_CURRENT_RANGE_STATE:
    {
      read_button(SET_BTN_PIN, handle_states);
      read_button(INC_BTN_PIN, handle_current_inc_btn);
      read_button(DEC_BTN_PIN, handle_current_dec_btn);
      break;
    }
  }
  delay(100); // this speeds up the simulation
}
