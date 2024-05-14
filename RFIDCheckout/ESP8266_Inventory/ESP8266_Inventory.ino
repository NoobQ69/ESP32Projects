#include <SPI.h>
#include <MFRC522.h>
#include <DES.h>
#include "WiFi.h";
#include "HTTPClient.h";
#include <WebServer.h>

TaskHandle_t Task1;
WiFiServer server(80);
WiFiClient client;

String URL = "http://192.168.43.96/testRFID/testRFID_db.php";

const char *ssid = "Sussy_hotspot";
const char *password = "nguyenqu@ng10";

String ProductKey = "";
String ProductName = "";
String Category = "";
float Price = 0;
String Manufacturer = "";
char IsInventory = '1';
char IsCheckout = '0';

#define RST_PIN 17  // Configurable, see typical pin layout above
#define SS_PIN 16    // Configurable, see typical pin layout above

DES des;

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
MFRC522::StatusCode status;


//3DES-Key (This is the standard key according to the NPX specification)
byte C_key[] = { 0x49, 0x45, 0x4D, 0x4B, 0x41, 0x45, 0x52, 0x42, 0x21, 0x4E, 0x41, 0x43, 0x55, 0x4F, 0x59, 0x46, 0x49, 0x45, 0x4D, 0x4B, 0x41, 0x45, 0x52, 0x42 };

uint64_t IV = 0;
boolean isPresent = 0;
boolean isEncrypted = 0;
boolean isUlC = 0;
char Scmd[28];
char Sdata[64];

// byte page=0;

byte buffer[24];
boolean isLocked[255];    // which Pages are locked to read only
byte startProtect = 255;  // starting from which page, the data is protectet by the key
boolean Access;           // how is the data protected: 0=write protection (data visible) 1=read/write protection (data = 0x00)

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

void reconnectWiFi() {
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi Disconnected!");
    Serial.println("Reconnecting WiFi");
    Serial.print(".");
    WiFi.disconnect();
    WiFi.reconnect();
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
}

String ReadIncomingRequest() {
  String clientRequest = "";
  String myresultat = "";
  while (client.available()) {
    clientRequest = (client.readStringUntil('\r'));

    if ((clientRequest.indexOf("HTTP/1.1") > 0) && (clientRequest.indexOf("/favicon.ico") < 0)) {
      myresultat = clientRequest;
    }
  }
  return myresultat;
}

void HandleRequest(void *parameter)
{
  while (1)
  {
    reconnectWiFi();
    client = server.available();
    while(!client) {
      client = server.available();
      vTaskDelay(1/portTICK_PERIOD_MS);
    }
    while (!client.available()) { vTaskDelay(1/portTICK_PERIOD_MS); }
    String clientRequest = (ReadIncomingRequest());
    Serial.println(clientRequest);
    clientRequest.remove(0, 5);
    clientRequest.remove(clientRequest.length() - 9, 9);

    Serial.println("Get:");
    Serial.println(clientRequest);
    if (clientRequest.startsWith("Connect"))
    {
      clientRequest = "ConnectSuccess";
    }
    else if (clientRequest.startsWith("sendValue"))
    {
      String getValues[4];
      splitString(getValues, clientRequest.substring(9), "&");
      
      String getSingleValue[2];
      splitString(getSingleValue, getValues[0], "=");
      ProductName = "\'";
      getSingleValue[1].replace("%20", " ");
      ProductName += getSingleValue[1];
      ProductName += "\'";

      splitString(getSingleValue, getValues[1], "=");
      Category = "\'";
      getSingleValue[1].replace("%20", " ");
      Category += getSingleValue[1];
      Category += "\'";

      splitString(getSingleValue, getValues[2], "=");
      Price = getSingleValue[1].toFloat();
      
      splitString(getSingleValue, getValues[3], "=");
      Manufacturer = "\'";
      getSingleValue[1].replace("%20", " ");
      Manufacturer += getSingleValue[1];
      Manufacturer += "\'";

      clientRequest = "GetNameSuccess";
    }
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println("");
    client.print(clientRequest);
    client.stop();
    vTaskDelay(1/portTICK_PERIOD_MS);
    client.flush();
    vTaskDelay(100/portTICK_PERIOD_MS);
  }
}

void splitString(String* arrayString, String text, String index)
{
  int i = 0, delimiter = -1, delimiterPrevious = -1;
  do
  {
    delimiter = text.indexOf(index, delimiter+1);
    if (delimiter != -1)
    {
      arrayString[i] = String(text.substring(delimiterPrevious+1, delimiter));
      i++;
      delimiterPrevious = delimiter;
    }
  }
  while (delimiter != -1);
  
  arrayString[i] = String(text.substring(delimiterPrevious+1));
}

bool checkIsEmptyName(String name)
{
  if (name == "")
  {
    return true;
  }
  return false;
}

void sendDataToServer()
{
  if (checkIsEmptyName(ProductName))
  {
    Serial.println("Name is not specified! Please add name for product before inventory.");
    return;
  }

  String postData = "productKey=" + String(ProductKey)
                   + "&productName=" + String(ProductName);
  postData        += "&category=" + String(Category)
                  + "&price=" + String(Price)
                  + "&manufacturer=" + String(Manufacturer);
  postData        += "&isInventory=" + String(IsInventory)
                  + "&isCheckout=" + String(IsCheckout); 

  HTTPClient http; 
  http.begin(URL);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  int httpCode = http.POST(postData); 
  String payload = http.getString(); 
  
  Serial.print("URL : "); Serial.println(URL); 
  Serial.print("Data: "); Serial.println(postData); 
  Serial.print("httpCode: "); Serial.println(httpCode); 
  Serial.print("payload : "); Serial.println(payload); 
  Serial.println("--------------------------------------------------");
}

void setup() {
  Serial.begin(115200);  // Initialize serial communications with the PC
  connectWiFi();
  while (!Serial);  // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  des.init(C_key, IV);
  SPI.begin();                        // Init SPI bus
  mfrc522.PCD_Init();                 // Init MFRC522
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Read/Write Mifare Ultralight C"));
    server.begin();

  Serial.print("connected to : "); Serial.println(ssid);
  Serial.print("IP address: "); Serial.println(WiFi.localIP());
      // Task to run forever
  xTaskCreatePinnedToCore(  //
                          HandleRequest,              // Function to be called
                          "Task 1",           // Name of task
                          4096,                   // Stack size (bytes in ESP32, word in FreeRTOS)
                          NULL,                   // parameter to pass function
                          1,                      // Task priority ( 0 to configMAX_PRIORITIES - 1)
                          &Task1,                   // Task handle
                          0);               // Run on one core for demo purposes (ESP32 only)
}


void loop() {
  int i = 0;
  boolean IsCommand = 1;
  char x;
  if(WiFi.status() != WL_CONNECTED) { 
    connectWiFi();
  }
  if (Serial.available() > 0) {
    // Serial.println(freeRam());
    while (i < 40) {
      while (!Serial.available());
      x = Serial.read();
      if (IsCommand)
        Scmd[i] = x;
      else
        Sdata[i] = x;
      if (x == '\n') {  // newline
        if (IsCommand) {
          Scmd[i] = 0x00;  // proper Ending
          i = 41;          // exit
        } else {
          Sdata[i] = 0x00;
          i = 41;
        }
      } else if (x == ' ') {
        if (IsCommand == 1) {
          IsCommand = 0;
          Scmd[i] = 0x00;
          i = 0;
        } else
          i++;
      } else
        i++;
    }
    if (!isPresent)
      Serial.println(F("No PICC available!"));
    else {
      if (!strcmp(Scmd, "dump")) {  // dump data
        //DumpDataUltralight();
      } else if (!strcmp(Scmd, "auth")) {
        //RequestAuthUltralightC();
      } else if (!strcmp(Scmd, "newKey")) {
        //newKey();
      } else if (!strcmp(Scmd, "wchar")) {
        //writeData(0);
      } else if (!strcmp(Scmd, "whex")) {
        //writeData(1);
      } else if (!strcmp(Scmd, "protect")) {
        //protect(1);
      } else if (!strcmp(Scmd, "setpbit")) {
        //protect(0);
      }
    }
    Serial.flush();
    Scmd[0] = 0x00;
    Sdata[0] = 0x00;
  }
  // Look for new cards
  if (!isPresent) {
    if (!mfrc522.PICC_IsNewCardPresent()) {
      return;
    }

    // Select one of the cards
    if (!mfrc522.PICC_ReadCardSerial()) {
      return;
    } 
    else {
      isPresent = 1;
      if (mfrc522.uid.sak == 0x00) {
        if (checkIfUltralight()) {
          Serial.println(F("Mifare Ultralight C PICC found!"));
          getPageInfo();
          isUlC = 1;
        } 
        else {
          Serial.println(F("Other Mifare Ultralight compatible PICC found"));
          isUlC = 0;
        }
        Serial.print(F("UID: "));
        ProductKey = dumpInfo(mfrc522.uid.uidByte, mfrc522.uid.size);
        sendDataToServer();
      } 
      else {
        Serial.println(F("Other Card found, not compatible!"));
        mfrc522.PICC_HaltA();
        isPresent = 0;
        return;
      }
    }
  }
  if (isPresent) {  // test read - it it fails, the PICC is most likely gone
    byte byteCount = sizeof(buffer);
    status = mfrc522.MIFARE_Read(0, buffer, &byteCount);
    if (status != mfrc522.STATUS_OK) {
      isPresent = 0;
      isEncrypted = 0;
      mfrc522.PCD_StopCrypto1();
      Serial.println("Card gone...");
    }
  }
}

boolean checkIfUltralight(void) {
  byte Count = sizeof(buffer);

  if (mfrc522.MIFARE_Read(43, buffer, &Count) == mfrc522.STATUS_OK) {
    if (mfrc522.MIFARE_Read(44, buffer, &Count) == mfrc522.STATUS_OK) {
      return 0;
    } else {
      Count = sizeof(buffer);
      status = mfrc522.PICC_WakeupA(buffer, &Count);  // needed to wake up the card after receiving a NAK-answer
      return 1;
    }
  }
  Count = sizeof(buffer);
  status = mfrc522.PICC_WakeupA(buffer, &Count);  // needed to wake up the card after receiving a NAK-answer
  Serial.println(mfrc522.GetStatusCodeName(status));
  return 0;
}


boolean getPageInfo() {
  int i;
  uint32_t mask = 0;
  byte bsize = sizeof(buffer);
  status = mfrc522.MIFARE_Read(2, buffer, &bsize);  // read the first lock bits
  if (status == mfrc522.STATUS_OK) {
    mask = buffer[3];
    mask = mask << 8;
    mask = mask + buffer[2];
    isLocked[0] = 1;
    isLocked[1] = 1;
    isLocked[2] = 1;

    if (mask & 0x0001) {
      isLocked[3] = 1;
    }
    mask = mask >> 1;

    if (mask & 0x0001) {
      for (i = 0; i < 6; i++)
        isLocked[4 + i] = 1;
    }
    mask = mask >> 1;

    if (mask & 0x0001) {
      for (i = 0; i < 6; i++)
        isLocked[10 + i] = 1;
    }
    mask = mask >> 1;

    for (i = 0; i < 13; i++) {
      if (mask & 0x0001)
        isLocked[3 + i] = 1;
      mask = mask >> 1;
    }
  } else {
    bsize = sizeof(buffer);
    status = mfrc522.PICC_WakeupA(buffer, &bsize);  // needed to wake up the card after receiving a NAK-answer
    return 0;
  }
  status = mfrc522.MIFARE_Read(40, buffer, &bsize);
  if (status == mfrc522.STATUS_OK) {
    mask = buffer[1];
    mask = mask << 8;
    mask = mask + buffer[0];
    int j = 16;
    for (i = 0; i < 8; i++) {
      if ((i % 4) == 0) {
        if (mask & 0x0001)
          setBooleanBits(isLocked + j, 12);
      } else {
        if (mask & 0x0001)
          setBooleanBits(isLocked + j, 4);
        j = j + 4;
      }
      mask = mask >> 1;
    }


    for (i = 0; i < 3; i++) {
      if (mask & 0x0001)
        isLocked[41 + i] = 1;
      mask = mask >> 1;
    }

    if (mask & 0x0001)
      setBooleanBits(isLocked + 44, 4);
    mask = mask >> 1;

    for (i = 0; i < 3; i++) {
      if (mask & 0x0001)
        isLocked[41 + i] = 1;
      mask = mask >> 1;
    }

    if (mask & 0x0001)
      setBooleanBits(isLocked + 44, 4);
    mask = mask >> 1;

  } else {
    bsize = sizeof(buffer);
    status = mfrc522.PICC_WakeupA(buffer, &bsize);  // needed to wake up the card after receiving a NAK-answer
    return 0;
  }
  status = mfrc522.MIFARE_Read(42, buffer, &bsize);
  if (status == mfrc522.STATUS_OK) {
    startProtect = buffer[0];
  } else {
    bsize = sizeof(buffer);
    status = mfrc522.PICC_WakeupA(buffer, &bsize);  // needed to wake up the card after receiving a NAK-answer
    return 0;
  }
  status = mfrc522.MIFARE_Read(43, buffer, &bsize);
  if (status == mfrc522.STATUS_OK) {
    Access = buffer[0] & 0x01;
  } else {
    bsize = sizeof(buffer);
    status = mfrc522.PICC_WakeupA(buffer, &bsize);  // needed to wake up the card after receiving a NAK-answer
    return 0;
  }
  return 1;
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

byte char2byte(char *s) {
  byte x = 0;
  for (int i = 0; i < 2; i++) {
    char c = *s;
    if (c >= '0' && c <= '9') {
      x *= 16;
      x += c - '0';
    } else if (c >= 'A' && c <= 'F') {
      x *= 16;
      x += (c - 'A') + 10;
    } else if (c >= 'a' && c <= 'f') {
      x *= 16;
      x += (c - 'a') + 10;
    }
    s++;
  }
  return x;
}

/*
 * This small routine starts a request for encryption. 
 * for details see https://www.nxp.com/docs/en/data-sheet/MF0ICU2.pdf
 */
// void RequestAuthUltralightC(void) {
//   int i;
//   byte AuthBuffer[24] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };  //
//   byte AuthLength = 24;
//   byte RndA[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
//   byte RndB[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };                                                      // decrypted RndB
//   byte rRndB[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };                                                     // rotated RndB
//   byte encRndA[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };                                                   // encrypted RndA' from the PICC
//   byte dRndA[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };                                                     // decrypted RndA
//   byte message[24] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };  // Message to transfer
//   byte iv_ar[8];                                                                                  // this is the starting IV for decryption
//   byte byteCount = sizeof(buffer);

//   if (isEncrypted) {
//     Serial.println(F("PICC already authenticated"));
//     return;
//   }
//   // sparse and set the key
//   if (!setAuthKey()) {
//     Serial.println(F("key could not be set"));
//     return;
//   }
//   // set IV to 0x00
//   for (i = 0; i < 8; i++) {
//     iv_ar[i] = 0x00;
//   }
//   memcpy(&IV, iv_ar, 8);  // set IV
//   des.set_IV(IV);

//   // Build command buffer
//   AuthBuffer[0] = 0x1A;  // CMD_3DES_AUTH -> Ultralight C 3DES Authentication.
//   AuthBuffer[1] = 0x00;  //

//   // Calculate CRC_A
//   status = mfrc522.PCD_CalculateCRC(AuthBuffer, 2, &AuthBuffer[2]);
//   if (status != mfrc522.STATUS_OK) {
//     return;
//   }

//   AuthLength = sizeof(AuthBuffer);

//   // Transmit the buffer and receive the response, validate CRC_A.
//   status = mfrc522.PCD_TransceiveData(AuthBuffer, 4, AuthBuffer, &AuthLength, NULL, 0, true);
//   if (status != mfrc522.STATUS_OK) {
//     Serial.println("Ultralight C Auth failed");
//     Serial.println(mfrc522.GetStatusCodeName(status));
//     Serial.print(F("Reply: "));
//     dumpInfo(AuthBuffer, AuthLength);
//     return;
//   }
//   memcpy(message, AuthBuffer + 1, 8);  // copy the enc(RndB) from the message
//   memcpy(iv_ar, message, 8);           // use enc(RndB) as new IV for the next encryption.

//   des.set_size(8);
//   des.tdesCbcDecipher(message, RndB);  // decrypt enc(RndB) -> now we have RndB

//   randomSeed(AuthBuffer[1]);  // create RndA
//   int number;
//   for (int i = 0; i < 8; i++) {
//     number = random(255);
//     RndA[i] = number & 0xFF;  //
//   }

//   memcpy(rRndB, RndB, 8);         // copy RndB
//   rol(rRndB, 8);                  // create RndB'
//   memcpy(message, RndA, 8);       // copy RndA in the first part of message
//   memcpy(message + 8, rRndB, 8);  // adding RndB'

//   AuthBuffer[0] = 0xAF;  // set the PCD-command

//   memcpy(&IV, iv_ar, 8);  // set IV = to enk(RndB)
//   des.set_IV(IV);
//   des.set_size(16);
//   des.tdesCbcEncipher(message, &AuthBuffer[1]);

//   status = mfrc522.PCD_CalculateCRC(AuthBuffer, 17, &AuthBuffer[17]);
//   if (status != mfrc522.STATUS_OK) {
//     return;
//   }

//   memcpy(&IV, &AuthBuffer[9], 8);  // set IV to decrypt reply from PICC enc(RndA') -> RndA
//   des.set_IV(IV);

//   status = mfrc522.PCD_TransceiveData(AuthBuffer, 19, AuthBuffer, &AuthLength, NULL, 0, true);
//   if (status != mfrc522.STATUS_OK) {
//     Serial.print(F("Auth failed failed: "));
//     Serial.println(mfrc522.GetStatusCodeName(status));
//     Serial.println(F("Reply: "));
//     dumpInfo(AuthBuffer, AuthLength);
//     return;
//   } else {
//     if (AuthBuffer[0] == 0x00) {           // reply from PICC should start with 0x00
//       memcpy(encRndA, &AuthBuffer[1], 8);  // copy enc(RndA')
//       des.set_size(8);
//       des.tdesCbcDecipher(encRndA, dRndA);  // decrypt now we have decrypted RndA'
//       rol(RndA, 8);                         // rotate orgiginal RndA to RndA'
//       for (i = 0; i < 8; i++) {             // compare it
//         if (RndA[i] != dRndA[i]) {
//           i = 9;
//         }
//       }
//       if (i == 8) {
//         Serial.println(F("Keys are correct :-)"));
//         isEncrypted = 1;
//       } else
//         Serial.println(F("Keys do not match"));
//     } else {
//       Serial.println(F("Wrong answer!!!"));
//     }
//   }
//   return;
// }


// byte DumpDataUltralight(void) {
//   byte page = 0;

//   for (byte i = 0; i < 255; i++)  // initialize lock bit table
//     isLocked[i] = 0;
//   byte Count = sizeof(buffer);

//   Serial.println(F("Page lock auth 0  1  2  3"));
//   getPageInfo();  // get information about acess
//   status = mfrc522.MIFARE_Read(page, buffer, &Count);
//   while ((status == mfrc522.STATUS_OK)) {  // This loop stops at an NAK-answer
//     Serial.print(page);
//     if (page < 10)
//       Serial.print(F(" "));
//     Serial.print(F("   "));
//     if (isLocked[page])
//       Serial.print(F(" x   "));
//     else
//       Serial.print(F("     "));
//     if (page >= startProtect) {
//       if (Access)
//         Serial.print(F(" w   "));
//       else
//         Serial.print(F("r/w  "));
//     } else
//       Serial.print(F("     "));
//     dumpInfo(buffer, 4);
//     page++;
//     status = mfrc522.MIFARE_Read(page, buffer, &Count);
//   }
//   Count = sizeof(buffer);
//   status = mfrc522.PICC_WakeupA(buffer, &Count);  // needed to wake up the card after receiving a NAK-answer
//   return page;
// }

void setBooleanBits(boolean *ar, int len) {
  for (int i = 0; i < len; i++)
    ar[i] = 1;
}


// void protect(bool mode) {
//   byte value = 0;
//   byte page;
//   if (mode)  // set start page of protection
//     page = 42;
//   else  // set write or read/write protection
//     page = 43;

//   if (isLocked[page]) {
//     Serial.println(F("PICC is write protected"));
//     return;
//   }
//   if ((startProtect < page) && (!isEncrypted)) {
//     Serial.print(F("page "));
//     Serial.print(page);
//     Serial.println(F(" is password protected, autehticate first"));
//     return;
//   }

//   value = atoi(Sdata);

//   if (mode) {
//     if (value > 48)
//       value = 48;
//   } else {
//     if (value > 1)
//       value = 1;
//   }

//   for (byte i = 0; i < 4; i++)
//     buffer[i] = 0;

//   buffer[0] = value;

//   status = mfrc522.MIFARE_Ultralight_Write(page, buffer, 4);
//   if (status == mfrc522.STATUS_OK) {
//     if (mode) {
//       Serial.print(F("password protection starts now at page: "));
//       Serial.println(value);
//     } else {
//       Serial.print(F("protection bit is set to: "));
//       if (value)
//         Serial.println(F("write protected"));
//       else
//         Serial.println(F("read/write protected"));
//     }
//   } else {
//     if (mode)
//       Serial.print(F("ERROR: password protection not set: "));
//     else
//       Serial.print(F("ERROR: protection bit not set: "));
//     Serial.println(mfrc522.GetStatusCodeName(status));
//   }
// }


// boolean setAuthKey(void) {
//   int i;
//   for (i = 0; i < 32; i++) {
//     if (Sdata[i] == 0x00) {
//       Serial.println(F("Key too short! (needs to be 32 digits)"));
//       return 0;
//     } else {
//       if ((i % 2 == 0)) {
//         C_key[i / 2] = char2byte(&Sdata[i]);
//       }
//     }
//   }
//   for (i = 0; i < 8; i++) {
//     C_key[i + 16] = C_key[i];
//   }
//   des.change_key(C_key);
//   return 1;
// }


// boolean newKey(void) {
//   byte i, j, pos;
//   byte pos_ar[4] = { 7, 3, 15, 11 };
//   byte Buffer[4];
//   byte page = 0;

//   if (!isEncrypted) {
//     Serial.println(F("please authenticate fist"));
//     return 0;
//   }
//   if (setAuthKey()) {
//     for (j = 0; j < 4; j++) {
//       page = 0x2C + j;
//       pos = pos_ar[j];
//       for (i = 0; i < 4; i++) {
//         Buffer[i] = C_key[pos - i];
//       }
//       status = mfrc522.MIFARE_Ultralight_Write(page, Buffer, 4);
//     }
//     if (status == mfrc522.STATUS_OK) {
//       Serial.println(F("new key is written to the card!"));
//     } else {
//       Serial.print(F("new key could not be written: "));
//       Serial.println(mfrc522.GetStatusCodeName(status));
//       // restart the PICC
//       mfrc522.PICC_IsNewCardPresent();  // is required, as the PICC remains silent after a NAK
//       mfrc522.PICC_ReadCardSerial();
//     }
//   } else
//     return 0;

//   return 1;
// }

// void writeData(boolean mode) {
//   byte page = 0;
//   byte i = 0;
//   byte Buffer[4] = { 0, 0, 0, 0 };

//   if (isUlC)
//     Serial.println(F("Ultralight C"));
//   else
//     Serial.println(F("not a Ultralight C PICC!"));

//   while ((Sdata[i] != ' ') && (i < 3)) {
//     page = page * 10;
//     page = page + Sdata[i] - 48;
//     i++;
//   }
//   i++;  // to overcome the ' '

//   if (page < 4) {
//     Serial.println(F("no user memory here"));
//     return;
//   }
//   if (isUlC && (page > 39)) {
//     Serial.println(F("no user memory here"));
//     return;
//   }
//   byte j = 0;
//   boolean done = 0;
//   for (; page < 40; page++) {
//     for (j = 0; j < 4; j++) {
//       if (Sdata[i] == 0x00) {
//         while (j < 4) {
//           Buffer[j] = 0;
//           j++;
//         }
//         done = 1;
//       } else {
//         if (mode) {
//           Buffer[j] = char2byte(Sdata + i);
//           i = i + 2;
//         } else {
//           Buffer[j] = Sdata[i];
//           i++;
//         }
//       }
//     }

//     status = mfrc522.MIFARE_Ultralight_Write(page, Buffer, 4);
//     Serial.print(F("writing page "));
//     Serial.print(page);
//     Serial.println(mfrc522.GetStatusCodeName(status));
//     if (status != mfrc522.STATUS_OK) {
//       return;
//     }
//     if (done)
//       return;
//   }
// }

// // Needed to create RndB' out of RndB
// void rol(byte *data, int len) {
//   byte first = data[0];
//   for (int i = 0; i < len - 1; i++) {
//     data[i] = data[i + 1];
//   }
//   data[len - 1] = first;
// }



