#include <stdint.h>

#include "avr/io.h"
#include "util/delay.h"

const byte analogPin = A0;
//const byte ledPin =  13;
const byte tonePin = 7;
const byte interruptPin = 2;

volatile uint8_t count = 0;

void click(void);

#ifndef abs
   #define abs(x) (((x)<0) ? -(x) : (x))
#endif

uint8_t ping_pong(uint8_t min, uint8_t max, uint8_t offset){
    uint8_t delta = (max - min);
    return min + abs(((offset + delta) % (delta << 1)) - delta);
}

void setup(void)
{  
  Serial.begin(9600);
  Serial.print("init...");
                
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), click, RISING);
}

void loop(void)                     
{
  Serial.println(ping_pong(0, 24, count));
  count++;
  
  _delay_ms(125);
}

////
void click(void){
  _delay_ms(150);

  Serial.print("click: ");
  uint16_t adc = analogRead(analogPin);

  Serial.print(adc);
  Serial.print(" ");

  Serial.println(ping_pong(0, 24, count));

  tone(tonePin, adc + 200, 500);
}