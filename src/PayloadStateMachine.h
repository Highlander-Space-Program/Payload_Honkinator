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


*/

#include<Arduino.h>
#include "FlashMemory.h"
#include "MH-Z19.h"
#include "Altimeter.h"
#include "Globals.h"


bool stableAltitude (int16_t Alt) {
    static int stabilityCounter = 0;

    if (Alt < 10) {
        stabilityCounter++;
        
        if (stabilityCounter >= 20) {
            return true;
        }
    
    } else {
        stabilityCounter = 0 ;
    }
    return false;
}



Transitions checkTransition (int16_t currAlt, int16_t prevAlt){ 
    switch (currentState) {
        case ON_PAD:
            if (currAlt > prevAlt + 10) {
                return LAUNCHED;
            }
        break;

        case ASCENDING:
            if (!pyroArmed && currAlt >= 600) {
                return ASCENT_PASSED_600;
            }

            if (currAlt < prevAlt) {
                return APOGEE;
            }
        break;

        case DESCENDING:
            if (pyroArmed && currAlt <= 600) {
                return DESCENT_PASSED_600;
            }

            if (stableAltitude(currAlt)) {
                return TOUCHDOWN;
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

    if (finalRead == 0) {
        readAltitude(&currAltitude);
        getCO2(&ppmCO2);
        timeStamp = getTimeStamp();
        currEntry = {currAltitude, ppmCO2, timeStamp, uint8_t(currentState)};
    }

    currMD = {timeStamp, uint8_t(currentState), uint16_t(currPageNum), eBufIndx, useMD = true};

    transition = checkTransition (currAltitude, prevAltitude);

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
                if ((currPageNum >= 1) && (currPageNum <= 32750)/*between pages 1 and 32,750 */) {
                    writeEntryBuffToFlash (currPageNum, eBuff, sizeof(eBuff));
                    eBufIndx = 0;
                    currPageNum++;

                    eBuff[eBufIndx] = currEntry;
                    eBufIndx++;

                     

                } else if (currPageNum >= 32750/*last page*/) {
                    writeEntryBuffToFlash (32750, eBuff, sizeof(eBuff));
                    eBufIndx = 0;

                    eBuff[eBufIndx] = currEntry;
                    eBufIndx++;

                   

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
                if ((currPageNum >= 1) && (currPageNum <= 32750)/*between pages 1 and 32,750 */) {
                    writeEntryBuffToFlash (currPageNum, eBuff, sizeof(eBuff));
                    eBufIndx = 0;
                    currPageNum++;

                    eBuff[eBufIndx] = currEntry;
                    eBufIndx++;

                    //store metaData

                } else if (currPageNum >= 32750/*last page*/) {
                    writeEntryBuffToFlash (32750, eBuff, sizeof(eBuff));
                    eBufIndx = 0;

                    eBuff[eBufIndx] = currEntry;
                    eBufIndx++;

                    //store metaData

                }

                 writeMetaDataToFlash(&currMD);

            } 
        break;

        case LANDED:
            //read last data
            //store data + metaData
            //start car horn
            if (eBufIndx < (sizeof(eBuff) / sizeof(eBuff[0]))) {
                eBuff[eBufIndx] = currEntry;
                eBufIndx++;

                //store metaData
            
            } else {
                if (( (currPageNum >= 1) && (currPageNum <= 32750) ) && (eBufIndx > 0)/*between pages 1 and 32,766 && eBuffIndx > 0*/) {
                    writeEntryBuffToFlash (currPageNum, eBuff, sizeof(eBuff));
                    eBufIndx = 0;
                    currPageNum++;

                    eBuff[eBufIndx] = currEntry;
                    eBufIndx++;

                    //store metaData

                } else if (currPageNum >= 32750/*last page*/) {
                    writeEntryBuffToFlash (32750, eBuff, sizeof(eBuff));
                    eBufIndx = 0;

                    eBuff[eBufIndx] = currEntry;
                    eBufIndx++;

                    //store metaData

                }

            } 
            
            finalRead = true;
            writeMetaDataToFlash(&currMD);
        break;

    }

    prevAltitude = currAltitude;

}



#endif // PAYLOAD_STATE_MACHINE_H