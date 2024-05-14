//tim wifi có tên ESPxxxxxxx ko có pass (xx là ký tự số)
// nhập 192.168.4.1 để vào tìm wifi và tiến hành cài đặt mới.
//#include <ESP8266WiFi.h>          //https://github.com/esp8266/Ardu
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFiManager.h>    //https://github.com/tzapu/WiFiManager (đây là link gốc, còn muốn làm giống mình thì adđ thư viện mình việt hóa trong file đính kèm)
#define nutbam_wifi_moi 35  //Chính là chân GPIO4 có ký hiệu là D2 trên bo mạch. nối qua trở 1 k->nút bấm->GND.

void xoawificu() {
  if (digitalRead(nutbam_wifi_moi) == LOW)  // nếu nút bấm xóa wifi cũ đc bấm
  {
    {
      while (digitalRead(nutbam_wifi_moi) == LOW)
        ;  // chừng nào nut bấm_wifi_mới vẫn đc giữ
      {

        Serial.println(digitalRead(nutbam_wifi_moi));  // in giá trị nút bấm ra màn hình 0 or 1
        WiFiManager wifiManager;                       // gọi bộ nhớ wifi cũ

        wifiManager.resetSettings();  // xóa bộ nhớ wifi cũ AUTO QUAY VỀ Void setup do đó ko chạy lệnh dưới
      }
    }
  }
}

void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());                       // in ra địa chỉ wifi
  Serial.println(myWiFiManager->getConfigPortalSSID());  //in ra tên wifi sẽ phát ra
}

void setup() {
  pinMode(nutbam_wifi_moi, INPUT);

  digitalWrite(nutbam_wifi_moi, 1);

  Serial.begin(115200);
  WiFiManager wifiManager;

  if (!wifiManager.autoConnect("www", "12345678")) {
    Serial.println("KẾT NỐI THẤT BẠI ĐANG RESET ESP CHỜ.....");

    delay(800);
  }
  Serial.println("KẾT NỐI THÀNH CÔNG:)");  //Nếu kết nối wifi thành công, in thông báo ra màn hình
}

void loop() {
  xoawificu();
}
