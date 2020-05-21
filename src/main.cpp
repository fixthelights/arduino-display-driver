/**
 * Blink
 *
 * Turns on an LED on for one second,
 * then off for one second, repeatedly.
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

// void setDisplayAddress(byte address, byte data){
//   for(int i = 0; i < sizeof(displayAddressMapping); i++){
//     if(displayAddressMapping[i] == address){
//       displayMemory[i] = data;
//     }
//   }
// }

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

  if(command | B00001111 == B00001111){
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

      colonRegisterUpdate = colonRegisterUpdate | addressPointer == 0x04;

      //intendedOutput[3 - Wire.available()] = number;

      if(addressPointer == 15) {
        addressPointer = 0;
      }else{
        addressPointer++;
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
  }
  Serial.println("-- End --");
}

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  DDRD = DDRD | B11111100;
  DDRB = B111111;

  PORTB = B011110;
  
  //Serial.begin(9600);
  Wire.begin(B11100000);
  Wire.onReceive(receiveEvent);
}

int NUMBERS [10][2] = {
  {B11101100, B000001},
  {B01100000, B000000},
  {B11011100, B000000},
  {B11111000, B000000},
  {B01110000, B000001},
  {B10111000, B000001},
  {B10111100, B000001},
  {B11100000, B000000},
  {B11111100, B000001},
  {B11111000, B000001}
};

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
}

int digital [] = {B111101, B111011, B110111, B101111};

// the loop function runs over and over again forever
void loop() {
  unsigned long preRun = micros();
  for(int i = 0; i < 1000; i++){
    for (int n = 0; n < 4; n++) {
      clearDisplay();
      clearPixel();
      PORTB = PORTB & digital[n];
      printNumber(displayAddressMapping[n]);
    }
  }
  unsigned long postRun = micros();
  unsigned long timeTaken = postRun - preRun;
  //Serial.println(timeTaken);
  
  int first = timeTaken / 1000;
  int second = (timeTaken % 1000) / 100;
  int third = (timeTaken % 100) / 10;
  int fourth = timeTaken % 10;

  intendedOutput[0] = first;
  intendedOutput[1] = second;
  intendedOutput[2] = third;
  intendedOutput[3] = fourth;

  if(timeTaken > 9999) {
    intendedOutput[0] = 0;
    intendedOutput[1] = 0;
    intendedOutput[2] = 0;
    intendedOutput[3] = 0;
  }
  //Serial.println(micros() - postRun);
  //delay(1000);
}