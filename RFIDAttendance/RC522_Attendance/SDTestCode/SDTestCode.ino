#include <SPI.h>
#include <SD.h>
#include "SD_Utilities.h"

int numOfLines = 0;
String getFromFile, getLine;

#define SD_CS_PIN        15  // or 5

// These pins will be use for SPI2
#define SD_CLK_PIN       14
#define SD_MOSI_PIN      13
#define SD_MISO_PIN      21
SPIClass SPI2(HSPI);

int Count_flag = 0;

void setup()
{
  Serial.begin(115200);
  Serial.println("Serial communication started.");
  delay(1000);
  pinMode(SD_CS_PIN, OUTPUT); // SS

  SPI2.begin(SD_CLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);

  if (!SD.begin(SD_CS_PIN, SPI2))
  {
    Serial.println("Card Mount Failed");
    return;
  }
  else 
  {
    getSDCardInformations();
  }

  File file = SD.open("/data.txt", FILE_WRITE);
  if(!file) {
    Serial.println("File doesn't exist");
    Serial.println("Creating file...");
  }
  else {
    Serial.println("File already exists");  
  }
  
  file.close();
  // deleteFile(SD, "/data.txt");
  // deleteFile(SD, "/hello.txt");
  // deleteFile(SD, "/nameFile.txt");
  // listDir(SD, "/", 0);


  String arr = "Line0_uid=K016,ti=12:30:59,date=20/11/2023";
  char arr2[] = "Line1_uid=K017,ti=12:30:59,date=20/11/2023";
  char arr3[] = "Line2_uid=K018,ti=12:31:59,date=20/11/2023";
  char arr4[] = "Line3_uid=K019,ti=12:32:59,date=20/11/2023";
  char arr5[] = "Line4_uid=K020,ti=12:33:59,date=20/11/2023";
  char arr6[] = "Line5_uid=K021,ti=12:34:59,date=20/11/2023";

  readFile(SD, "/data.txt", getFromFile,numOfLines);
  writeFile(SD, "/data.txt", arr.c_str(), true);
  appendFile(SD, "/data.txt", arr2, true);
  appendFile(SD, "/data.txt", arr3, true);
  appendFile(SD, "/data.txt", arr4, true);
  appendFile(SD, "/data.txt", arr5, true);
  appendFile(SD, "/data.txt", arr6, true);
  // // // writeFile(SD, "/data.txt", "K017,23/12/2023,1:23:0");
  // readFile(SD, "/data.txt", getFromFile,numOfLines);
  // Serial.print("Number of line(s):");
  // Serial.println(numOfLines);
  readFile(SD, "/data.txt", getFromFile, numOfLines);
  
  Serial.println("Get from data.txt:");
  Serial.println(getFromFile);
  Serial.println("N.o line data.txt:");
  Serial.println(numOfLines);
  Serial.println();

  while (numOfLines > 0)
  {
    readALineFile(SD, "/data.txt", getLine, numOfLines);
    Serial.println("Get a line:");
    Serial.println(getLine);
    Serial.println();
    deleteALineFile(SD, "/data.txt", numOfLines);
    readFile(SD, "/data.txt", getFromFile, numOfLines);
    Serial.println("N.o line remained:");
    Serial.println(numOfLines);
    Serial.println();
    delay(800);
  }
  // getFromFile = "";
  // Serial.println();
  // Serial.println();
  // Serial.print("After deleting line:");
  // deleteALineFile(SD, "/data.txt", 1);
  // numOfLines = 0;
  // readFile(SD, "/data.txt", getFromFile,numOfLines);
  // Serial.print("Number of line(s):");
  // Serial.println(numOfLines);
  
}

void loop()
{
  if (Serial.available())
  {
    Serial.print("Get from Serial:");
    String getValue = Serial.readString();
    getFromFile = "";
    Serial.print(getValue);

    if (getValue.startsWith("del"))
    {
      deleteFile(SD, "/data.txt");
    }
    else if (getValue.startsWith("rd"))
    {
    // readALineFile(SD, "/data.txt", getFromFile, getValue.toInt());
      Serial.print("Reading a line: ");
      Serial.println(getFromFile);

    }
  }
  delay(100);
}

