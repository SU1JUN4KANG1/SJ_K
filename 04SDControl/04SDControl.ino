/*
 * 04SDControl
 * 
 * control the SDCard,
 * read/write/rename/append/delete file
 * list/creat/remove directory
 * 
 * The MIT License (MIT)
 * Copyright (C) 2019  Seeed Technology Co.,Ltd.
 */

// include the library:
#include <spartan_edge_esp32_boot.h>

// initialize the spartan_edge_esp32_boot library
spartan_edge_esp32_boot esp32Cla;

// SD_MMC directory control 
void listDir(fs::FS &fs, const char * dirname, uint8_t levels);
void createDir(fs::FS &fs, const char * path);
void removeDir(fs::FS &fs, const char * path);

// SD_MMC file control 
void readFile(fs::FS &fs, const char * path);
void writeFile(fs::FS &fs, const char * path, const char * message);
void appendFile(fs::FS &fs, const char * path, const char * message);
void renameFile(fs::FS &fs, const char * path1, const char * path2);
void deleteFile(fs::FS &fs, const char * path);

// the setup routine runs once when you press reset:
void setup() {
  // initialization 
  esp32Cla.begin();  

  // Mount SD Card
  if(!SD_MMC.begin()){
    Serial.println("Card Mount Failed,please reboot the board");
    return;
  }

  // output the SDCard size
  uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
  Serial.printf("SD_MMC Card Size: %lluMB\n", cardSize);

  listDir(SD_MMC, "/", 0);
  createDir(SD_MMC, "/mydir");
  listDir(SD_MMC, "/", 0);
  removeDir(SD_MMC, "/mydir");
  listDir(SD_MMC, "/", 2);
  writeFile(SD_MMC, "/hello.txt", "Hello ");
  appendFile(SD_MMC, "/hello.txt", "World!\n");
  readFile(SD_MMC, "/hello.txt");
  deleteFile(SD_MMC, "/foo.txt");
  renameFile(SD_MMC, "/hello.txt", "/foo.txt");
  readFile(SD_MMC, "/foo.txt");
}

// the loop routine runs over and over again forever:
void loop() {
  // nothink to do
}

// list the files in the directory 
void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if(!root){
    Serial.println("Failed to open directory");
      return;
    }
  if(!root.isDirectory()){
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while(file){
    if(file.isDirectory()){
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if(levels){
        listDir(fs, file.name(), levels -1);
        }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
      file = root.openNextFile();
  }
}

// creat a directory
void createDir(fs::FS &fs, const char * path){
  Serial.printf("Creating Dir: %s\n", path);
  if(fs.mkdir(path)){
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}

// remove a directory
void removeDir(fs::FS &fs, const char * path){
  Serial.printf("Removing Dir: %s\n", path);
  if(fs.rmdir(path)){
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
  }
}

// read a file and output data by Serial
void readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while(file.available()){
    Serial.write(file.read());
  }
}

// write a file, it will recover data
void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
}

// append message in file end
void appendFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file){
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message)){
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
}

// rename a file
void renameFile(fs::FS &fs, const char * path1, const char * path2){
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
  }
}

// delete a file 
void deleteFile(fs::FS &fs, const char * path){
  Serial.printf("Deleting file: %s\n", path);
  if(fs.remove(path)){
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}
