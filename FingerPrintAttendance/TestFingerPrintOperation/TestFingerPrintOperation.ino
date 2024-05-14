#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>

#define DEBUG_ON

int relay1 = 12;
int relay2 = 13;
int relay3 = 14;
int relay4 = 27;

uint8_t id;

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial2);

void setup() {
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);
  pinMode(relay4, OUTPUT);
  digitalWrite(relay1, LOW);
  digitalWrite(relay2, LOW);
  digitalWrite(relay3, LOW);
  digitalWrite(relay4, LOW);
  Serial.begin(9600);
  // while (!Serial);  // For Yun/Leo/Micro/Zero/...
  delay(2000);
  Serial.println("\n\nAdafruit finger detect test");

  // set the data rate for the sensor serial port
  finger.begin(57600);
  delay(5);
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }

  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  Serial.print(F("Status: 0x"));
  Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x"));
  Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: "));
  Serial.println(finger.capacity);
  Serial.print(F("Security level: "));
  Serial.println(finger.security_level);
  Serial.print(F("Device address: "));
  Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: "));
  Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: "));
  Serial.println(finger.baud_rate);

  finger.getTemplateCount();

  if (finger.templateCount == 0) {
    Serial.print("Sensor doesn't contain any fingerprint data. Please run the 'enroll' example.");
  } 
  else {
    Serial.println("Waiting for valid finger...");
    Serial.print("Sensor contains ");
    Serial.print(finger.templateCount);
    Serial.println(" templates");
  }
}

int mode = 0;

void loop()  // run over and over again
{
  if (Serial.available())
  {
    String getRequest = Serial.readString();
    if (getRequest.startsWith("reg"))
    {
      mode = 1;
    }
    else if (getRequest.startsWith("atc"))
    {
      mode = 2;
    }
    else if (getRequest.startsWith("inf"))
    {
      mode = 3;
    }
    else if (getRequest.startsWith("del"))
    {
      finger.emptyDatabase();
      Serial.println("Now database is empty :)");
    }
  }

  if (mode == 1)
  {
    Serial.println("Ready to enroll a fingerprint!");
    Serial.println("Please type in the ID # (from 1 to 127) you want to save this finger as...");
    id = readnumber();
    getFingerprintEnroll();
    mode = 0;
  }
  else if (mode == 2)
  {
    getFingerprintID();
  }
  else if (mode == 3)
  {
    getSensorInfo();
    mode = 0;
  }

}

void getSensorInfo()
{
  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  Serial.print(F("Status: 0x"));
  Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x"));
  Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: "));
  Serial.println(finger.capacity);
  Serial.print(F("Security level: "));
  Serial.println(finger.security_level);
  Serial.print(F("Device address: "));
  Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: "));
  Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: "));
  Serial.println(finger.baud_rate);

  finger.getTemplateCount();

  if (finger.templateCount == 0) {
    Serial.print("Sensor doesn't contain any fingerprint data. Please run the 'enroll' example.");
  } 
  else {
    Serial.println("Waiting for valid finger...");
    Serial.print("Sensor contains ");
    Serial.print(finger.templateCount);
    Serial.println(" templates");
  }
}

uint8_t readnumber(void) {
  uint8_t num = 0;

  while (num == 0) {
    while (! Serial.available());
    num = Serial.parseInt();
  }
  return num;
}

// Register finger
uint8_t getFingerprintEnroll() 
{
#ifdef DEBUG_ON
  Serial.println("Ready to enroll a fingerprint!");
  Serial.println("Please type in the ID # (from 1 to 127) you want to save this finger as...");
#endif

  id = readnumber();
  
  int p = -1;
#ifdef DEBUG_ON
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
#endif
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
#ifdef DEBUG_ON
      Serial.println("Image taken");
#endif
      break;
    case FINGERPRINT_NOFINGER:
#ifdef DEBUG_ON
      Serial.println(".");
#endif
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
#ifdef DEBUG_ON
      Serial.println("Communication error");
#endif
      break;
    case FINGERPRINT_IMAGEFAIL:
#ifdef DEBUG_ON
      Serial.println("Imaging error");
#endif
      break;
    default:
#ifdef DEBUG_ON
      Serial.println("Unknown error");
#endif
      break;
    }
  }

  // OK success!
  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
#ifdef DEBUG_ON
      Serial.println("Image converted");
#endif
      break;
    case FINGERPRINT_IMAGEMESS:
#ifdef DEBUG_ON
      Serial.println("Image too messy");
#endif
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
#ifdef DEBUG_ON
      Serial.println("Communication error");
#endif
      return p;
    case FINGERPRINT_FEATUREFAIL:
#ifdef DEBUG_ON
      Serial.println("Could not find fingerprint features");
#endif
      return p;
    case FINGERPRINT_INVALIDIMAGE:
#ifdef DEBUG_ON
      Serial.println("Could not find fingerprint features");
#endif
      return p;
    default:
#ifdef DEBUG_ON
      Serial.println("Unknown error");
#endif
      return p;
  }

#ifdef DEBUG_ON
  Serial.println("Remove finger");
#endif
  
  p = 0;
  // turn led on
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
    delay(100);
  }
  // turn led off
#ifdef DEBUG_ON
  Serial.print("ID "); Serial.println(id);
#endif

  p = -1;
  Serial.println("Place same finger again");
  
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
#ifdef DEBUG_ON
      Serial.println("Image taken");
#endif
      break;
    case FINGERPRINT_NOFINGER:
      // blink led 
#ifdef DEBUG_ON
      Serial.print(".");
#endif
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
#ifdef DEBUG_ON
      Serial.println("Communication error");
#endif
      break;
    case FINGERPRINT_IMAGEFAIL:
#ifdef DEBUG_ON
      Serial.println("Imaging error");
#endif
      break;
    default:
#ifdef DEBUG_ON
      Serial.println("Unknown error");
#endif
      break;
    }

    if (p != FINGERPRINT_NOFINGER && p != FINGERPRINT_OK)
    {
      Serial.println("Failed to register fingerprint!");
      return p;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
#ifdef DEBUG_ON
      Serial.println("Image converted");
#endif
      break;
    case FINGERPRINT_IMAGEMESS:
#ifdef DEBUG_ON
      Serial.println("Image too messy");
#endif
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
#ifdef DEBUG_ON
      Serial.println("Communication error");
#endif
      return p;
    case FINGERPRINT_FEATUREFAIL:
#ifdef DEBUG_ON
      Serial.println("Could not find fingerprint features");
#endif
      return p;
    case FINGERPRINT_INVALIDIMAGE:
#ifdef DEBUG_ON
      Serial.println("Could not find fingerprint features");
#endif
      return p;
    default:
#ifdef DEBUG_ON
      Serial.println("Unknown error");
#endif
      return p;
  }

  // OK converted!
#ifdef DEBUG_ON
  Serial.print("Creating model for #");  Serial.println(id);
#endif

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
#ifdef DEBUG_ON
    Serial.println("Prints matched!");
#endif
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
#ifdef DEBUG_ON
    Serial.println("Communication error");
#endif
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
#ifdef DEBUG_ON
    Serial.println("Fingerprints did not match");
#endif
    return p;
  } else {
#ifdef DEBUG_ON
    Serial.println("Unknown error");
#endif
    return p;
  }

#ifdef DEBUG_ON
  Serial.print("ID "); Serial.println(id);
#endif
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
#ifdef DEBUG_ON
    Serial.println("Stored!");
#endif
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
#ifdef DEBUG_ON
    Serial.println("Communication error");
#endif
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
#ifdef DEBUG_ON
    Serial.println("Could not store in that location");
#endif
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
#ifdef DEBUG_ON
    Serial.println("Error writing to flash");
#endif
    return p;
  } else {
#ifdef DEBUG_ON
    Serial.println("Unknown error");
#endif
    return p;
  }

  return p;
}

uint8_t getFingerprintID() {
  
  uint8_t p = finger.getImage();

  switch (p) {
    case FINGERPRINT_OK:
#if DEBUG_ON
      Serial.println("Image taken");
#endif
      break;
    case FINGERPRINT_NOFINGER:
#if DEBUG_ON
      Serial.println("No finger detected");
#endif
      finger.LEDcontrol(FINGERPRINT_LED_OFF, 0, FINGERPRINT_LED_BLUE);
      finger.LEDcontrol(FINGERPRINT_LED_OFF, 0, FINGERPRINT_LED_RED);
      delay(100);  //don't ned to run this at full speed.
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
#if DEBUG_ON
      Serial.println("Communication error");
#endif
      return p;
    case FINGERPRINT_IMAGEFAIL:
#if DEBUG_ON
      Serial.println("Imaging error");
#endif
      return p;
    default:
#if DEBUG_ON
      Serial.println("Unknown error");
#endif
      return p

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
#if DEBUG_ON
      Serial.println("Image converted");
#endif
      break;
    case FINGERPRINT_IMAGEMESS:
#if DEBUG_ON
      Serial.println("Image too messy");
#endif
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
#if DEBUG_ON
      Serial.println("Communication error");
#endif
      return p;
    case FINGERPRINT_FEATUREFAIL:
#if DEBUG_ON
      Serial.println("Could not find fingerprint features");
#endif
      return p;
    case FINGERPRINT_INVALIDIMAGE:
#if DEBUG_ON
      Serial.println("Could not find fingerprint features");
#endif
      return p;
    default:
#if DEBUG_ON
      Serial.println("Unknown error");
#endif
      return p;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
#if DEBUG_ON
    Serial.println("Found a print match!");
#endif
    finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_PURPLE, 10);
    delay(1000);

  } 
  else if (p == FINGERPRINT_PACKETRECIEVEERR) {
#if DEBUG_ON
    Serial.println("Communication error");
#endif
    return p;
  } 
  else if (p == FINGERPRINT_NOTFOUND) {
#if DEBUG_ON
    Serial.println("Did not find a match");
#endif
    finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_RED, 10);
    delay(1000);
    return p;
  } 
  else {
#if DEBUG_ON
    Serial.println("Unknown error");
#endif
    return p;
  }

#if DEBUG_ON
  // found a match!
  Serial.print("Found ID #");
  Serial.print(finger.fingerID);
  Serial.print(" with confidence of ");
  Serial.println(finger.confidence);
#endif

  return finger.fingerID;
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) return -1;

#ifdef DEBUG_ON
  // found a match!
  Serial.print("Found ID #");
  Serial.print(finger.fingerID);
  Serial.print(" with confidence of ");
  Serial.println(finger.confidence);
#endif

  return finger.fingerID;
}

