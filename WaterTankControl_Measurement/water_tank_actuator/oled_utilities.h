#ifndef OLED_UTILITIES
#define OLED_UTILITIES

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define PADDING_X_TEXT 35
#define PADDING_Y_TEXT 14

#define PADDING_X_IMG 10
#define PADDING_Y_IMG 6

enum oled_mode
{
  OLED_MONITOR,
  OLED_MENU,
  OLED_SETTING,
  OLED_INFO,
};

typedef struct 
{
  int oled_mode;
  int enter_btn;
  int back_btn;
  int up_btn;
  int down_btn;

} buttons_gui_t;

buttons_gui_t Buttons_1;

typedef struct 
{
  String content;
  int type;

} OLED_PACKAGE;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/// FOR SSD1306 128x32
void oled128x32_drawBitmap(const unsigned char *bitmap_array, int x = 0, int y = 0, int size_x=0, int size_y=0, bool is_cleared_display=true) 
{
  if (is_cleared_display == true)
    display.clearDisplay();

  display.drawBitmap(x, y, bitmap_array, size_x, size_y, 2);
  display.display();
}

void oled128x32_printText(const char *text, int x = 0, int y = 0, int size_text=1) 
{
  display.setTextSize(size_text);
  display.setTextColor(WHITE);
  display.setCursor(x, y);
  display.println(text);
  display.display();
}


//void oled128x32_printScreen(const char *text, int type, int size_text=1)
//{
//  switch (type)
//  {
//    case OLED_CONFIG_STATE:
//    {
//      oled128x32_drawBitmap(epd_bitmap_IconConfig, PADDING_X_IMG, PADDING_Y_IMG, 20, 20);
//      oled128x32_printText(text, PADDING_X_TEXT, PADDING_Y_TEXT, size_text);
//      break;
//    }
//    case OLED_ERROR_STATE: 
//    {
//      oled128x32_drawBitmap(epd_bitmap_IconError, PADDING_X_IMG, PADDING_Y_IMG, 20, 20);
//      oled128x32_printText(text, PADDING_X_TEXT, PADDING_Y_TEXT, size_text);
//      break;
//    }
//    case OLED_LOADING_STATE:
//    {
//      oled128x32_drawBitmap(epd_bitmap_IconLoading, PADDING_X_IMG, PADDING_Y_IMG, 20, 20);
//      oled128x32_printText(text, PADDING_X_TEXT, PADDING_Y_TEXT, size_text);
//      break;
//    }
//    case OLED_OPERATE_STATE: 
//    {
//      oled128x32_drawBitmap(epd_bitmap_IconOperate, PADDING_X_IMG, PADDING_Y_IMG, 20, 20);
//      oled128x32_printText(text, PADDING_X_TEXT, PADDING_Y_TEXT, size_text);
//      break;
//    }
//    case OLED_SUCCESS_STATE: 
//    {
//      oled128x32_drawBitmap(epd_bitmap_IconSuccess, PADDING_X_IMG, PADDING_Y_IMG, 20, 20);
//      oled128x32_printText(text, PADDING_X_TEXT, PADDING_Y_TEXT, size_text);
//      break;
//    }
//    case OLED_LOGO_STATE: 
//    {
//      oled128x32_drawBitmap(epd_bitmap_IconLogo, PADDING_X_IMG, 4, 25, 25);
//      oled128x32_printText(text, PADDING_X_TEXT, PADDING_Y_TEXT, size_text);
//      break;
//    }
//    default:
//    {
//      oled128x32_drawBitmap(epd_bitmap_Border, 0, 0, 128, 32);
//      oled128x32_printText("Unknow err!", PADDING_X_TEXT, PADDING_Y_TEXT);
//      break;
//    }
//  }
//  oled128x32_drawBitmap(epd_bitmap_Border, 0, 0, 128, 32, false);
////  xTimerStart(one_shot_timer, portMAX_DELAY);
//}

/// ----------------------------------------------------------------------

#endif