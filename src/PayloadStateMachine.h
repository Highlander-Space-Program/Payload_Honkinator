#ifndef PAYLOAD_STATE_MACHINE_H
#define PAYLOAD_STATE_MACHINE_H

/*
Statemachine type shit 

WE eject at 600ft 

States:
On Pad
-Doesn take measurements from MHZ
-Reads altimeter doesnt store it
-Checks for ascending altitude (Switch)

Ascending 
- Records altimeter and MHZ data 
- Writes to flash
- Checks alttitude for apogee
- Notes when altitdue = 600ft, set a flag
- Hit apogee (switch)

Descending 
- Records altimeter and MHZ data 
- Writes to flash 
- Checks altitude for 2nd time hitting 600ft 
- Ejects (fires pyro charge) (fast)
- Checks for stable altitude AKA landing (switch)

Landed 
- Takes finals measurements of altitude and MHZ (use a flag)
- Stops storing data (and perhaps stop reading it?)
- Honks horn


*/

#include<Arduino.h>
#include <iostream>
#include "FlashMemory.h"
#include "MH-Z19.h"
#include "Altimeter.h"
#include "Globals.h"


bool stableAltitude (int16_t currAlt, int16_t prevAlt) { //+-15ft
    static int stabilityCounter = 0;

    if ((currAlt <= (15 + prevAlt)) && (currAlt >= (prevAlt - 15) )) {
        stabilityCounter++;
        
        if (stabilityCounter >= 29) { //appx 10 seconds at the same altitude
            Serial.print("stabilityCounter: "); Serial.println(stabilityCounter);
            return true;
        }
    
    } else {
        stabilityCounter = 0 ;
    }

    Serial.print("stabilityCounter: "); Serial.println(stabilityCounter);
    return false;
}


Transitions checkTransition (int16_t currAlt, int16_t prevAlt){ 
    switch (currentState) {
        case ON_PAD:
            if (currAlt > prevAlt + 4) {
                return LAUNCHED;
            }
        break;

        case ASCENDING:
            if (!pyroArmed && currAlt >= 600) {
                return ASCENT_PASSED_600;
            }

            if (currAlt < prevAlt + 4) {
                return APOGEE;
            }
        break;

        case DESCENDING:
            if (stableAltitude(currAlt, prevAlt)) {
                return TOUCHDOWN;
            }
        
            if (pyroArmed && currAlt <= 600) {
                return DESCENT_PASSED_600;
            }

        break;

        case LANDED:
            return NA;            
        break;
    }

    return NA;

}




//Everything Everywhere All At Once
void Tick_Payload () {

    Serial.println("");
    Serial.println("");
    Serial.println("");
    Serial.println("Tick Payload started");
    digitalWrite(OBLED, HIGH);

    if (finalRead == 0) {
        readAltitude(&currAltitude);
        getCO2(&ppmCO2);
        timeStamp = getTimeStamp();
        currEntry.altitude_e      = currAltitude;
        currEntry.co2PPM_e        = ppmCO2;
        currEntry.timestamp_e     = timeStamp;
        currEntry.currentState_e  = uint8_t(currentState);
        currEntry.pyroEjected_e   = pyroEjected;

        Serial.print("currAltitude: "); Serial.println(currAltitude);
        Serial.print("prevAltitude: "); Serial.println(prevAltitude);
        Serial.print("CO2 PPM: "); Serial.println(ppmCO2);
    }

    Serial.print("Current State: "); Serial.println(currentState);
    Serial.print("Raw TimeStamp: "); Serial.println(getTimeStamp());
    
    

    std::cout << "currEntry ~ Alt: " << currEntry.altitude_e
            << "ft,  CO2: " <<  currEntry.co2PPM_e
            << "PPM,  State: " << int(currEntry.currentState_e)
            << ",  TS: "  << currEntry.timestamp_e
            << ",  pyroEjected: "  << currEntry.pyroEjected_e << std::endl;

    currMD.lastTimeStamp = timeStamp;
    currMD.currentState     = uint8_t(currentState);
    currMD.currentPageNum = uint16_t(currPageNum);
    currMD.structCount = eBufIndx;
    currMD.useMetaData= true;
    currMD.pyroArmed = pyroArmed;
    currMD.finalRead = finalRead;

    std::cout << "currMD ~ timeStamp: " << currMD.lastTimeStamp
            << "ms,  State: " <<  int(currMD.currentState)
            << ",  pageNum: " << currMD.currentPageNum
            << ",  structCount: "  << int(currMD.structCount)
            << ",  pyroArmed: "  << currMD.pyroArmed 
            << ",  finalRead: "  << currMD.finalRead 
            << std::endl;

    transition = checkTransition(currAltitude, prevAltitude);
    Serial.print("Transition evaluated: ");
    Serial.println(transition);  // Should match your enum values


    switch (transition) {
        case LAUNCHED:
            currentState = ASCENDING;
        break;

        case ASCENT_PASSED_600:
            pyroArmed = true;
        break;

        case APOGEE:
            currentState = DESCENDING;
        break;

        case DESCENT_PASSED_600:
            //eject
            //set MOSFET gate to high long enough to inite ematch
            digitalWrite(PYROPIN, HIGH);
            delay(20);
            digitalWrite(PYROPIN, LOW);
            pyroEjected = true;

        break;

        case TOUCHDOWN:
            currentState = LANDED;
        break;

        case NA:
        break;
    }

    

    switch (currentState) {
        case ON_PAD:
            //nothing just check for ASCENDING
        break;

        case ASCENDING:
            //get altitude and CO2 data
            //store data + metaData
            //check for apogee
            if (eBufIndx < (sizeof(eBuff) / sizeof(eBuff[0]))) {
                eBuff[eBufIndx] = currEntry;
                eBufIndx++;

                
            
            } else {
                if ((currPageNum >= 1) && (currPageNum <= 32749)/*between pages 1 and 32,750 */) {
                    writeEntryBuffToFlash (currPageNum, eBuff, sizeof(eBuff));
                    eBufIndx = 0;
                    currPageNum++;

                    eBuff[eBufIndx] = currEntry;
                    eBufIndx++;

                    Serial.println("");
                    Serial.println("WROTE ENTRY BUFFER:");
                    Serial.println("");


                     

                } else if (currPageNum >= 32750/*last page*/) {
                    writeEntryBuffToFlash (32750, eBuff, sizeof(eBuff));
                    eBufIndx = 0;

                    eBuff[eBufIndx] = currEntry;
                    eBufIndx++;

                    Serial.println("");
                    Serial.println("WROTE ENTRY BUFFER:");
                    Serial.println("");

                }

            } 

             writeMetaDataToFlash(&currMD);


        break;

        case DESCENDING:
            //get altitude and CO2 data
            //store data + metaData
            //check for DESCENT_PASSED_600, if so eject

            if (eBufIndx < (sizeof(eBuff) / sizeof(eBuff[0]))) {
                eBuff[eBufIndx] = currEntry;
                eBufIndx++;

                //store metaData
            
            } else {
                if ((currPageNum >= 1) && (currPageNum <= 32749)/*between pages 1 and 32,750 */) {
                    writeEntryBuffToFlash (currPageNum, eBuff, sizeof(eBuff));
                    eBufIndx = 0;
                    currPageNum++;

                    eBuff[eBufIndx] = currEntry;
                    eBufIndx++;

                    //store metaData
                    Serial.println("");
                    Serial.println("WROTE ENTRY BUFFER:");
                    Serial.println("");

                } else if (currPageNum >= 32750/*last page*/) {
                    writeEntryBuffToFlash (32750, eBuff, sizeof(eBuff));
                    eBufIndx = 0;

                    eBuff[eBufIndx] = currEntry;
                    eBufIndx++;

                    //store metaData
                    Serial.println("");
                    Serial.println("WROTE ENTRY BUFFER:");
                    Serial.println("");

                }

                 writeMetaDataToFlash(&currMD);

            } 
        break;

        case LANDED:
            //read last data
            //store data + metaData
            //start car horn

            if (!finalRead) {
                if (eBufIndx < (sizeof(eBuff) / sizeof(eBuff[0]))) {
                    eBuff[eBufIndx] = currEntry;
                    eBufIndx++;

                    //store metaData
                
                } else {
                    if (( (currPageNum >= 1) && (currPageNum <= 32749) ) && (eBufIndx > 0)/*between pages 1 and 32,766 && eBuffIndx > 0*/) {
                        writeEntryBuffToFlash (currPageNum, eBuff, sizeof(eBuff));
                        eBufIndx = 0;
                        currPageNum++;

                        eBuff[eBufIndx] = currEntry;
                        eBufIndx++;

                        finalRead = true;

                        //store metaData
                        Serial.println("");
                    Serial.println("WROTE ENTRY BUFFER:");
                    Serial.println("");

                    } else if (currPageNum >= 32750/*last page*/) {
                        writeEntryBuffToFlash (32750, eBuff, sizeof(eBuff));
                        eBufIndx = 0;

                        eBuff[eBufIndx] = currEntry;
                        eBufIndx++;

                        finalRead = true;

                        //store metaData
                        Serial.println("");
                    Serial.println("WROTE ENTRY BUFFER:");
                    Serial.println("");

                    }

                } 
                
                writeMetaDataToFlash(&currMD);
            }

            //honk for 2 seconds every 30 seconds
            delay(1000);
            digitalWrite(CARHORNPIN, HIGH);
            delay(500);
            digitalWrite(  CARHORNPIN, LOW);
        break;

    }

    prevAltitude = currAltitude;

    delay(10);
    digitalWrite(OBLED, LOW);

    Serial.println("");Serial.println("Post Tick stats:");
    Serial.print("currAltitude: "); Serial.println(currAltitude);
        Serial.print("prevAltitude: "); Serial.println(prevAltitude);
        Serial.print("CO2 PPM: "); Serial.println(ppmCO2);
        Serial.print("Current State: "); Serial.println(currentState);
    Serial.print("Raw TimeStamp: "); Serial.println(getTimeStamp());
     std::cout << "currEntry ~ Alt: " << currEntry.altitude_e
            << "ft,  CO2: " <<  currEntry.co2PPM_e
            << "PPM,  State: " << int(currEntry.currentState_e)
            << ",  TS: "  << currEntry.timestamp_e
            << ",  pyroEjected: "  << currEntry.pyroEjected_e << std::endl;
            std::cout << "currMD ~ timeStamp: " << currMD.lastTimeStamp
            << "ms,  State: " <<  int(currMD.currentState)
            << ",  pageNum: " << currMD.currentPageNum
            << ",  structCount: "  << int(currMD.structCount)
            << ",  pyroArmed: "  << currMD.pyroArmed 
            << ",  finalRead: "  << currMD.finalRead 
            << std::endl;

            Serial.print("Transition evaluated: ");
    Serial.println(transition); 


    Serial.println("Tick payload end");
    Serial.println("");Serial.println("");Serial.println("");
    

}



#endif // PAYLOAD_STATE_MACHINE_H