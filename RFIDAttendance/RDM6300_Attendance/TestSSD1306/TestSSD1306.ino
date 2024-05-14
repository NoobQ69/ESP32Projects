#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Icons.h"

static TimerHandle_t one_shot_timer = NULL;
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define NUMFLAKES     10 // Number of snowflakes in the animation example

enum OLEDState {
  OLED_CONFIG_STATE,
  OLED_ERROR_STATE, 
  OLED_LOADING_STATE,
  OLED_OPERATE_STATE, 
  OLED_SUCCESS_STATE,
  OLED_BEGIN_STATE,
  OLED_LOGO_STATE
};

#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16

void oled128x32_drawBitmap(const unsigned char *bitmapArray, int x = 0, int y = 0, int sizeX=0, int sizeY=0, bool isClearedDisplay=true) {
  if (isClearedDisplay)
    display.clearDisplay();

  display.drawBitmap(x, y, bitmapArray, sizeX, sizeY, 2);
  display.display();
  // delay(1000);
}

void oled128x32_printText(const char *text, int x = 0, int y = 0, int sizeText=1) {
  display.setTextSize(sizeText);
  display.setTextColor(WHITE);
  display.setCursor(x, y);
  display.println(text);
  display.display();  
  // delay(100);
}

void myTimerCallback(TimerHandle_t xTimer) { // Called when one of the timers expires

  // Print message if timer 0 expired
  if ((uint32_t)pvTimerGetTimerID(xTimer) == 0) {
    display.clearDisplay();
    display.display();
    // Serial.println("One-shot timer expired");
  }
}

#define PADDING_X_TEXT 35
#define PADDING_Y_TEXT 14

void oled128x32_printScreen(const char *text, int type)
{
  switch (type)
  {
    case OLED_CONFIG_STATE:
    {
      oled128x32_drawBitmap(epd_bitmap_IconConfig, 10, 10, 20, 20);
      oled128x32_printText(text, PADDING_X_TEXT, PADDING_Y_TEXT);
      break;
    }
    case OLED_ERROR_STATE: 
    {
      oled128x32_drawBitmap(epd_bitmap_IconError, 10, 10, 20, 20, false);
      oled128x32_printText(text, PADDING_X_TEXT, PADDING_Y_TEXT);
      break;
    }
    case OLED_LOADING_STATE:
    {
      oled128x32_drawBitmap(epd_bitmap_IconLoading, 10, 10, 20, 20, false);
      oled128x32_printText(text, PADDING_X_TEXT, PADDING_Y_TEXT);
      break;
    }
    case OLED_OPERATE_STATE: 
    {
      oled128x32_drawBitmap(epd_bitmap_IconOperate, 10, 10, 20, 20);
      oled128x32_printText(text, PADDING_X_TEXT, PADDING_Y_TEXT);
      break;
    }
    case OLED_SUCCESS_STATE: 
    {
      oled128x32_drawBitmap(epd_bitmap_IconSuccess, 10, 10, 20, 20, false);
      oled128x32_printText(text, PADDING_X_TEXT, PADDING_Y_TEXT);
      break;
    }
    case OLED_LOGO_STATE: 
    {
      oled128x32_drawBitmap(epd_bitmap_IconLogo, 10, 4, 25, 25, false);
      oled128x32_printText(text, PADDING_X_TEXT, PADDING_Y_TEXT);
      break;
    }
    default:
    {
      oled128x32_drawBitmap(epd_bitmap_Border, 0, 0, 128, 32);
      oled128x32_printText("Unknow err!", PADDING_X_TEXT, PADDING_Y_TEXT);
      break;
    }
  }
    oled128x32_drawBitmap(epd_bitmap_Border, 0, 0, 128, 32, false);
  // delay(100);
  xTimerStart(one_shot_timer, portMAX_DELAY);
}


void setup() {
  Serial.begin(9600); 
  pinMode(5, INPUT_PULLDOWN);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  one_shot_timer = xTimerCreate(
                    "One-shot timer oled",           // Name of timer
                    30000 / portTICK_PERIOD_MS,  // Period of timer (in ticks)
                    pdFALSE,                    // Auto-reload
                    (void *)0,                  // Timer ID
                    myTimerCallback);           // Callback function
  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();

}

#define BTN_NUMER_OF_COUNT 150
bool readButton(int btn, int holdTimeOn = 0) {
  volatile int holdTimeCount = 0;
  if (digitalRead(btn) == HIGH) {
    vTaskDelay(20/portTICK_PERIOD_MS);
    if (digitalRead(btn) == HIGH) {
      while (digitalRead(btn) == HIGH) {
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

int TempMode = 1;
volatile char PreChangeSetTimeoutBtn = 0, CurChangeSetTimeoutBtn = 0;
void handleSetTimeOutButton() {
  if (digitalRead(5) == HIGH)
  {
    CurChangeSetTimeoutBtn = 1;
    TempMode = 1;
    // system1.attendanceMode = ATTENDANCE_TIMEOUT;
  }
  else if (digitalRead(5) == LOW)
  {
    CurChangeSetTimeoutBtn = 0;
    TempMode = 0;
    // system1.attendanceMode = ATTENDANCE_TIMEIN;
  }
  else {
    CurChangeSetTimeoutBtn = 0;
    TempMode = 0;
    // system1.attendanceMode = ATTENDANCE_TIMEIN;
  }

  if (PreChangeSetTimeoutBtn != CurChangeSetTimeoutBtn)
  {
    PreChangeSetTimeoutBtn = CurChangeSetTimeoutBtn;
    if (TempMode == 1)
    {
      oled128x32_printScreen(" Set in", OLED_OPERATE_STATE);
      // BuzzerMakesSound(BUZZER_SHORT_TIME);
    }
    else
    {
      oled128x32_printScreen(" Set out", OLED_OPERATE_STATE);
      // BuzzerMakesSound(BUZZER_SHORT_TIME);
    }
  }
}

void loop() {
  // digitalWrite(15, HIGH);
  // Serial.println("Hello, ESP32!");
  // delay(500); // this speeds up the simulation
  // digitalWrite(15, LOW);
  // delay(500); // this speeds up the simulation
  // oled12832_drawBitmap(epd_bitmap_Border, 0, 0, 128, 32);    // Draw a small bitmap image
  Serial.println(digitalRead(5));
  handleSetTimeOutButton();
  delay(100);
  // oled128x32_printScreen(" Set in", OLED_OPERATE_STATE);
  // delay(1000);
  // display.clearDisplay();
  // delay(1000);
  // oled128x32_printScreen(" ERROR", OLED_ERROR_STATE);
  // oled12832_drawBitmap(epd_bitmap_IconConfig, 40, 10, 20, 20, false);    // Draw a small bitmap image
  // delay(1000);
  // oled12832_drawBitmap(epd_bitmap_IconError, 40, 10, 20, 20);    // Draw a small bitmap image
  // oled12832_drawBitmap(epd_bitmap_IconLoading, 40, 10, 20, 20);    // Draw a small bitmap image
  // oled12832_drawBitmap(epd_bitmap_IconLogo, 40, 0, 25, 25);    // Draw a small bitmap image
  // oled12832_drawBitmap(epd_bitmap_IconOperate, 40, 10, 20, 20);    // Draw a small bitmap image
  // oled12832_drawBitmap(epd_bitmap_IconSuccess, 40, 10, 20, 20);    // Draw a small bitmap image
}