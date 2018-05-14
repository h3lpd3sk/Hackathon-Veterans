#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// If using software SPI (the default case):
#define OLED_MOSI   9
#define OLED_CLK   10
#define OLED_DC    11
#define OLED_CS    12
#define OLED_RESET 13
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

#include <Adafruit_NeoPixel.h>
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

#define WS2812_PIN 6

void setup(void)
{  
  Serial.begin(9600);
  Serial.print("init...");
                
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), click, RISING);

  // Parameter 1 = number of pixels in strip
  // Parameter 2 = pin number (most are valid)
  // Parameter 3 = pixel type flags, add together as needed:
  //   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
  //   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
  //   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
  //   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
  Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, WS2812_PIN, NEO_GRB + NEO_KHZ400);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  strip.setPixelColor(0, 0xff, 0xff, 0xff); //rgb
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
