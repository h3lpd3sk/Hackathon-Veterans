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

String dtext = "";
bool udisplay = true;

void setup(void)
{  
  //    
  Serial.begin(9600);
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), click, FALLING);

  //
  strip = Adafruit_NeoPixel(16, WS2812_PIN, NEO_GRB + NEO_KHZ800);
  strip.begin();
  strip.show();

  // initialize and clear display
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  
  
  setOled("WELCOME");
  
}

void setOled(String text)
{
  dtext = text;
  udisplay = true;
}

void updateOled()
{
  if(udisplay) {
    udisplay = false;
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(64 - (dtext.length()*6),32 - 6);
    display.print(dtext);
    display.display();
  }
}

typedef enum{
  state_idle,
  state_countdown,
  state_game_1,
} state_t;

state_t state = state_idle;

volatile int8_t offset;
uint8_t cw = true;

uint8_t adc;

void loop(void)                     
{
  uint8_t z;
  
  switch(state){
    case state_idle:
      //set difficulty (0-880)
      adc = (analogRead(analogPin) / 110) + 1;
      if(adc > 8) {
        adc = 8;
      }

      Serial.println(adc);
      _delay_ms(500);
      
      break;
    case state_game_1:
      if(cw){
        offset++;
      } else {
        offset--;
      }
      
      if(offset == 16){
        offset = 0;
      }

      if(offset == -1) {
        offset = 15;
      }

      if(offset == random(16))
      {
        cw = !cw;
      }
      
      uint8_t j;
      for(j=0; j<16; j++){
        strip.setPixelColor(j, 0, 0, 0);
      }
      strip.setPixelColor(0, 0, 255, 0);
      strip.setPixelColor(offset, 255, 0, 0); 
      strip.show();

      
      for(z=adc; z<9;z++){
        _delay_ms(20);
      }
      break;
    case state_countdown:
      z = 4;
      while(z--){
        if(z == 0){
          state = state_game_1;
          setOled("LET'S GO!");
          break;
        }
        _delay_ms(500);
        tone(tonePin, 1000/z, 200*(4-z));
        setOled(String(z));
        updateOled();
      }
      
      break;
  }

  updateOled();
}

//
void click(void){
  //_delay_ms(150);

  switch(state){
    case state_idle:
       
       state = state_countdown;
       break;
    case state_game_1:
       tone(tonePin, 1000, 200);
       state = state_idle;

       if(offset == 0) {
         setOled("HIT!");
       } else {
         setOled("MISS!");
        
       }
       Serial.print("click: ");
       Serial.println(offset);
  
       break;
  }
}
