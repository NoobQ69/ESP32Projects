#include <SPI.h>
#include <MFRC522.h>
#include "WiFi.h"
#include "HTTPClient.h"
#include <NTPtimeESP.h>
#include <RtcDS1302.h>
#include <SPI.h>
#include <SD.h>
#include "SD_Utilities.h"

#define BUTTON_1
#define BUTTON_2
#define LED_1
#define LED_2
#define LED_3

#define SD_CS_PIN 15  // or 5

#define DS1302_CLK 27
#define DS1302_DAT 26
#define DS1302_RST 25

enum TimeInterval {
  TIME_INTERVAL = 200,
  ONE_SECOND = 1000,
  TEN_SECOND = 10000,
  ONE_MINUTE = 60000,
};


#define SS_PIN 16
#define RST_PIN 17

NTPtime NTPch("ch.pool.ntp.org");  // connect to Server NTP
int Hour;
int Minute;
int Second;
int Week;
int Day;
int Month;
int Year;

strDateTime dateTime;
ThreeWire myWire(DS1302_DAT, DS1302_CLK, DS1302_RST);  // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);

const char* ssid = "Sussy_hotspot";
const char* password = "nguyenqu@ng10";

WiFiClient client;
// String URL = "http://192.168.43.96/checkoutRFID/checkoutRFID.php";
// String ProductKey = "0";
// char IsCheckout = '1';
// String timeCheckout = "";
char IsPresent = 0;
int PreviousTime = 0, PreviousTime2 = 0;

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
MFRC522::StatusCode status;        //variable to get card status
byte buffer[30];                   //data transfer buffer (16+2 bytes data+CRC)
byte timebuffer[20];               //data transfer buffer (16+2 bytes data+CRC)
byte size = sizeof(buffer);
uint8_t pageAddr = 0x05;      //In this example we will write/read 16 bytes (page 5,6,7 and 8).
uint8_t pageAddrTime = 0x09;  // write/read 8 bytes (page 9 and 10).
                              //Ultraligth mem = 16 pages. 4 bytes per page.
                              //Pages 0 to 4 are for special functions.

void getTime() {
  dateTime = NTPch.getNTPtime(7.0, 0);  // set múi giờ việt nam thứ 7

  if (dateTime.valid) {
    // NTPch.printDateTime(dateTime); // in ra ngày giờ tháng năm
    Hour = dateTime.hour;      // Gio
    Minute = dateTime.minute;  // Phut
    Second = dateTime.second;  // Giay
    Week = dateTime.dayofWeek;
    Year = dateTime.year;
    Month = dateTime.month;
    Day = dateTime.day;
  }
}

void connectWiFi() {
  WiFi.mode(WIFI_OFF);
  delay(1000);
  //This line hides the viewing of ESP as wifi hotspot
  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
}

bool WriteToTag(byte* buff, byte size, uint8_t pageAddress) {
  for (int i = 0; i < size; i++) {
    //data is writen in blocks of 4 bytes (4 bytes per page)
    status = (MFRC522::StatusCode)mfrc522.MIFARE_Ultralight_Write(pageAddress + i, &buff[i * 4], 4);
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Read() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return false;
    }
  }
  return true;
}

bool ReadFromTag(byte* buff, byte size, uint8_t pageAddress) {
  Serial.println(F("Reading data ... "));
  //data in 4 block is readed at once.
  status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(pageAddress, buff, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }
  return true;
}

String dumpInfo(byte* ar, int len) {
  String uid = "";
  for (int i = 0; i < len; i++) {
    if (ar[i] < 0x10)
      Serial.print(F("0"));
    uid += String(ar[i]);
    Serial.print(ar[i], HEX);
    Serial.print(F(" "));
  }
  Serial.println();
  return uid;
}

// void sendDataToServer()
// {
//   String postData = "productKey=" + String(ProductKey) + "&isCheckout=" + String(IsCheckout) + "&dayCheckout=" + String(timeCheckout);

//   HTTPClient http;
//   http.begin(URL);
//   http.addHeader("Content-Type", "application/x-www-form-urlencoded");

//   int httpCode = http.POST(postData);
//   String payload = http.getString();

//   Serial.print("URL : "); Serial.println(URL);
//   Serial.print("Data: "); Serial.println(postData);
//   Serial.print("httpCode: "); Serial.println(httpCode);
//   Serial.print("payload : "); Serial.println(payload);
//   Serial.println("--------------------------------------------------");
// }

void getSDCardInformations() {
  uint8_t cardType = SD.cardType();
  Serial.print("SD Card Type: ");

  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } 
  else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } 
  else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } 
  else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
}

#define countof(a) (sizeof(a) / sizeof(a[0]))

void printDateTime(const RtcDateTime& dt) {
  char datestring[26];

  snprintf_P(datestring,
             countof(datestring),
             PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
             dt.Month(),
             dt.Day(),
             dt.Year(),
             dt.Hour(),
             dt.Minute(),
             dt.Second());
  Serial.print(datestring);
}

void setup() {
  Serial.begin(9600);  // Initialize serial communications with the PC
  Serial.print("compiled: ");
  Serial.print(__DATE__);
  Serial.println(__TIME__);

  Rtc.Begin();

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  printDateTime(compiled);
  Serial.println();

  if (!Rtc.IsDateTimeValid()) {
    // Common Causes:
    //    1) first time you ran and the device wasn't running yet
    //    2) the battery on the device is low or even missing

    Serial.println("RTC lost confidence in the DateTime!");
    Rtc.SetDateTime(compiled);
  }

  if (Rtc.GetIsWriteProtected()) {
    Serial.println("RTC was write protected, enabling writing now");
    Rtc.SetIsWriteProtected(false);
  }

  if (!Rtc.GetIsRunning()) {
    Serial.println("RTC was not actively running, starting now");
    Rtc.SetIsRunning(true);
  }

  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled) {
    Serial.println("RTC is older than compile time!  (Updating DateTime)");
    Rtc.SetDateTime(compiled);
  } else if (now > compiled) {
    Serial.println("RTC is newer than compile time. (this is expected)");
  } else if (now == compiled) {
    Serial.println("RTC is the same as compile time! (not expected but all is fine)");
  }

  SPI.begin();         // Init SPI bus
  mfrc522.PCD_Init();  // Init MFRC522 card
  Serial.println(F("Sketch has been started!"));
  memcpy(buffer, "Product checked:", 16);
  Serial.println("Serial communication started.");

  pinMode(SD_CS_PIN, OUTPUT);  // SS
  SPIClass SPI2(HSPI);

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Card Mount Failed");
    return;
  }

  getSDCardInformations();
  connectWiFi();
}

void loop() {
  int currentTime = millis();
  if (currentTime - PreviousTime > TIME_INTERVAL) {
    PreviousTime = millis();
    if (WiFi.status() != WL_CONNECTED) {
      connectWiFi();
    }

    // Look for new cards
    if (!mfrc522.PICC_IsNewCardPresent()) {
      IsPresent = 0;
      return;
    }

    if (IsPresent == 1) {
      return;
    }
    // Select one of the cards
    if (!mfrc522.PICC_ReadCardSerial())
      return;

    // Write data **********************************************
    getTime();
    String timeCheckout = "\'";
    timeCheckout += String(Hour) + ":" + String(Minute) + ":" + String(Second);
    timeCheckout += String(Day) + "/" + String(Month) + "/" + String(Year);
    timeCheckout += "\'";
    memcpy(timebuffer, timeCheckout.c_str(), 20);
    WriteToTag(buffer, size, pageAddr);
    WriteToTag(timebuffer, 8, 9);


    Serial.println(F("MIFARE_Ultralight_Write() OK "));
    Serial.println();
    String ProductKey = dumpInfo(mfrc522.uid.uidByte, mfrc522.uid.size);

    writeFile(SD, "/hello.txt", ProductKey.c_str());
    // Read data ***************************************************
    ReadFromTag(buffer, size, pageAddr);
    ReadFromTag(timebuffer, 20, 9);

    Serial.print(F("Readed data: "));
    //Dump a byte array to Serial
    readFile(SD, "/hello.txt");
    for (byte i = 0; i < 16; i++) {
      Serial.write(buffer[i]);
    }
    for (byte i = 0; i < 20; i++) {
      Serial.write(timebuffer[i]);
    }
    Serial.println();

    mfrc522.PICC_HaltA();
    IsPresent = 1;
  }
  if (currentTime - PreviousTime2 >= ONE_SECOND) {
    PreviousTime2 = millis();
    RtcDateTime now = Rtc.GetDateTime();
    getTime();

    Serial.print(Hour);
    Serial.print(" ");
    Serial.print(Minute);
    Serial.print(Second);
    Serial.print(" ");
    Serial.print(Day);
    Serial.print("/");
    Serial.print(Month);
    Serial.print("/");
    Serial.print(Year);
    Serial.println();

    printDateTime(now);
    Serial.println();

    if (!now.IsValid()) {
      // Common Causes:
      //    1) the battery on the device is low or even missing and the power line was disconnected
      Serial.println("RTC lost confidence in the DateTime!");
    }
  }
}