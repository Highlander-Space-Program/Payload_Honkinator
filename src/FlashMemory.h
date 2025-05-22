#ifndef FLASH_MEMORY_H
#define FLASH_MEMORY_H

#include <Arduino.h>
#include <SPI.h>


/*
CS Pin = Chip Select Pin
When set to low = Stand ready for my arrival worm
Low = tells the FLASH to pay attention, done prior to sending it a command

Set to High: Stop paying attention to the (data) bus
*/
#define CS_PIN 5  


//SPI.transfer() sends a byte into the flash and at the same time gets one back

// Sends the write enable command, done prior to just about everything
void write_enable() {
  digitalWrite(CS_PIN, LOW);
  SPI.transfer(0x06);              
  digitalWrite(CS_PIN, HIGH);
}

/*
Checks the busy status register until its no longer busy, done after every SPI command
Ensures nothing happens until the command is done executing 
"SPI.transfer(0x05);" is the command that says "get ready to read status register 1"
"SPI.transfer(0x00)" sends a "dummy" byte to "clock" the flash so that it sends a byte back
This byte comes from the status register 1 thanks to the 0x05 command, so it returns its current byte
This is "polling" the register
The "& 0x01" is a bit mask so we only look at the lowest bit (bit zero) which is the BUSY bit
When bit zero is set 1 (busy) then it  equivalent to "true" as such the loop keeps running
When bit zero is set to 0 (not busy) then it is equivalent to "false" and the loop breaks
*/ 
void waitForWrite() {
  digitalWrite(CS_PIN, LOW);

  SPI.transfer(0x05);
  while (SPI.transfer(0x00) & 0x01); // Wait while BUSY bit is set

  digitalWrite(CS_PIN, HIGH);
}


/*
Erases 1 sector = 16 pages = 16*256 bytes
It constructs the tData buffer which contains: erase sector command (0x20), 
and memAddr of where the sector starts
*/
void sectorErase(uint16_t pageNum) {

  uint32_t memAddr = pageNum*256;
  uint8_t tData[4]; // 4 bytes in a 8 byte array
  tData[0] = 0x20; //sector erase command
  tData[1] = ((memAddr >> 16) & 0xFF); //MSB of the memAddr
  tData[2] = ((memAddr >> 8) & 0xFF); //middly byte of the memAddr
  tData[3] = (memAddr & 0xFF);       //LSB of the memAddr
  

  write_enable();
  digitalWrite(CS_PIN, LOW);

  SPI.transfer(tData, 4);

  digitalWrite(CS_PIN, HIGH);
  waitForWrite();
}



void writeDataToFlash (uint16_t pageNum, uint8_t *dataToWrite, size_t bytes){
  uint8_t cmd = 0x02;
  uint32_t memAddr = pageNum*256;

  uint8_t tData[bytes +4]; //bytes = size ofdataToWrite + 4 for the cmd and memAddr
  tData[0] = cmd;
  tData[1] = ((memAddr >> 16) & 0xFF); 
  tData[2] = ((memAddr >> 8) & 0xFF); 
  tData[3] = (memAddr & 0xFF); //these are the extra 4 bytes

  /*
  Since size(tData) = bytes + 4 and the first 4 bytes are already occupied
  we shift its index by 4, then we write all the data from the input buffer
  */
  for (int i = 0; i < bytes; i++){ 
    tData[i+4] = dataToWrite[i];  
  }

  write_enable();
  digitalWrite (CS_PIN, LOW);

  SPI.transfer(tData, bytes + 4);
  digitalWrite (CS_PIN, HIGH);
  waitForWrite();

}


/*
Reading does not reauire a write enable as per datasheet
Also doest require waitForWrite because the busy bit doesnt get flagged
*/
void readDataFromFlash (uint16_t pageNum, uint8_t *buffToStore, size_t bytes){
  uint8_t cmd = 0x03;
  uint32_t memAddr = pageNum*256;

  uint8_t tData[4];  // 1 byte for command + 3 bytes for address
  tData[0] = cmd;
  tData[1] = ((memAddr >> 16) & 0xFF); 
  tData[2] = ((memAddr >> 8) & 0xFF); 
  tData[3] = (memAddr & 0xFF); 

  digitalWrite (CS_PIN, LOW);

  SPI.transfer(tData, 4);

  for (int i = 0; i < bytes; i++) {
    buffToStore[i] = SPI.transfer(0x00); //clocking the flash, storing to buffer
  }

  digitalWrite (CS_PIN, HIGH);

}


//Struct 
typedef struct __attribute__((packed)) {
  int16_t altitude_e;    //2 bytes
  uint16_t co2PPM_e;      //2 bytes
  uint32_t timestamp_e;   //4 bytes unsigned long
  uint8_t currentState_e;  //1 bytes prolly
  bool pyroEjected_e;     //1 byte

} Entry; //currently 10 bytes = 25 entries per page, 25 entres = 250 bytes ~ 1 page


void writeEntryBuffToFlash(uint16_t page, Entry* sBuffer, size_t numEntries) {
  
  size_t numBytes = numEntries * sizeof(Entry);
  uint8_t byteBuffer[numBytes] = {0};
  
  memcpy(byteBuffer, sBuffer, numBytes);
 
  // Cast to Entry* to read the last entry as a struct
  Entry* entryView = (Entry*)byteBuffer;
  Entry lastEntry = entryView[numEntries - 1];

  Serial.print("numBytes: ");
  Serial.println(numBytes);

  Serial.println("Last Entry (fields):");
  Serial.print("  Altitude: ");
  Serial.print(lastEntry.altitude_e);
  Serial.print(" ft,  CO2: ");
  Serial.print(lastEntry.co2PPM_e);
  Serial.print(" PPM,  Timestamp: ");
  Serial.print(lastEntry.timestamp_e);
  Serial.print(" ms,  State: ");
  Serial.println(lastEntry.currentState_e);

  Serial.print("Last Entry (raw bytes): ");
  uint8_t* lastStructPtr = (uint8_t*)&entryView[numEntries - 1];
  for (int i = 0; i < sizeof(Entry); ++i) {
    if (lastStructPtr[i] < 0x10) Serial.print("0"); // pad single-digit hex
    Serial.print(lastStructPtr[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  Serial.println();


  writeDataToFlash(page, byteBuffer, numBytes);
}


void extractFlashPages (uint16_t lastPage) {
  uint8_t extractorBuff[256];

  for (uint16_t i = 1; i <= lastPage; i++){
    readDataFromFlash (i, extractorBuff, 256);

    Entry* extractedEntries = (Entry*)extractorBuff; //transforms the extractor buffer into an array of structs

    for (int j = 0; j < (250 / sizeof(Entry))  ; j++) {
      Serial.print ("Page "); Serial.print (i);
      Serial.print(", Entry "); Serial.print(j);
      Serial.print(" | Altitude: "); Serial.print(extractedEntries[j].altitude_e);
      Serial.print(" ft, CO2: "); Serial.print(extractedEntries[j].co2PPM_e);
      Serial.print (" PPM, timeStamp: "); Serial.print(extractedEntries[j].timestamp_e);
      Serial.print (" ms, currentState: "); Serial.println(extractedEntries[j].currentState_e);


    }

  }

}

//getTimeStamp function
uint32_t getTimeStamp() {
  static uint32_t startingTime = 0;

  if (startingTime == 0) {
    startingTime = millis();
  }

  return millis() - startingTime; //time since getTimeStamp was first called
}

//Metadata struct 11 bytes
typedef struct __attribute__((__packed__)) {
  uint32_t lastTimeStamp;     // 4 bytes
  uint8_t currentState;       // 1 byte
  uint16_t currentPageNum;    // 2 bytes
  uint8_t structCount;        // 1 byte
  bool useMetaData;           // 1 byte
  bool pyroArmed;             // 1 byte
  bool finalRead;             // 1 byte
} metaData;

//Will be written to page 32768 - 16 = 32752 or indexes 32751 -> 32767 so that i can erase sector w/o abandon


void writeMetaDataToFlash(metaData* mData) {

  sectorErase(32751);

  uint8_t byteBuffer[sizeof(metaData)];
  
  
  memcpy(byteBuffer, mData, sizeof(byteBuffer));
 
  writeDataToFlash(32751, byteBuffer, sizeof(byteBuffer));

  Serial.println("Raw byteBuffer contents:");
  for (int i = 0; i < sizeof(metaData); i++) {
    Serial.print("0x");
    Serial.print(byteBuffer[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  Serial.println ("Wrote MD succesfully??");
  Serial.println("");
}


bool checkMetaDataFlag () {
  uint8_t extractorBuff[sizeof(metaData)]; //should always be 9

  readDataFromFlash (32751, extractorBuff, sizeof(metaData));

  metaData extractedMetaData;
  memcpy(&extractedMetaData, extractorBuff, sizeof(metaData));

  return (extractedMetaData.useMetaData); 

}

void extractMetaData (metaData* mData) {
  uint8_t extractorBuff[sizeof(metaData)]; //should always be 9

  readDataFromFlash (32751, extractorBuff, sizeof(extractorBuff));

  Serial.println("Raw extractorBuff contents:");
  for (int i = 0; i < sizeof(metaData); i++) {
    Serial.print("0x");
    Serial.print(extractorBuff[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  
  memcpy(mData, extractorBuff, sizeof(extractorBuff));

}




#endif // FLASH_MEMORY_H