#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include "FlashMemory.h"
#include "Altimeter.h"
#include "MH-Z19.h"

//State machine enums
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
constexpr uint8_t CARHORNPIN = 26;

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
constexpr size_t ENTRY_BUFFER_SIZE_BYTES = 25;
constexpr uint32_t ENTRY_MAX_PAGE = 32750;

Entry eBuff[ENTRY_BUFFER_SIZE_BYTES] = {0};
uint8_t eBufIndx = 0;
uint32_t currPageNum = 1;

uint32_t timeStamp = 0;
int16_t currAltitude = 0;
int16_t prevAltitude = 0;
uint16_t ppmCO2 = 0;
bool pyroEjected = 0;

Entry currEntry = {currAltitude, ppmCO2, timeStamp, currentState, pyroEjected};
metaData currMD = {0}; //checked in setup
bool useMD;

//Sampling rate stuff
const uint32_t samplingRate = 340; //ms
uint32_t lastTick = 0;
uint32_t now;

//Vairiable intializer
void initializeVariables() {
    Serial.println("[Init] Initializing Variables");


    timeStamp = getTimeStamp();
    Serial.print("[Init] Timestamp: ");
    Serial.println(timeStamp);

    readAltitude(&currAltitude);
    prevAltitude = currAltitude;
        
    getCO2(&ppmCO2);

    // if (checkMetaDataFlag() == true) { //Power cycle event (in flight or whenevr)

    //     //Serial.println ("Recovering Meta Data");
    //     //Serial.println("");
    //     extractMetaData(&currMD);
        
    //     timeStamp = currMD.lastTimeStamp;
    //     currentState = RocketState(currMD.currentState);
    //     currPageNum = currMD.currentPageNum;
    //     eBufIndx = currMD.structCount;
    //     eBufIndx++;
    //     pyroArmed = currMD.pyroArmed; //check if pyroArmed;
    //     finalRead = currMD.finalRead; //check if finalRead occurred;

    //     currEntry = {currAltitude, ppmCO2, timeStamp, uint8_t(currentState), pyroEjected};

    //     Serial.print("[Init] Recovered state: ");
    //     Serial.println(currentState);
    //     Serial.print("[Init] Page #: ");
    //     Serial.println(currPageNum);
    //     Serial.print("[Init] CO2: ");
    //     Serial.println(ppmCO2);
    //     Serial.print("[Init] Altitude: ");
    //     Serial.println(currAltitude);


    //} else {
        useMD = false;
        currMD = {0};
        
        currMD.lastTimeStamp = timeStamp;
        currMD.currentState     = uint8_t(currentState);
        currMD.currentPageNum = uint16_t(currPageNum);
        currMD.structCount = eBufIndx;
        currMD.useMetaData= true;
        
        
        currEntry.altitude_e      = currAltitude;
        currEntry.co2PPM_e        = ppmCO2;
        currEntry.timestamp_e     = timeStamp;
        currEntry.currentState_e  = uint8_t(currentState);
        currEntry.pyroEjected_e   = pyroEjected;

        Serial.println("[Init] No metadata found. Starting fresh.");
        Serial.print("[Init] Initial State: ");
        Serial.println(currentState);
        Serial.print("[Init] Altitude: ");
        Serial.println(currAltitude);
        Serial.print("[Init] CO2: ");
        Serial.println(ppmCO2);

        Serial.println(currentState);
        Serial.print("[Init] Page #: ");
        Serial.println(currPageNum);
        Serial.print("[Init] CO2: ");
        Serial.println(ppmCO2);
        Serial.print("[Init] Altitude: ");
        Serial.println(currAltitude);


    //}


}





#endif // GLOBALS_H