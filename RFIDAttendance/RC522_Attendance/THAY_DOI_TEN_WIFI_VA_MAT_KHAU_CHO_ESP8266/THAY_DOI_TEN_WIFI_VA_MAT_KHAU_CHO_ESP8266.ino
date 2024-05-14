//tim wifi có tên ESPxxxxxxx ko có pass (xx là ký tự số)
// nhập 192.168.4.1 để vào tìm wifi và tiến hành cài đặt mới.
#include <ESP8266WiFi.h>  //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "WiFiManager.h"  //https://github.com/tzapu/WiFiManager (đây là link gốc, còn muốn làm giống mình thì adđ thư viện mình việt hóa trong file đính kèm)

#define ledPinwifi D4
#define nutbam_wifi_moi 4  //Chính là chân GPIO4 có ký hiệu là D2 trên bo mạch. nối qua trở 1 k->nút bấm->GND.

void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());                       // in ra địa chỉ wifi
  Serial.println(myWiFiManager->getConfigPortalSSID());  //in ra tên wifi sẽ phát ra
}

void setup() {
  lednhapnhay();
  pinMode(nutbam_wifi_moi, INPUT);
  pinMode(ledPinwifi, OUTPUT);  // đèn báo wifi khi kết nối thành công
  digitalWrite(nutbam_wifi_moi, HIGH);

  Serial.begin(115200);
  WiFiManager wifiManager;

  wifiManager.setAPCallback(configModeCallback);
  if (!wifiManager.autoConnect()) {
    Serial.println("KẾT NỐI THẤT BẠI ĐANG RESET ESP CHỜ.....");
    ESP.reset();  //Nếu kết nối thất bại, thử kết nối lại bằng cách reset thiết bị
    delay(800);
  }
  Serial.println("KẾT NỐI THÀNH CÔNG:)");  //Nếu kết nối wifi thành công, in thông báo ra màn hình
  digitalWrite(ledPinwifi, LOW);           //đèn led wifi sáng liên tục báo đang kết nối.
}
//===================CHƯƠNG TRÌNH CON XÓA WIFI CŨ VÀ CÀI WIFI MỚI==================================
void xoawificu() {
  if (digitalRead(nutbam_wifi_moi) == LOW)  // nếu nút bấm xóa wifi cũ đc bấm
  {
    {
      while (digitalRead(nutbam_wifi_moi) == LOW)
        ;  // chừng nào nut bấm_wifi_mới vẫn đc giữ
      {

        Serial.println(digitalRead(nutbam_wifi_moi));  // in giá trị nút bấm ra màn hình 0 or 1
        WiFiManager wifiManager;                       // gọi bộ nhớ wifi cũ
        digitalWrite(ledPinwifi, HIGH);
        wifiManager.resetSettings();  // xóa bộ nhớ wifi cũ AUTO QUAY VỀ Void setup do đó ko chạy lệnh dưới
      }
    }
  }
}
//=================== CHƯƠNG TRÌNH CHÍNH LIÊN TỤC KIỂM TRA SẴN SÀNG GỌI LỆNH CÀI WIFI MỚI===========
void loop() {
  xoawificu();
}
//=================led nhấp nháy báo kết nối==================================================
void lednhapnhay() {
  digitalWrite(ledPinwifi, LOW);  // hiệu ứng led wifi nhấp nháy thông báo hãy ấn nút gọi webserver
  delay(400);
  digitalWrite(ledPinwifi, HIGH);
  delay(400);
  digitalWrite(ledPinwifi, LOW);
  delay(400);
  digitalWrite(ledPinwifi, HIGH);
  delay(400);
  digitalWrite(ledPinwifi, LOW);
  delay(400);
  digitalWrite(ledPinwifi, HIGH);
  delay(400);
  digitalWrite(ledPinwifi, LOW);
  delay(400);
  digitalWrite(ledPinwifi, HIGH);
  // cuối lệnh là tắt hẳn...sau đó sẽ đc sáng hẳn lên khi người dùng bấm giữ tại ctr khác
}
