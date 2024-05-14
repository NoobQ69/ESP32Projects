#ifndef HMI_UTILITIES
#define HMI_UTILITIES

#include <SPI.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735

#define TFT_CS         5
#define TFT_RST        4
#define TFT_DC         2

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

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

void hmi_main_menu(Water_Tank_t tank[])
{
  int x_texts_position = 5, y_texts_position = 55, text_spacing = 10;
  int x_texts_data_position = 85, y_texts_data_position = 55;
  tft.fillScreen(ST77XX_GREY);
  hmi_fill_rects(120, 124, 2, 2, ST7735_WHITE);
  hmi_draw_text("Tank", 15, 5);

  String tank_position_str = String(tank[Tank_position-1].tank_id);
  hmi_draw_text(tank_position_str.c_str(), 50, 20, 3);

  hmi_fill_rects(120, 76, 2, 50, ST7735_BLUE);
  hmi_draw_text("curr volume", x_texts_position, y_texts_position, 1, ST7735_WHITE);
  hmi_draw_text("is calib", x_texts_position, y_texts_position+text_spacing, 1, ST7735_WHITE);
  hmi_draw_text("upper", x_texts_position, y_texts_position + text_spacing*2, 1, ST7735_WHITE);
  hmi_draw_text("lower", x_texts_position, y_texts_position + text_spacing*3, 1, ST7735_WHITE);
  hmi_draw_text("protection", x_texts_position, y_texts_position + text_spacing*4, 1, ST7735_WHITE);
  hmi_draw_text("mode", x_texts_position, y_texts_position + text_spacing*5, 1, ST7735_WHITE);
  hmi_draw_text("leakage", x_texts_position, y_texts_position + text_spacing*6, 1, ST7735_WHITE);

  hmi_draw_text(convert_num_to_str(tank[Tank_position-1].current_volume).c_str(), x_texts_data_position, y_texts_data_position, 1, ST7735_WHITE);
  
  if (tank[Tank_position-1].is_calibrated_flag)
    hmi_draw_text("Yes", x_texts_data_position, y_texts_data_position + text_spacing, 1, ST7735_WHITE);
  else
    hmi_draw_text("No", x_texts_data_position, y_texts_data_position + text_spacing, 1, ST7735_WHITE);

  hmi_draw_text(convert_num_to_str(tank[Tank_position-1].upper_limit).c_str(), x_texts_data_position, y_texts_data_position + text_spacing*2, 1, ST7735_WHITE);
  hmi_draw_text(convert_num_to_str(tank[Tank_position-1].lower_limit).c_str(), x_texts_data_position, y_texts_data_position + text_spacing*3, 1, ST7735_WHITE);

  String var_str = "";
  if (tank[Tank_position-1].protection == true)
    var_str = "On";
  else
    var_str = "Off";
  hmi_draw_text(var_str.c_str(), x_texts_data_position, y_texts_data_position + text_spacing*4, 1, ST7735_WHITE);

  if (tank[Tank_position-1].mode == true)
    var_str = "Auto";
  else
    var_str = "Manual";
  hmi_draw_text(var_str.c_str(), x_texts_data_position, y_texts_data_position + text_spacing*5, 1, ST7735_WHITE);
  
  if (tank[Tank_position-1].leakage == true)
    var_str = "Yes";
  else
    var_str = "No";
  hmi_draw_text(var_str.c_str(), x_texts_data_position, y_texts_data_position + text_spacing*6, 1, ST7735_WHITE);

  hmi_draw_text("WiFi", 127, 5);
  hmi_draw_text("online", 122, 30);
  hmi_draw_text("load", 127, 60);
  hmi_draw_text("flush", 125, 90);

  if (System1.internet_state_flag)
    hmi_fill_circle(140, 20, 4, ST7735_GREEN);
  else
    hmi_fill_circle(140, 20, 4, ST7735_RED);
  
  if (System1.mqtt_connection_flag)
    hmi_fill_circle(140, 45, 4, ST7735_GREEN);
  else
    hmi_fill_circle(140, 45, 4, ST7735_RED);

  if (tank[Tank_position-1].load_state_flag == true)
    hmi_fill_circle(140, 80, 4, ST7735_GREEN);
  else
    hmi_fill_circle(140, 80, 4, ST7735_RED);
  
  if (tank[Tank_position-1].flush_state_flag == true)
    hmi_fill_circle(140, 105, 4, ST7735_GREEN);
  else
    hmi_fill_circle(140, 105, 4, ST7735_RED);

  if (tank[Tank_position-1].state == true)
    hmi_fill_circle(90, 30, 4, ST7735_GREEN);
  else
    hmi_fill_circle(90, 30, 4, ST7735_RED);
}

void hmi_update_wifi_state()
{
  if (System1.internet_state_flag)
    hmi_fill_circle(140, 20, 4, ST7735_GREEN);
  else
    hmi_fill_circle(140, 20, 4, ST7735_RED);
}

void hmi_init()
{
  tft.initR(INITR_BLACKTAB);      // Init ST7735S chip, black tab
  // put your setup code here, to run once:
  tft.fillScreen(ST7735_WHITE);
  tft.setRotation(3);
}

#endif