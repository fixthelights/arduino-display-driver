/**
 * 
 * Arduino as Display Driver
 *
 * Modeled after https://www.adafruit.com/product/879
 * Compatible with https://github.com/adafruit/Adafruit_LED_Backpack
 * Implemented using https://cdn-shop.adafruit.com/datasheets/ht16K33v110.pdf
 * 
 */

#include "Arduino.h"
#include "Wire.h"

byte intendedOutput[4] = {2,2,2,6};
byte displayMemory[16] = {0};
byte displayMemoryPortD[16] = {0};
byte displayMemoryPortB[16] = {0};

// Full display address mapping (only four will be utilized)
// int displayAddressMapping[16] = {
//   0x0,
//   0x2,
//   0x4,
//   0x6,
//   0x8,
//   0xA,
//   0xC,
//   0xE,
//   0x1,
//   0x3,
//   0x5,
//   0x7,
//   0x9,
//   0xB,
//   0xD,
//   0xF
// };

int displayAddressMapping[4] = {
  0x0,
  0x2,
  0x6,
  0x8
};

byte pinMapper[8][2] = {
  {0, 7},
  {0, 6},
  {0, 5},
  {0, 3},
  {0, 2},
  {1, 0},
  {0, 4},
  {1, 5}
};

byte portDRegisterMapper(byte displayData){
  byte result = 0;
  for(int i = 0; i < 8; i++){
    bool state = (bool)bitRead(displayData,i);
    if(state && pinMapper[i][0] == 0){
      bitSet(result,pinMapper[i][1]);
    }
  }
  return result;
}

byte portBRegisterMapper(byte displayData){
  byte result = 0;
  for(int i = 0; i < 8; i++){
    bool state = (bool)bitRead(displayData,i);
    if(state && pinMapper[i][0] == 1){
     bitSet(result,pinMapper[i][1]);
    }
  }
  return result;
}

void receiveEvent(int howMany){
  Serial.print("Wire.available(): ");
  Serial.println(Wire.available());

  byte command = Wire.read();
  Serial.print("Command: ");
  Serial.println(command);

  if((command | B00001111) == B00001111){
    byte addressPointer = command & B00001111;
    boolean colonRegisterUpdate = false;

    while(Wire.available()){
      int data = Wire.read();
      Serial.print("Address pointer: ");
      Serial.println(addressPointer);
      displayMemory[addressPointer] = data;
      Serial.print("Data: ");
      Serial.println(data, BIN);
      Serial.print("Mapped PORTD: ");
      Serial.println(portDRegisterMapper(data), BIN);
      displayMemoryPortD[addressPointer] = portDRegisterMapper(data);
      Serial.print("Mapped PORTB: ");
      Serial.println(portBRegisterMapper(data), BIN);
      displayMemoryPortB[addressPointer] = portBRegisterMapper(data);

      if(addressPointer == 0x04){
        colonRegisterUpdate = true;
      }

      if(addressPointer++ == 0x0F) {
        addressPointer = 0;
      }
    }

    if(colonRegisterUpdate && displayMemory[0x04] == B00000010){
      displayMemoryPortB[0x02] = portBRegisterMapper(displayMemory[0x02] | B10000000);
      displayMemoryPortB[0x06] = portBRegisterMapper(displayMemory[0x06] | B10000000);
    }else if(colonRegisterUpdate && displayMemory[0x04] == B00000000){
      displayMemoryPortB[0x02] = portBRegisterMapper(displayMemory[0x02]);
      displayMemoryPortB[0x06] = portBRegisterMapper(displayMemory[0x06] & B01111111);
    }else{
      displayMemoryPortB[0x06] = portBRegisterMapper(displayMemory[0x06] & B01111111);
    }
  }else if((command & B11100000) == B11100000){
    Serial.print("Raw Brightness: ");
    Serial.println(command & B00001111,BIN);
    Serial.print("Brightness: ");
    Serial.println((command & B00001111) * 16,BIN);
    OCR2A = (command & B00001111) * 16; 
  }
  Serial.println("-- End --");
}

// the setup function runs once when you press reset or power the board
void setup() {
  DDRD = DDRD | B11111100;
  DDRB = B111111;
  DDRC = B111111;

  PORTB = B011110;
  
  TCCR2A = _BV(COM2A1) | _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(CS20);
  OCR2A = 50;

  //Serial.begin(9600);
  Wire.begin(B11100000);
  Wire.onReceive(receiveEvent);
}

void printNumber(byte addressPointer) {
  // PORTD = NUMBERS[number][0];
  // PORTB = PORTB | NUMBERS[number][1];
  PORTD = displayMemoryPortD[addressPointer];
  PORTB = PORTB | displayMemoryPortB[addressPointer];
}

void clearDisplay() {
  PORTD = PORTD & B00000011;
  PORTB = PORTB & B011110;
}

void clearPixel() {
  PORTB = PORTB | B011110;
  PORTC = PORTC & B110000;
}

int digital [] = {B111101, B111011, B110111, B101111};
int analog [] = {B001000, B000100, B000010, B000001};

// the loop function runs over and over again forever
void loop() {
  unsigned long preRun = micros();
  for(int i = 0; i < 1000; i++){
    for (int n = 0; n < 4; n++) {
      clearDisplay();
      clearPixel();
      PORTC = PORTC | analog[n];
      printNumber(displayAddressMapping[n]);
    }
  }
  unsigned long postRun = micros();
  unsigned long timeTaken = postRun - preRun;
  //Serial.println(timeTaken);
  //Serial.println(micros() - postRun);
  //delay(1000);
}