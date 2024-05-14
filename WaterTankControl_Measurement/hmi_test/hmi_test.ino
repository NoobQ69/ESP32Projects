#include <SPI.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735

typedef struct
{
  int tank_id;
  unsigned int current_volume;
  unsigned int upper_limit;
  unsigned int lower_limit;
  unsigned int capacity;
  bool state;
  bool protection;
  bool mode;
  bool leakage;

} Water_Tank_t;

Water_Tank_t Tank1[8];
int Tank_position = 0;

#define TFT_CS         5
#define TFT_RST        4
#define TFT_DC         2

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

void hmi_draw_text(const char *text, short x = 0, short y = 0, int text_size = 1, uint16_t color = ST7735_BLACK) 
{
  tft.setCursor(x, y);
  tft.setTextColor(color);
  tft.setTextSize(text_size);
  tft.setTextWrap(true);
  tft.print(text);
}

void hmi_fill_rects(short width, short height, short x = 0, short y = 0, uint16_t color=ST7735_BLACK) 
{
  tft.fillRect(x, y, width, height, color);
}

void hmi_fill_circle(short x = 0, short y = 0, short r=10, uint16_t color=ST7735_BLACK)
{
  tft.fillCircle(x, y, r, color);
}

#define WT_BUTTON_1 13
#define WT_BUTTON_2 14
#define WT_BUTTON_3 15
#define WT_BUTTON_4 19

#define WT_RELAY_PIN 22
#define WT_BUZZER_PIN 33

String convert_num_to_str(int number, int digit_limit = 3)
{
  String temp_str = String(number);
  int l = temp_str.length();
  if (l < digit_limit)
  {
    String num_str = "";
    for (int i = l; i < digit_limit; i++)
    {
      num_str += "0";
    }
    num_str += temp_str;
    return num_str;
  }
  return temp_str;
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
  }
}

void buzzer_makes_sound(int delay_time = 20)
{
  digitalWrite(WT_BUZZER_PIN, HIGH);
  delay(delay_time);
  digitalWrite(WT_BUZZER_PIN, LOW);
}

int read_button(int button_pin, bool is_holding = true)
{
  if (digitalRead(button_pin) == LOW)
  {
    delay(20);
    if (digitalRead(button_pin) == LOW)
    {
      if (is_holding)
      {
        while (digitalRead(button_pin) == LOW)
        {
          delay(10);
        }
      }
      return 1;
    }
  }
  return 0;
}

void main_menu()
{
  int x_texts_position = 5, y_texts_position = 55, text_spacing = 10;
  int x_texts_data_position = 85, y_texts_data_position = 55;
  tft.fillScreen(ST77XX_GREY);
  hmi_fill_rects(120, 124, 2, 2, ST7735_WHITE);
  hmi_draw_text("relay", 15, 5);

  String tank_position_str = String(Tank1[Tank_position].tank_id);
  hmi_draw_text(tank_position_str.c_str(), 50, 20, 3);

  hmi_fill_rects(120, 76, 2, 50, ST7735_BLUE);
  hmi_draw_text("curr volume", x_texts_position, y_texts_position, 1, ST7735_WHITE);
  hmi_draw_text("capacity", x_texts_position, y_texts_position+text_spacing, 1, ST7735_WHITE);
  hmi_draw_text("upper", x_texts_position, y_texts_position + text_spacing*2, 1, ST7735_WHITE);
  hmi_draw_text("lower", x_texts_position, y_texts_position + text_spacing*3, 1, ST7735_WHITE);
  hmi_draw_text("protection", x_texts_position, y_texts_position + text_spacing*4, 1, ST7735_WHITE);
  hmi_draw_text("mode", x_texts_position, y_texts_position + text_spacing*5, 1, ST7735_WHITE);
  hmi_draw_text("leakage", x_texts_position, y_texts_position + text_spacing*6, 1, ST7735_WHITE);

  hmi_draw_text(convert_num_to_str(Tank1[Tank_position].current_volume).c_str(), x_texts_data_position, y_texts_data_position, 1, ST7735_WHITE);
  hmi_draw_text(convert_num_to_str(Tank1[Tank_position].capacity).c_str(), x_texts_data_position, y_texts_data_position + text_spacing, 1, ST7735_WHITE);
  hmi_draw_text(convert_num_to_str(Tank1[Tank_position].upper_limit).c_str(), x_texts_data_position, y_texts_data_position + text_spacing*2, 1, ST7735_WHITE);
  hmi_draw_text(convert_num_to_str(Tank1[Tank_position].lower_limit).c_str(), x_texts_data_position, y_texts_data_position + text_spacing*3, 1, ST7735_WHITE);

  String var_str = "";
  if (Tank1[Tank_position].protection == true)
    var_str = "On";
  else
    var_str = "Off";
  hmi_draw_text(var_str.c_str(), x_texts_data_position, y_texts_data_position + text_spacing*4, 1, ST7735_WHITE);

  if (Tank1[Tank_position].mode == true)
    var_str = "Auto";
  else
    var_str = "Manual";
  hmi_draw_text(var_str.c_str(), x_texts_data_position, y_texts_data_position + text_spacing*5, 1, ST7735_WHITE);
  
  if (Tank1[Tank_position].leakage == true)
    var_str = "Yes";
  else
    var_str = "No";
  hmi_draw_text(var_str.c_str(), x_texts_data_position, y_texts_data_position + text_spacing*6, 1, ST7735_WHITE);

  hmi_draw_text("WiFi", 127, 5);
  hmi_draw_text("online", 122, 30);
  hmi_draw_text("load", 127, 60);
  hmi_draw_text("flush", 125, 90);

  hmi_fill_circle(140, 20, 4, ST7735_RED);
  hmi_fill_circle(140, 45, 4, ST7735_RED);
  hmi_fill_circle(140, 80, 4, ST7735_RED);
  hmi_fill_circle(140, 105, 4, ST7735_RED);

  if (Tank1[Tank_position].state == true)
    hmi_fill_circle(90, 30, 4, ST7735_GREEN);
  else
    hmi_fill_circle(90, 30, 4, ST7735_RED);
}

void setup() 
{
  Serial.begin(9600);
  tft.initR(INITR_BLACKTAB);      // Init ST7735S chip, black tab
  // put your setup code here, to run once:
  tft.fillScreen(ST7735_WHITE);
  tft.setRotation(3);

  pinMode(WT_BUTTON_1, INPUT_PULLUP);
  pinMode(WT_BUTTON_2, INPUT_PULLUP);
  pinMode(WT_BUTTON_3, INPUT_PULLUP);
  pinMode(WT_BUTTON_4, INPUT_PULLUP);

  pinMode(WT_RELAY_PIN, OUTPUT);
  pinMode(WT_BUZZER_PIN, OUTPUT);
  
  water_tank_init(Tank1);
  main_menu();
}

void loop() 
{
  // put your main code here, to run repeatedly:
  if (read_button(WT_BUTTON_1))
  {
    buzzer_makes_sound();
    Tank_position = (Tank_position + 1) % 8; 
    // digitalWrite(WT_RELAY_PIN, HIGH);
    main_menu();
    Serial.println("button 1 pushed");
  }
  if (read_button(WT_BUTTON_2))
  {
    buzzer_makes_sound();
    // digitalWrite(WT_RELAY_PIN, LOW);
    if (Tank1[Tank_position].state == true)
    {
      hmi_fill_circle(90, 30, 4, ST7735_RED);
      Tank1[Tank_position].state = false;
    }
    else
    {
      hmi_fill_circle(90, 30, 4, ST7735_GREEN);
      Tank1[Tank_position].state = true;
    }
    Serial.println("button 2 pushed");
  }
  if (read_button(WT_BUTTON_3, false))
  {
    buzzer_makes_sound();
    hmi_fill_circle(140, 80, 4, ST7735_GREEN);
    while (read_button(WT_BUTTON_3, false)) delay(20);
    hmi_fill_circle(140, 80, 4, ST7735_RED);
    // digitalWrite(WT_RELAY_PIN, HIGH);
    Serial.println("button 3 pushed");
  }
  if (read_button(WT_BUTTON_4, false))
  {
    buzzer_makes_sound();
    hmi_fill_circle(140, 105, 4, ST7735_GREEN);
    while (read_button(WT_BUTTON_4, false)) delay(20);
    hmi_fill_circle(140, 105, 4, ST7735_RED);
    // digitalWrite(WT_RELAY_PIN, LOW);
    Serial.println("button 4 pushed");
  }
}
