#ifndef SD_UTILITIES
#define SD_UTILITIES

#endif

#include "FS.h"
#include "SD.h"
#include "SPI.h"

// void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
//   Serial.printf("Listing directory: %s\n", dirname);

//   File root = fs.open(dirname);
//   if(!root){
//     Serial.println("Failed to open directory");
//     return;
//   }
//   if(!root.isDirectory()){
//     Serial.println("Not a directory");
//     return;
//   }

//   File file = root.openNextFile();
//   while(file){
//     if(file.isDirectory()){
//       Serial.print("  DIR : ");
//       Serial.println(file.name());
//       if(levels){
//         listDir(fs, file.name(), levels -1);
//       }
//     } else {
//       Serial.print("  FILE: ");
//       Serial.print(file.name());
//       Serial.print("  SIZE: ");
//       Serial.println(file.size());
//     }
//     file = root.openNextFile();
//   }
// }

// void createDir(fs::FS &fs, const char * path){
//   Serial.printf("Creating Dir: %s\n", path);
//   if(fs.mkdir(path)){
//     Serial.println("Dir created");
//   } else {
//     Serial.println("mkdir failed");
//   }
// }

// void removeDir(fs::FS &fs, const char * path){
//   Serial.printf("Removing Dir: %s\n", path);
//   if(fs.rmdir(path)){
//     Serial.println("Dir removed");
//   } else {
//     Serial.println("rmdir failed");
//   }
// }

bool readFile(fs::FS &fs, const char * path, String &textStr, int &numberOfLine){
#ifdef DEBUG_ON
  Serial.printf("Reading file: %s\n", path);
#endif
  numberOfLine = 0;
  File file = fs.open(path);
  if(!file) {
#ifdef DEBUG_ON
    Serial.println("Failed to open file for reading");
#endif
    return false;
  }
  textStr = "";
  while(file.available()) {
    char c = file.read();
    textStr += c;
    if (c == '\n') {
      numberOfLine++;
#ifdef DEBUG_ON
      Serial.println();
#endif
    }
    else {
#ifdef DEBUG_ON
      Serial.print(c);
#endif
    }
  }
  file.close();
  return true;
}

bool writeFile(fs::FS &fs, const char * path, const char * message, bool isNewLine=false){
#ifdef DEBUG_ON
  Serial.printf("Writing file: %s\n", path);
#endif

  File file = fs.open(path, FILE_WRITE);
  if(!file){
#ifdef DEBUG_ON
    Serial.println("Failed to open file for appending");
#endif
    return false;
  }
  int result;
  if (isNewLine) {
    result = file.println(message);
  }
  else {
    result = file.print(message);
  }
  
  if(!result){
#ifdef DEBUG_ON
    Serial.println("Write failed");
#endif
    file.close();
    return false;
  }
#ifdef DEBUG_ON
  Serial.println("Message appended");
#endif
  file.close();
  return true;
}

bool appendFile(fs::FS &fs, const char * path, const char * message, bool isNewLine=false){
#ifdef DEBUG_ON
  Serial.printf("Appending to file: %s\n", path);
#endif

  File file = fs.open(path, FILE_APPEND);
  if(!file){
#ifdef DEBUG_ON
    Serial.println("Failed to open file for appending");
#endif
    return false;
  }

  int result;
  if (isNewLine) {
    result = file.println(message);
  }
  else {
    result = file.print(message);
  }
  
  if(!result){
#ifdef DEBUG_ON
    Serial.println("Append failed");
#endif
    file.close();
    return false;
  }
#ifdef DEBUG_ON
  Serial.println("Message appended");
#endif
  file.close();
  return true;
}

bool renameFile(fs::FS &fs, const char * path1, const char * path2){
#ifdef DEBUG_ON
  Serial.printf("Renaming file %s to %s\n", path1, path2);
#endif
  if (fs.rename(path1, path2)) {
#ifdef DEBUG_ON
    Serial.println("File renamed");
#endif
    return true;
  } 
#ifdef DEBUG_ON
  Serial.println("Rename failed");
#endif
  return false;
}

bool deleteFile(fs::FS &fs, const char * path) {
#ifdef DEBUG_ON
  Serial.printf("Deleting file: %s\n", path);
#endif
  if(fs.remove(path)){
#ifdef DEBUG_ON
    Serial.println("File deleted");
#endif
    return true;
  }
#ifdef DEBUG_ON
  Serial.println("Delete failed");
#endif
  return false;
}

bool deleteALineFile(fs::FS &fs, const char * path, int line) {
#ifdef DEBUG_ON
  Serial.printf("Delete a line on file: %s\n", path);
#endif

  int currentLine = 0;
  String tempPath = "/temp.txt";
  String tempStorage = "";
  File file = fs.open(path);
  if(!file) {
#ifdef DEBUG_ON
    Serial.println("Failed to open file for deleting line");
#endif
    return false;
  }
  while(file.available()){
    char c = file.read();
    if (c == '\n') {
#ifdef DEBUG_ON
      Serial.println();
#endif
      if (line != currentLine)
        tempStorage += c;

      currentLine++;
    }
    else {
      if (line != currentLine) {
#ifdef DEBUG_ON
      Serial.print(c);
#endif
        tempStorage += c;
      }
    }
  }
  file.close();

  while (!file) {
    file = fs.open(tempPath.c_str(), FILE_WRITE);
    if(!file) {
  #ifdef DEBUG_ON
        Serial.println("File doesn't exist");
        Serial.println("Creating file...");
  #endif
    }
    else {
  #ifdef DEBUG_ON
        Serial.println("File already exists");  
  #endif
    }
    vTaskDelay(20/portTICK_PERIOD_MS);
  }
  file.close();

  if (!(tempStorage == "")) {
    if (!writeFile(fs, tempPath.c_str(), tempStorage.c_str())) {
      return false;
    }
  }
  
  if (!deleteFile(fs, path)) {
    return false;
  }

  if (!renameFile(fs, tempPath.c_str(), path)) {
    return false;
  }

  return true;
}

bool readALineFile(fs::FS &fs, const char * path, String &lineStorage, int lineNumber) {
#ifdef DEBUG_ON
  Serial.print("Reading a line on file: ");
  Serial.println(path);
#endif

  int currentLine = 0;
  File file = fs.open(path);
  if(!file) {
#ifdef DEBUG_ON
    Serial.println("Failed to open file for deleting line");
#endif
    return false;
  }

  lineStorage = "";
  while(file.available()) {
    char c = file.read();
    if (c == '\n') {
#ifdef DEBUG_ON
      Serial.println();
#endif
      currentLine++;
    }
    else {
      if (lineNumber == currentLine) {
#ifdef DEBUG_ON
      Serial.print(c);
#endif
        lineStorage += c;
      }
    }
  }
  file.close();
  return true;
}

// void testFileIO(fs::FS &fs, const char * path){
//   File file = fs.open(path);
//   static uint8_t buf[512];
//   size_t len = 0;
//   uint32_t start = millis();
//   uint32_t end = start;
//   if(file){
//     len = file.size();
//     size_t flen = len;
//     start = millis();
//     while(len){
//       size_t toRead = len;
//       if(toRead > 512){
//         toRead = 512;
//       }
//       file.read(buf, toRead);
//       len -= toRead;
//     }
//     end = millis() - start;
//     Serial.printf("%u bytes read for %u ms\n", flen, end);
//     file.close();
//   } else {
//     Serial.println("Failed to open file for reading");
//   }


//   file = fs.open(path, FILE_WRITE);
//   if(!file){
//     Serial.println("Failed to open file for writing");
//     return;
//   }

//   size_t i;
//   start = millis();
//   for(i=0; i<2048; i++){
//     file.write(buf, 512);
//   }
//   end = millis() - start;
//   Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
//   file.close();
// }
