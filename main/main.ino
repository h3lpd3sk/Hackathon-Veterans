#define SSD1306_128_64
#undef SSD1306_128_32
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#define OLED_ADDR   0x3C

Adafruit_SSD1306 display(-1);

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

#include <Adafruit_NeoPixel.h>
#include <stdint.h>

#include "avr/io.h"
#include "util/delay.h"

#define analogPin A0
//const byte ledPin  13;
#define tonePin 7
#define interruptPin 19

#define WS2812_PIN 6

//volatile uint8_t count = 0;

void click(void);

//#ifndef abs
//   #define abs(x) (((x)<0) ? -(x) : (x))
//#endif

//uint8_t ping_pong(uint8_t min, uint8_t max, uint8_t offset){
//    uint8_t delta = (max - min);
//    return min + abs(((offset + delta) % (delta << 1)) - delta);
//}


Adafruit_NeoPixel strip;

void setup(void)
{  
  //    
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), click, FALLING);

  //
  strip = Adafruit_NeoPixel(16, WS2812_PIN, NEO_GRB + NEO_KHZ800);
  strip.begin();
  strip.show();

  // initialize and clear display
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  display.display();

  
  // display a pixel in each corner of the screen
  display.drawPixel(0, 0, WHITE);
  display.drawPixel(127, 0, WHITE);
  display.drawPixel(0, 63, WHITE);
  display.drawPixel(127, 63, WHITE);

  // display a line of text
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(27,30);
  display.print("Hello, world!");

  // update display with all of the above graphics
  display.display();
  
}

typedef enum{
  state_idle,
  state_game_1,
} state_t;

state_t state = state_idle;

volatile uint8_t offset;

void loop(void)                     
{
  switch(state){
    case state_idle:
    
      break;
    case state_game_1:
      offset++;
      if(offset == 16){
        offset = 0;
      }
      
      uint8_t j;
      for(j=0; j<16; j++){
        strip.setPixelColor(j, 0, 0, 0);
      }
      strip.setPixelColor(0, 0, 255, 0);
      strip.setPixelColor(offset, 255, 0, 0); 
      strip.show();
      _delay_ms(20);

      break;
  }
}

//
void click(void){
  _delay_ms(150);

  switch(state){
    case state_idle:
       state = state_game_1;
       break;
    case state_game_1:
       tone(tonePin, 1000, 200);
       state = state_idle;

       Serial.print("click: ");
       Serial.println(offset);
  
       break;
  }
}
