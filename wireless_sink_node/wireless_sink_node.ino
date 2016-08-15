// **********************************************************************************
// This sketch is an example of how wireless programming can be achieved with a Moteino
// that was loaded with a custom 1k bootloader (DualOptiboot) that is capable of loading
// a new sketch from an external SPI flash chip
// The sketch includes logic to receive the new sketch 'over-the-air' and store it in
// the FLASH chip, then restart the Moteino so the bootloader can continue the job of
// actually reflashing the internal flash memory from the external FLASH memory chip flash image
// The handshake protocol that receives the sketch wirelessly by means of the RFM69 radio
// is handled by the SPIFLash/WirelessHEX69 library, which also relies on the RFM69 library
// These libraries and custom 1k Optiboot bootloader are at: http://github.com/lowpowerlab
// **********************************************************************************
// Copyright Felix Rusu, LowPowerLab.com
// Library and code by Felix Rusu - felix@lowpowerlab.com
// **********************************************************************************
// License
// **********************************************************************************
// This program is free software; you can redistribute it
// and/or modify it under the terms of the GNU General
// Public License as published by the Free Software
// Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will
// be useful, but WITHOUT ANY WARRANTY; without even the
// implied warranty of MERCHANTABILITY or FITNESS FOR A
// PARTICULAR PURPOSE. See the GNU General Public
// License for more details.
//
// You should have received a copy of the GNU General
// Public License along with this program.
// If not, see <http://www.gnu.org/licenses/>.
//
// Licence can be viewed at
// http://www.gnu.org/licenses/gpl-3.0.txt
//
// Please maintain this license information along with authorship
// and copyright notices in any redistribution of this code
// **********************************************************************************
#include <RFM69.h>         //get it here: https://www.github.com/lowpowerlab/rfm69
#include <SPI.h>
#include <SPIFlash.h>      //get it here: https://www.github.com/lowpowerlab/spiflash
#include <avr/wdt.h>
#include <WirelessHEX69.h> //get it here: https://github.com/LowPowerLab/WirelessProgramming/tree/master/WirelessHEX69

#define NETWORKID   164
#define GATEWAYID   1
#define NODEID      3       // node ID used for this unit
//Match frequency to the hardware version of the radio on your Moteino (uncomment one):
#define FREQUENCY   RF69_433MHZ
//#define FREQUENCY   RF69_868MHZ
//#define FREQUENCY     RF69_915MHZ
#define IS_RFM69HW  //uncomment only for RFM69HW! Leave out if you have RFM69W!
#define SERIAL_BAUD 115200
#define ACK_TIME    30  // # of ms to wait for an ack
#define ENCRYPTKEY "sampleEncryptKey" //(16 bytes of your choice - keep the same on all encrypted nodes)
#define BLINKPERIOD 500

#ifdef __AVR_ATmega1284P__
#define LED           15 // Moteino MEGAs have LEDs on D15
#define FLASH_SS      23 // and FLASH SS on D23
#else
#define LED           9 // Moteinos hsave LEDs on D9
#define FLASH_SS      8 // and FLASH SS on D8
#endif

RFM69 radio;
char input = 0;
long lastPeriod = -1;
int TRANSMITPERIOD = 500; //transmit a packet to gateway so often (in ms)
byte sendSize=0;
boolean requestACK = false;
int emptytime = 20000;//time to empty different per container
int wooshtime = 20000;

/////////////////////////////////////////////////////////////////////////////
// flash(SPI_CS, MANUFACTURER_ID)
// SPI_CS          - CS pin attached to SPI flash chip (8 in case of Moteino)
// MANUFACTURER_ID - OPTIONAL, 0x1F44 for adesto(ex atmel) 4mbit flash
//                             0xEF30 for windbond 4mbit flash
//                             0xEF40 for windbond 16/64mbit flash
/////////////////////////////////////////////////////////////////////////////
SPIFlash flash(FLASH_SS, 0xEF30); //EF30 for windbond 4mbit flash

typedef struct {
  int           nodeId; //store this nodeId
  unsigned long uptime; //uptime in ms
  float         data;   //temperature maybe?
} Payload;
Payload theData;

void setup() {
  pinMode(LED, OUTPUT);
  Serial.begin(SERIAL_BAUD);
  radio.initialize(FREQUENCY, NODEID, NETWORKID);
  radio.encrypt(ENCRYPTKEY); //OPTIONAL
#ifdef IS_RFM69HW
  radio.setHighPower(); //only for RFM69HW!
#endif
  Serial.print("Start node...");
  char buff[50];
  sprintf(buff, "\nTransmitting at %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.println(buff);

  if (flash.initialize())
    Serial.println("SPI Flash Init OK!");
  else
    Serial.println("SPI Flash Init FAIL!");
  pinMode(3, OUTPUT);//Drain
  pinMode(4, INPUT);//pinkdrain--feedback wires representing open and closed
  pinMode(5, INPUT);//graydrain
  pinMode(6, OUTPUT);//Vent
  pinMode(7, INPUT);//pinkvent
  pinMode(9, INPUT);//grayvent
  pinMode(14, OUTPUT);//Air
  pinMode(15, INPUT);//pinkair
  pinMode(16, INPUT);//grayair

  pinMode(17, INPUT);//sensor
}

void loop() {
  delay(500);
  int sensorValue = digitalRead(17);
  int pinkdrain = digitalRead(4);
  int graydrain = digitalRead(5);
  int pinkvent = digitalRead(7);
  int grayvent = digitalRead(9);
  int pinkair = digitalRead(15);
  int grayair = digitalRead(16);
  bool valveCheck = false;

  //status();//subroutine to read status of valves shown at bottom of code
  //Serial.print(sensorValue);
  //    digitalWrite(3, HIGH);//drain
  //    digitalWrite(6, HIGH);//Vent
  //    digitalWrite(14, HIGH);//air
  sensorValue = digitalRead(17);
  if (sensorValue == 0) {
    //when sensor is dry and air is closed
    digitalWrite(3, LOW);//drain
    digitalWrite(6, LOW);//Vent
    digitalWrite(14, LOW);//air
  }

  else if (sensorValue == 1) { //empty cycle
    Serial.println("emptying");
    digitalWrite(3, HIGH);//drain
    digitalWrite(6, HIGH);//Vent
    while (valveCheck == false) { //wait for drain and vent valves to be closed
      pinkdrain = digitalRead(4);
      graydrain = digitalRead(5);
      pinkvent = digitalRead(7);
      grayvent = digitalRead(9);
      if ((graydrain == 1) && (pinkdrain == 0) && (grayvent == 1) && (pinkvent == 0)) {
        valveCheck = true;
      }
    }
    valveCheck = false;

    digitalWrite(14, HIGH);//air
    delay(emptytime);//time to empty set at top of code
    delay(emptytime);
    delay(emptytime);
    delay(emptytime);
    delay(emptytime);
    delay(emptytime);
    delay(emptytime);//2min20sec

    digitalWrite(14, LOW);//air
    while (valveCheck == false) { //wait for air to be closed
      pinkair = digitalRead(15);
      grayair = digitalRead(16);
      if ((pinkair == 1) && (grayair == 0)) {
        valveCheck = true;
      }
    }
    valveCheck = false;
    delay(wooshtime);//time for pressure to dissipate

    digitalWrite(6, LOW);//vent//when air is closed, open vent release pressrue
    while (valveCheck == false) {
      pinkvent = digitalRead(7);
      grayvent = digitalRead(9);
      if ((pinkvent == 1) && (grayvent == 0)) {
        valveCheck = true;
      }
    }
    valveCheck = false;

    digitalWrite(3, LOW);//drain
    while (valveCheck == false) {
      pinkdrain = digitalRead(4);
      graydrain = digitalRead(5);
      if ((pinkdrain == 1) && (graydrain == 0)) {
        valveCheck = true;
      }
    }
    valveCheck = false;
    //}else{digitalWrite(4,LOW);}
  }

  if (Serial.available() > 0) {
    input = Serial.read();
    if (input == 'd') //d=dump first page
    {
      Serial.println("Flash content:");
      int counter = 0;

      while (counter <= 256) {
        Serial.print(flash.readByte(counter++), HEX);
        Serial.print('.');
      }

      Serial.println();
    }
    else if (input == 'e')
    {
      Serial.print("Erasing Flash chip ... ");
      flash.chipErase();
      while (flash.busy());
      Serial.println("DONE");
    }
    else if (input == 'i')
    {
      Serial.print("DeviceID: ");
      Serial.println(flash.readDeviceId(), HEX);
    }
    else if (input == 'r')
    {
      Serial.print("Rebooting");
      resetUsingWatchdog(true);
    }
    else if (input == 'R')
    {
      Serial.print("RFM69 registers:");
      radio.readAllRegs();
    }
    else if (input >= 48 && input <= 57) //0-9
    {
      Serial.print("\nWriteByte("); Serial.print(input); Serial.print(")");
      flash.writeByte(input - 48, millis() % 2 ? 0xaa : 0xbb);
    }
  }

  // Check for existing RF data, potentially for a new sketch wireless upload
  // For this to work this check has to be done often enough to be
  // picked up when a GATEWAY is trying hard to reach this node for a new sketch wireless upload
  if (radio.receiveDone())
  {
    Serial.print("Got [");
    Serial.print(radio.SENDERID);
    Serial.print(':');
    Serial.print(radio.DATALEN);
    Serial.print("] > ");
    for (byte i = 0; i < radio.DATALEN; i++)
      Serial.print((char)radio.DATA[i], HEX);
    Serial.println();
    CheckForWirelessHEX(radio, flash, true);
    Serial.println();
  }
  
  if (radio.receiveDone())
  {
    Serial.print('[');Serial.print(radio.SENDERID, DEC);Serial.print("] ");
    for (byte i = 0; i < radio.DATALEN; i++)
      Serial.print((char)radio.DATA[i]);
    Serial.print("   [RX_RSSI:");Serial.print(radio.readRSSI());Serial.print("]");

    if (radio.ACKRequested())
    {
      radio.sendACK();
      Serial.print(" - ACK sent");
      delay(10);
    }
    Blink(LED,5);
    Serial.println();
  }
  
  int currPeriod = millis()/TRANSMITPERIOD;
  if (currPeriod != lastPeriod)
  {
    //fill in the struct with new values
    theData.nodeId = NODEID;
    theData.uptime = millis();
    theData.data = 123.4;
    
    Serial.print("Sending struct (");
    Serial.print(sizeof(theData));
    Serial.print(" bytes) ... ");
    if (radio.sendWithRetry(GATEWAYID, (const void*)(&theData), sizeof(theData)))
      Serial.print(" ok!");
    else Serial.print(" nothing...");
    Serial.println();
    Blink(LED,3);
    lastPeriod=currPeriod;
  }
  ////////////////////////////////////////////////////////////////////////////////////////////
}

void Blink(byte PIN, int DELAY_MS)
{
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN,LOW);
}
