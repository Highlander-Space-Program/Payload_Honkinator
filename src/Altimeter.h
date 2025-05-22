#ifndef   ALTIMETER_H
#define  ALTIMETER_H

#include <Arduino.h>
#include <HardwareSerial.h>

//#define ALT_RX_PIN 13

extern HardwareSerial AltSerial;

//void initAltimeter();


// void initAltimeter() {
//     AltSerial.begin(9600, SERIAL_8N1, ALT_RX_PIN, -1);
// }

void readAltitude(int16_t *alt) {
    
    static char charBuff[32]; //These static variabels maintain their value even when the function is done and called again
    static uint8_t idx = 0; 
    char c; //will grab the char of the altimeter input

    while (AltSerial.available()) { //while the altimeter serial buffer has chars other than newline /n

        c = AltSerial.read(); //reads in the next available char from the serial buffer
        
        if (c == '\n') { //if that char is a newline (end of entry) then
        
            charBuff[idx] = '\0';    //terminates the buffer with a null for atof (look for a null terminated string)

            *alt = (int16_t)strtol(charBuff, NULL, 10);  // Convert string to float

            Serial.print("[Altimeter] Altitude reading: ");
            Serial.println(*alt);

            
            idx = 0; //reset the index since we finished the line
            memset(charBuff, 0, sizeof(charBuff)); //clear the buffer

            
        
        } else if (idx < sizeof(charBuff) - 1) { //if idx is within charBuff range (minus one for null)
            charBuff[idx] = c; 
            idx++;
        }
    }

}


#endif // ALTIMETER_H