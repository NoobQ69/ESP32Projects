#include <SPI.h>
#include <MFRC522.h>
// #include "WiFi.h";
// #include "HTTPClient.h";
// #include <NTPtimeESP.h>

// NTPtime NTPch("ch.pool.ntp.org");  // connect to Server NTP
int Hour;                          
int Minute;
int Second;
int Week;
int Day;
int Month;
int Year;

// strDateTime dateTime;

#define SS_PIN 16
#define RST_PIN 17

const char *ssid = "Sussy_hotspot";
const char *password = "nguyenqu@ng10";

// WiFiClient client;
String URL = "http://192.168.43.96/checkoutRFID/checkoutRFID.php";
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
MFRC522::StatusCode status;        //variable to get card status

// byte buffer[30];  //data transfer buffer (16+2 bytes data+CRC)
byte timebuffer[20];  //data transfer buffer (16+2 bytes data+CRC)
// byte size = sizeof(buffer);

String ProductKey = "0";
char IsCheckout = '1';
String timeCheckout = "";

uint8_t pageAddr = 0x04;    //In this example we will write/read 16 bytes (page 5,6,7 and 8).
uint8_t pageAddrTime = 0x09; // write/read 8 bytes (page 9 and 10).
                          //Ultraligth mem = 16 pages. 4 bytes per page.
                          //Pages 0 to 4 are for special functions.

// void getTime() {
//   dateTime = NTPch.getNTPtime(7.0, 0);  // set múi giờ việt nam thứ 7
  
//   if (dateTime.valid) {
//     // NTPch.printDateTime(dateTime); // in ra ngày giờ tháng năm
//     Hour = dateTime.hour;    // Gio
//     Minute = dateTime.minute;    // Phut
//     Second = dateTime.second;   // Giay
//     Week = dateTime.dayofWeek;
//     Year = dateTime.year;
//     Month = dateTime.month;
//     Day = dateTime.day;
//   }
// }

// void connectWiFi() {
//   WiFi.mode(WIFI_OFF);
//   delay(1000);
//   //This line hides the viewing of ESP as wifi hotspot
//   WiFi.mode(WIFI_STA);
  
//   WiFi.begin(ssid, password);
//   Serial.println("Connecting to WiFi");
  
//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//     Serial.print(".");
//   }
// }

bool WriteToTag(byte* buff, byte size, uint8_t pageAddress)
{
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

bool ReadFromTag(byte* buff, byte size, uint8_t pageAddress)
{
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

String dumpInfo(byte *ar, int len) {
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

MFRC522::MIFARE_Key key;

void setup() {
  Serial.begin(9600);  // Initialize serial communications with the PC
  SPI.begin();         // Init SPI bus
  mfrc522.PCD_Init();  // Init MFRC522 card
  Serial.println(F("Sketch has been started!"));
  // memcpy(buffer, "Product checked:", 16);
  // connectWiFi();
  for (byte i = 0; i < 6; i++) {
      key.keyByte[i] = 0xFF;
  }
}

void loop() {
  // if(WiFi.status() != WL_CONNECTED) { 
  //   connectWiFi();
  // }
  // Look for new cards
  if (!mfrc522.PICC_IsNewCardPresent())
    return;

  // Select one of the cards
  if(!mfrc522.PICC_ReadCardSerial())
    return;

  // Write data **********************************************
  // getTime();
  // timeCheckout = "\'";
  // timeCheckout += String(Hour) + ":" + String(Minute) + ":" + String(Second);
  // timeCheckout += String(Day) + "/" + String(Month) + "/" + String(Year);
  // timeCheckout += "\'";
  // memcpy(timebuffer, timeCheckout.c_str(), 20);
    // Read data ***************************************************
    byte dataBlock[]    = {
        0x01, 0x03, 0x05, 0x07, //  1,  2,   3,  4,
        0x05, 0x06, 0x07, 0x08, //  5,  6,   7,  8,
        0x09, 0x0a, 0xfc, 0x0b, //  9, 10, 255, 11,
        0x0c, 0x0d, 0xee, 0x0f  // 12, 13, 14, 15
    };
  byte trailerBlock  = 7;
  // byte trailerBlock   = 7;
  MFRC522::StatusCode status;
  byte buffer[18];
  byte size = sizeof(buffer);
  Serial.print(F("Card UID:"));
  String ProductKey = dumpInfo(mfrc522.uid.uidByte, mfrc522.uid.size);
  Serial.println(ProductKey);

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.println(mfrc522.PICC_GetTypeName(piccType));
  if (    piccType != MFRC522::PICC_TYPE_MIFARE_MINI
      &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
      &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
      Serial.println(F("This sample only works with MIFARE Classic cards."));
      return; // continue;
  }
      // Authenticate using key B
    Serial.println(F("Authenticating again using key B..."));
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }

    // Write data to the block
    Serial.print(F("Writing data into block ")); Serial.print(pageAddr);
    Serial.println(F(" ..."));
    dumpInfo(dataBlock, 16); Serial.println();
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(pageAddr, dataBlock, 16);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Write() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    }
    Serial.println();
      // Authenticate using key A
  Serial.println(F("Authenticating using key A..."));
  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
      Serial.print(F("PCD_Authenticate() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return;
  }

      // Show the whole sector as it currently is
  Serial.println(F("Current data in sector:"));
  mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, 1);
  Serial.println();
  
    // Read data from the block
  Serial.print(F("Reading data from block ")); Serial.print(pageAddr);
  Serial.println(F(" ..."));
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(pageAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Read() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
  }
  Serial.print(F("Data in block ")); Serial.print(pageAddr); Serial.println(F(":"));
  dumpInfo(buffer, 16); Serial.println();
  Serial.println();

  // WriteToTag(buffer, size, pageAddr);
  // WriteToTag(timebuffer, 8, 9);

  // Serial.println(F("MIFARE_Ultralight_Write() OK "));
  // Serial.println();
  // ProductKey = dumpInfo(mfrc522.uid.uidByte, mfrc522.uid.size);
  // // sendDataToServer();

  // // Read data ***************************************************
  // ReadFromTag(buffer, size, pageAddr);
  // ReadFromTag(timebuffer, 20, 9);

  Serial.print(F("Readed data: "));
  //Dump a byte array to Serial
  for (byte i = 0; i < 16; i++) {
    Serial.print(buffer[i], HEX);
    Serial.print(" ");
  }
  // for (byte i = 0; i < 20; i++) {
  //   Serial.write(timebuffer[i]);
  // }
  Serial.println();

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}