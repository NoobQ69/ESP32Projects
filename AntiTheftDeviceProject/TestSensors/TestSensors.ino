#include "main.h"

device_system_t Device1;
// email_data_t Email_container_1;

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// Declaration for SSD1306 display connected using I2C
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C

#define LED_PIN 2

int on_off = 0;
int hour = 0, minute = 0;
String email = "losCartos6@gmail.com";
// Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
// Declaration for SSD1306 display connected using software SPI:
//#define OLED_MOSI   9
//#define OLED_CLK   10
//#define OLED_DC    11
//#define OLED_CS    12
//#define OLED_RESET 13
//Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
bool read_button(int btn, void (*callback_func)() = NULL) 
{
  volatile int holdTimeCount = 0;
  if (digitalRead(btn) == LOW) 
  {
    vTaskDelay(20/portTICK_PERIOD_MS);
    if (digitalRead(btn) == LOW) 
    {
      while (digitalRead(btn) == LOW) 
      {
        vTaskDelay(20/portTICK_PERIOD_MS);
      }
      Serial.println("Button pushed");
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
  if (on_off == 0)
  {
    oled128x32_printText("System OFF", 0, 20, 2);
  }
  else
  {
    oled128x32_printText("System ON", 0, 20, 2);
  }
  oled128x32_printText("10:20", 10, 38, 1);
  oled128x32_printText("WiFi", 10, 50, 1);
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
  oled128x32_printText(String(hour).c_str(), 14, 14, 1);
  oled128x32_printText(String(minute).c_str(), 64, 14, 1);
  oled128x32_printText(String(hour).c_str(), 14, 48, 1);
  oled128x32_printText(String(minute).c_str(), 64, 48, 1);
  // *******************************************
  oled128x32_printText("Time on", 0, 0, 1);
  oled128x32_printText("Time off", 0, 32, 1);
  display.clearDisplay();
}

void update_info_mode()
{
  oled128x32_printText("Email:", 0, 0, 1);
  oled128x32_printText(email.c_str(), 0, 20, 1);
  oled128x32_printText("Recipient:", 0, 40, 1);
  oled128x32_printText(email.c_str(), 0, 50, 1);
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
    on_off = 0;
  }
  else if (Buttons_1.enter_btn == 1)
  {
    //*****************************
    on_off = 1;
  }
  update_monitor_mode();
}

void handle_back_monitor()
{
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
    hour++;
    if (hour > 23) hour = 0;
  }
  else if (Buttons_1.enter_btn == 1)
  {
    minute++;
    if (minute > 59) minute = 0;
  }
  update_setting_mode();
}
void handle_down_setting()
{
  if (Buttons_1.enter_btn == 0)
  {
    hour--;
    if (hour < 0) hour = 23;
  }
  else if (Buttons_1.enter_btn == 1)
  {
    minute--;
    if (minute < 0) minute = 59;
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
      // read_button(BACK_BUTTON, handle_back_menu);
      break;
    }
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(ENTER_BUTTON, INPUT_PULLUP);
  pinMode(BACK_BUTTON, INPUT_PULLUP);
  pinMode(UP_BUTTON, INPUT_PULLUP);
  pinMode(DOWN_BUTTON, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  // digitalWrite(LED_PIN, HIGH);
  // initialize the OLED object
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (1)
    {
      delay(500);
    }
  }
  Buttons_1.oled_mode = 0;
  // Uncomment this if you are using SPI
  //if(!display.begin(SSD1306_SWITCHCAPVCC)) {
  //  Serial.println(F("SSD1306 allocation failed"));
  //  for(;;); // Don't proceed, loop forever
  //}
  // Clear the buffer.
  display.clearDisplay();
  // Display Text
  // display.setTextSize(1.5);
  // display.setTextColor(WHITE);
  // display.setCursor(0,28);
  // display.println("Este proyecto");
  // display.println("es muy interesante");
  
  // display.display();
  // delay(2000);
  // display.clearDisplay();
  // // Display Inverted Text
  // display.setTextColor(BLACK, WHITE); // 'inverted' text
  // display.setCursor(0,28);
  // display.println("Hello world!");
  // display.display();
  // delay(2000);
  // display.clearDisplay();
  // // Changing Font Size
  // display.setTextColor(WHITE);
  // display.setCursor(0,24);
  // display.setTextSize(2);
  // display.println("Hello!");
  // display.display();
  // delay(2000);
  // display.clearDisplay();
  // // Display Numbers
  // display.setTextSize(1);
  // display.setCursor(0,28);
  // display.println(123456789);
  // display.display();
  // delay(2000);
  // display.clearDisplay();
  // // Specifying Base For Numbers
  // display.setCursor(0,28);
  // display.print("0x"); display.print(0xFF, HEX); 
  // display.print("(HEX) = ");
  // display.print(0xFF, DEC);
  // display.println("(DEC)"); 
  // display.display();
  // delay(2000);
  // display.clearDisplay();
  // // Display ASCII Characters
  // display.setCursor(0,24);
  // display.setTextSize(2);
  // display.write(3);
  // display.display();
  // delay(2000);
  // display.clearDisplay();
  // // Scroll full screen
  // display.setCursor(0,0);
  // display.setTextSize(1);
  // display.println("Full");
  // display.println("screen");
  // display.println("scrolling!");
  // display.display();
  // display.startscrollright(0x00, 0x07);
  // delay(2000);
  // display.stopscroll();
  // delay(1000);
  // display.startscrollleft(0x00, 0x07);
  // delay(2000);
  // display.stopscroll();
  // delay(1000);    
  // display.startscrolldiagright(0x00, 0x07);
  // delay(2000);
  // display.startscrolldiagleft(0x00, 0x07);
  // delay(2000);
  // display.stopscroll();
  // display.clearDisplay();
  // // Scroll part of the screen
  // display.setCursor(0,0);
  // display.setTextSize(1);
  // display.println("Scroll");
  // display.println("some part");
  // display.println("of the screen.");
  // display.display();
  // display.startscrollright(0x00, 0x00);
}


void loop() {
  // put your main code here, to run repeatedly:
  handle_modes_oled();
}
