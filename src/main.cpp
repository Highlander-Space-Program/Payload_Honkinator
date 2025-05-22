#include <SPI.h>
#include <Arduino.h>
#include <HardwareSerial.h>
#include <iostream>
#include <string>
#include <stdint.h>
#include "FlashMemory.h"
#include "MH-Z19.h"
#include "Altimeter.h"
#include "PayloadStateMachine.h"
#include "Globals.h"


//metaData currMDExt; //testing stuff


//-----------------------------------------------------------SETUP------------------------------------------------------------------
void setup() {
  //General Stuff
  Serial.begin(115200);
  pinMode (OBLED, OUTPUT);

  //Pyro stuff
  pinMode (PYROPIN, OUTPUT);
  digitalWrite(PYROPIN, LOW);

  //Car Horn stuff
  pinMode (CARHORNPIN, OUTPUT);
  digitalWrite(CARHORNPIN, LOW);

  //Flash Memory
  SPI.begin();            // Default: SCK=18, MISO=19, MOSI=23
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);


  //MH-Z19 & Altimeter 
  MHZSerial.begin(9600, SERIAL_8N1, MH_RX_PIN, MH_TX_PIN);
  AltSerial.begin(9600, SERIAL_8N1, ALT_RX_PIN, -1);

  initializeVariables();

  //Timestamp getTimeStamp
  timeStamp = getTimeStamp();

  Serial.print("[Setup Done] Current state: ");
  Serial.println(currentState);
 
  //  //Testing Stuff
  //  Serial.print ("Size of Entry strcuts: ");
  //  Serial.print (sizeof(Entry));
  //  Serial.println (" bytes.");
  //  Serial.println ("");

  // //check metaData
  // //if (checkMetaDataFlag() == 1) {
  //   //extractMetaData(&currMD);
  // //} else {
  //   sectorErase(32751);
  //   Serial.println ("Sector erased");
  
  // //}

}

//-----------------------------------------------------------LOOP------------------------------------------------------------------

//bool printed = false;

void loop() {


  now = millis();

  if (now - lastTick >= samplingRate) {
    lastTick = now;
    Tick_Payload();
  }
  

  // //Blink OBLED to confirm code is running
  
  // digitalWrite(OBLED, HIGH);
  // delay(70);
  // digitalWrite(OBLED, LOW);
  // delay(70);

  // timeStamp = getTimeStamp();

  // if (currPageNum < 5) {
  //   readAltitude(&currAltitude);
  //   getCO2(&ppmCO2);
  //   currEntry = {currAltitude, ppmCO2, timeStamp, uint8_t(currentState)};
  //   currMD = {timeStamp, uint8_t(currentState), uint16_t(currPageNum), eBufIndx, useMD, pyroArmed, finalRead};
  
  //   if (eBufIndx < (sizeof(eBuff) / sizeof(eBuff[0]))) {
  //       eBuff[eBufIndx] = currEntry;
  //       eBufIndx++;

  //   } else {
  //       if ((currPageNum >= 1) && (currPageNum <= 32750)/*between pages 1 and 32,750 */) {
  //           writeEntryBuffToFlash (currPageNum, eBuff, eBufIndx);
  //           eBufIndx = 0;
  //           currPageNum++;

  //           std::cout << std::endl << std::endl << "entryBuff written correctly" << std::endl << std::endl; 

  //           eBuff[eBufIndx] = currEntry;
  //           eBufIndx++; 

  //       } else if (currPageNum >= 32750/*last page*/) {
  //           writeEntryBuffToFlash (32750, eBuff, eBufIndx);
  //           eBufIndx = 0;

  //           std::cout << std::endl << std::endl << "wrote to last page" << std::endl << std::endl; 

  //           eBuff[eBufIndx] = currEntry;
  //           eBufIndx++;

  //       }

  //   } 

  //   currMD.useMetaData = true;
  //   writeMetaDataToFlash(&currMD);
  
  //   std::cout << "Altitude Curr: " << currAltitude << "ft  Prev: " << prevAltitude << "ft" << std::endl;
  //   std::cout << "CO2: " << ppmCO2 << "PPM"  << std::endl;
  //   std::cout << "Timestamp: " << timeStamp << "ms"  << std::endl;
  //   std::cout << "currPageNum: " << currPageNum << std::endl;
  //   std::cout << "pyroArmed: " << pyroArmed << std::endl;
  //   std::cout << "finelRead: " << finalRead << std::endl;
  //   std::cout << "State: " << currentState << std::endl;
  //   std::cout << "Transition: " << transition  << std::endl;
  //   std::cout << "currEntry ~ Alt: " << currEntry.altitude_e
  //           << "ft,  CO2: " <<  currEntry.co2PPM_e
  //           << "PPM,  State: " << int(currEntry.currentState_e)
  //           << ",  TS: "  << currEntry.timestamp_e << std::endl;

  //   std::cout << "eBuffSize: " << uint(eBufIndx)  << "structs in buffer"<< std::endl;
  //   std::cout << "sizeof(eBuff): " << sizeof(eBuff)  << "bytes" << std::endl;
  //   std::cout << "sizeof(currEntry): " << sizeof(currEntry)  << "  ~ should be 9 (bytes)"  << std::endl;
  
  
  // } else {
  //   if(!printed && (checkMetaDataFlag() == true)) {
      
  //     extractMetaData(&currMD);

  //     std::cout << "currMD.currentPageNum: " << currMD.currentPageNum << std::endl;
  //     std::cout << "currMD.currentPageNum == 4 " << bool(currMD.currentPageNum == 4) << std::endl;

  //     extractFlashPages(currMD.currentPageNum);

  //     printed = true;
  //   } else {
  //     sectorErase(1);
  //     currMD.useMetaData = false;
  //     writeMetaDataToFlash(&currMD);
  //   }

  // }
  // std::cout << "currMD ~ TS: " << currMD.lastTimeStamp
  //           << ",  state: " << int(currMD.currentState)
  //           << ",  pageNum: " << int(currMD.currentPageNum)
  //           << ",  structIndx: " << int(currMD.structCount)
  //           << ",  usMD: " << int(currMD.useMetaData) 
  //           << ",  sizeof(currMD): " << sizeof(currMD) << std::endl;
  //   std::cout << std::endl << std::endl;

  //   extractMetaData(&currMDExt);

  //   std::cout << "currMDExt ~ TS: " << currMDExt.lastTimeStamp
  //           << ",  state: " << int(currMDExt.currentState)
  //           << ",  pageNum: " << int(currMDExt.currentPageNum)
  //           << ",  structIndx: " << int(currMDExt.structCount)
  //           << ",  usMD: " << int(currMDExt.useMetaData) 
  //           << ",  sizeof(currMDExt): " << sizeof(currMDExt) << std::endl;
  //   std::cout << std::endl << std::endl;


  // std::cout << "Timestamp: " << timeStamp << "ms"  << std::endl;
  
}