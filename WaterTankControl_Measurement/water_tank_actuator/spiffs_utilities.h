#ifndef SPI_FFS_UTILITIES_H
#define SPI_FFS_UTILITIES_H

// Initialize SPIFFS
bool initSPIFFS() {
  if (!SPIFFS.begin(true)) 
  {
    return false;
    // Serial.println("An error has occurred while mounting SPIFFS");
  }
  return true;
  // Serial.println("SPIFFS mounted successfully");
}

// Read File from SPIFFS
String readFile(fs::FS &fs, const char * path){
  // Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if(!file || file.isDirectory())
  {
    // Serial.println("- failed to open file for reading");
    return String();
  }
  
  String fileContent;
  while(file.available())
  {
    fileContent = file.readStringUntil('\n');
    break;     
  }
  return fileContent;
}

// Write file to SPIFFS
bool writeFile(fs::FS &fs, const char * path, const char * message)
{
  // Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file)
  {
    // Serial.println("- failed to open file for writing");
    return false;
  }
  if(file.print(message))
  {
    return true;
    // Serial.println("- file written");
  } 
  return false;
  // Serial.println("- write failed");
}

#endif