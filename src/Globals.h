#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include "FlashMemory.h"
#include "Altimeter.h"
#include "MH-Z19.h"

//State machine structs
enum RocketState {
    ON_PAD = 0,
    ASCENDING = 1,
    DESCENDING = 2,
    LANDED = 3
};

enum Transitions {
    LAUNCHED = 0, 
    ASCENT_PASSED_600 = 1,      
    APOGEE = 2,                 
    DESCENT_PASSED_600 = 3,
    TOUCHDOWN = 4,
    NA = 5

};


//PURE GPIO Pins
constexpr uint8_t OBLED = 2; //main loop confirms code is running
constexpr uint8_t PYROPIN = 27; //stateMachine, fires E-MATCH

//Serial UART COMMS & Objects
const uint8_t MH_RX_PIN = 22;
const uint8_t MH_TX_PIN = 4;     
HardwareSerial MHZSerial (1);          //MH-Z19

const uint8_t ALT_RX_PIN = 13;   
HardwareSerial AltSerial (2);          //stratoLoggerCF

//stateMachine
volatile RocketState currentState = ON_PAD;
Transitions transition = NA;

bool pyroArmed = false;
bool finalRead = false;



//Entry stuff
constexpr size_t ENTRY_BUFFER_SIZE_BYTES = 28;
constexpr uint32_t ENTRY_MAX_PAGE = 32750;

Entry eBuff[ENTRY_BUFFER_SIZE_BYTES] = {0};
uint8_t eBufIndx = 0;
uint32_t currPageNum = 1;

uint32_t timeStamp = 0;
int16_t currAltitude = 0;
int16_t prevAltitude = 0;
uint16_t ppmCO2 = 0;

Entry currEntry = {currAltitude, ppmCO2, timeStamp, currentState};
metaData currMD = {0}; //checked in setup
bool useMD;

void initializeVariables() {
    timeStamp = getTimeStamp();

    /*if (checkMetaDataFlag() == true) { //Power cycle event (in flight or whenevr)

        Serial.println ("Recovering Meta Data");
        Serial.println("");
        extractMetaData(&currMD);
        
        extractMetaData(&currMD);
        timeStamp = currMD.lastTimeStamp;
        currentState = RocketState(currMD.currentState);
        currPageNum = currMD.currentPageNum;
        eBufIndx = currMD.structCount;
        eBufIndx++;
        pyroArmed = currMD.pyroArmed; //check if pyroArmed;
        finalRead = currMD.finalRead; //check if finalRead occurred;
        
        readAltitude(&currAltitude);
        prevAltitude = currAltitude;
        
        getCO2(&ppmCO2);
        

    } else {*/
        currMD = {0};
    //}


}





#endif // GLOBALS_H