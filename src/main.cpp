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

byte intendedOutput[4] = {2, 2, 2, 6};
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
    0x8};

byte pinMapper[8][2] = {
    {0, 6},
    {1, 2},
    {0, 5},
    {0, 4},
    {1, 3},
    {0, 2},
    {0, 3},
    {1, 0}};

struct SegmentPorts
{
  byte portB;
  byte portD;
};

SegmentPorts registerMapper(byte displayData){
  SegmentPorts result = {0, 0};
  for (int i = 0; i < 8; i++)
  {
    if(bitRead(displayData, i)){
      byte portName = pinMapper[i][0];
      if(portName == 0){
        bitSet(result.portD, pinMapper[i][1]);
      }else if(portName == 1){
        bitSet(result.portB, pinMapper[i][1]);
      }
    }
  }
  return result;
}

void receiveEvent(int howMany)
{
  // Serial.print("Wire.available(): ");
  // Serial.println(Wire.available());

  byte command = Wire.read();
  byte commandValue = command & B11110000;
  // Serial.print("Command: ");
  // Serial.println(command);

  if (commandValue == B00000000)
  {
    // Set display's memory

    byte addressPointer = command & B00001111;
    bool colonRegisterUpdate = false;

    while (Wire.available())
    {
      int data = Wire.read();
      // Serial.print("Address pointer: ");
      // Serial.println(addressPointer);
      displayMemory[addressPointer] = data;
      // Serial.print("Data: ");
      // Serial.println(data, BIN);
      // Serial.print("Mapped PORTD: ");
      SegmentPorts segPorts = registerMapper(data);
      // Serial.println(segPorts.portD, BIN);
      displayMemoryPortD[addressPointer] = segPorts.portD;
      // Serial.print("Mapped PORTB: ");
      // Serial.println(segPorts.portB, BIN);
      displayMemoryPortB[addressPointer] = segPorts.portB;

      if (addressPointer == 0x04)
      {
        colonRegisterUpdate = true;
      }

      if (addressPointer++ == 0x0F)
      {
        addressPointer = 0x00;
      }
    }

    // Toggle display's colon based on memory updates

    if (colonRegisterUpdate && displayMemory[0x04] == B00000010)
    {
      displayMemoryPortB[0x02] = registerMapper(displayMemory[0x02] | B10000000).portB;
      displayMemoryPortB[0x06] = registerMapper(displayMemory[0x06] | B10000000).portB;
    }
    else if (colonRegisterUpdate && displayMemory[0x04] == B00000000)
    {
      displayMemoryPortB[0x02] = registerMapper(displayMemory[0x02]).portB;
      displayMemoryPortB[0x06] = registerMapper(displayMemory[0x06] & B01111111).portB;
    }
    else
    {
      displayMemoryPortB[0x06] = registerMapper(displayMemory[0x06] & B01111111).portB;
    }
  }
  else if (commandValue == B11100000)
  {
    // Set display's brightness

    // Serial.print("Raw Brightness: ");
    // Serial.println(command & B00001111, BIN);
    // Serial.print("Brightness: ");
    // Serial.println((command & B00001111) * 16, BIN);
    OCR1A = (command & B00001111) * 16;
  }
  // Serial.println("-- End --");
}

// the setup function runs once when you press reset or power the board
void setup()
{
  DDRD = DDRD | B11111100;
  DDRB = B111111;
  DDRC = B111111;

  TCCR1A = _BV(COM1A1) | _BV(WGM12) | _BV(WGM10);
  TCCR1B = _BV(CS21);
  OCR1A = 16;

  // Serial.begin(9600);
  Wire.begin(B11100000);
  Wire.onReceive(receiveEvent);
}

void printNumber(byte addressPointer)
{
  PORTD = PORTD | displayMemoryPortD[addressPointer];
  PORTB = PORTB | displayMemoryPortB[addressPointer];
}

void clearDisplay()
{
  PORTD = PORTD & B10000011;
  PORTB = PORTB & B110010;
}

void clearPixel()
{
  PORTC = PORTC & B110000;
}

int analog[] = {B000001, B000010, B000100, B001000};

// the loop function runs over and over again forever
void loop()
{
  for (int n = 0; n < 4; n++)
    {
      clearDisplay();
      clearPixel();
      PORTC = PORTC | analog[n];
      printNumber(displayAddressMapping[n]);
      delayMicroseconds(110);
    }
}