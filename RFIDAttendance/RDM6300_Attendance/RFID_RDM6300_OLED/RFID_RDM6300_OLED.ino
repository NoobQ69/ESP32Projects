// #include <Arduino.h>
#include <rdm6300.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define RDM6300_RX_PIN 16  // read the SoftwareSerial doc above! may need to change this pin to 10...

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 32  // OLED display height, in pixels
Rdm6300 rdm6300;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
  Serial.begin(115200);

  rdm6300.begin(RDM6300_RX_PIN);

  Serial.println("\nPlace RFID tag near the rdm6300...");

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  delay(2000); //d:\ArduinoWorkSpace\libraries\Adafruit_BMP085_Library
  display.clearDisplay(); //d:\ArduinoWorkSpace\libraries\Adafruit_BMP085_Library
}

void loop() {
  /* get_new_tag_id returns the tag_id of a "new" near tag,
	following calls will return 0 as long as the same tag is kept near. */
  if (rdm6300.get_new_tag_id()) {
    Serial.println(rdm6300.get_tag_id(), HEX);
    display.clearDisplay();       // xóa sạch màn hình
    display.setTextSize(2);       // kích thước văn bản
    display.setTextColor(WHITE);  // màu văn bản, chỉ có một màu
    display.setCursor(0, 10);     // tọa độ hiện thị (x,y)
    // Display static text
    display.println(rdm6300.get_tag_id(), HEX);  // in ra nội dung văn bản
    display.display();                           // bắt đầu in
    delay(10);
  }
}
