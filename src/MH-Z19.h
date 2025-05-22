#ifndef MH_Z19_H
#define  MH_Z19_H

#include<Arduino.h>
#include <HardwareSerial.h>
#include <stdint.h>

//#define MH_RX_PIN 22
//#define MH_TX_PIN 4

extern HardwareSerial MHZSerial; 

//CMD defined by the datasheet, 0x86 is the actual command
uint8_t getCO2_cmd[9] = { 0xFF, 0x01, 0x86, 0, 0, 0, 0, 0, 0x79 };

void getCO2 (uint16_t *valueCO2) {
  uint8_t receiveBuff[9];
  Serial.println("[MH-Z19] Requesting CO2...");

  
  MHZSerial.write(getCO2_cmd, 9);

  delay(30);
  if (MHZSerial.available() >= 9) {
    MHZSerial.read(receiveBuff, 9);

    if (receiveBuff[0] == 0xFF && receiveBuff[1] == 0x86) {
      *valueCO2 = ((uint16_t)receiveBuff[2] << 8) | receiveBuff[3];
      Serial.print("[MH-Z19] CO2 reading: ");
      Serial.println(*valueCO2);


    } else {
      *valueCO2 = 23;
      Serial.println("[MH-Z19] Invalid header in response, returning 23.");

    }

      
  }

}


#endif // MH_Z19_H