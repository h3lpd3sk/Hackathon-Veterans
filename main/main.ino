#include "avr/io.h"
#include "util/delay.h"
#include <stdint.h>
#include "notes.h"

#define SSD1306_128_64
#undef SSD1306_128_32
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

Adafruit_SSD1306 display(-1);
#if (SSD1306_LCDHEIGHT != 64)
   #error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

#include <Adafruit_NeoPixel.h>

#define OLED_ADDR 0x3C
#define ADC_1 A0
#define ADC_2 A1
#define TONE_PIN 7
#define ISR_1 19
#define ISR_2 2
#define ISR_3 3
#define RING_PIN 6
#define RING_COUNT 16
#define BAR_COUNT 8

Adafruit_NeoPixel neopixel;

//static void isr(void);
//void tone_russia(void);

typedef enum{
  state_idle,
  state_countdown,
  state_game_one,
  state_game_two,
  state_game_one_result
} state_t;

state_t game_states[2] = {
  state_game_one,
  state_game_two
};

////////////////////
void neopixel_black(Adafruit_NeoPixel neopixel, uint8_t count){
  uint8_t i;
  for(i=0;i<count;i++){
    neopixel.setPixelColor(i, 0, 0, 0);
  }
  neopixel.show();
}
/////////////////////
volatile state_t state = state_idle;
volatile int8_t offset;

#define oled_buffer_size 16
static char oled_buffer[oled_buffer_size];

volatile bool isr_update = false;
static uint8_t i; 
/////////////////////
void setup(void){  
  Serial.begin(9600);
  randomSeed(analogRead(ADC_1 + ADC_2));
  
  pinMode(ISR_1, INPUT_PULLUP);
  pinMode(ISR_2, INPUT_PULLUP);
  pinMode(ISR_3, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ISR_1), isr, FALLING);
  attachInterrupt(digitalPinToInterrupt(ISR_2), isr, FALLING);
  attachInterrupt(digitalPinToInterrupt(ISR_3), isr, FALLING);

  neopixel = Adafruit_NeoPixel(RING_COUNT, RING_PIN, NEO_GRB + NEO_KHZ800);
  neopixel.begin();
  neopixel.show();

  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
}

String dtext = "";
//bool udisplay = true;

void oled_set(String text){
  dtext = text;
  //udisplay = true;
}
void oled_render(void){
  //if(udisplay){
     //udisplay = false;
     display.clearDisplay();
     display.setTextSize(2);
     display.setTextColor(WHITE);
     display.setCursor(64 - (dtext.length()*6),32 - 6);
     display.print(dtext);
     display.display();
  //}
}

uint8_t cw = true;
uint8_t adc;

void loop(void)                     
{
  uint8_t z;
  uint8_t j;
  
  switch(state){
    case state_idle:
      oled_set("PLACE CARD");
      oled_render();

      //neopixel_black(neopixel, RING_COUNT);
      for(j=0; j<16; j++){
        neopixel.setPixelColor(j, 0, 0, 0);
      }
      neopixel.show();
      
      //set difficulty (0-880)
      adc = (analogRead(ADC_1) / 110) + 1;
      if(adc > 8) {
        adc = 8;
      }
      //Serial.print(adc);
      //Serial.print("---");
      //Serial.println(map(analogRead(ADC_1), 0, 1023, 0, 8));

      //Serial.println(adc);
      _delay_ms(500);
      
      break;
    case state_countdown:
      if(isr_update){
        isr_update = false;
        uint8_t r = random(0,2);

        for(i=3;i>0;i--){
           snprintf(oled_buffer, oled_buffer_size, "%d !", i);
           oled_set(oled_buffer);
           oled_render();

           tone(TONE_PIN, 1000/i, 200*(4-i));
           
           _delay_ms(750);
        }

        state = game_states[r];
        if(state == state_game_one)
        {
          oled_set("HIT TARGET");
        }

        if(state == state_game_two)
        {
          oled_set("START TAPPING");
        }
        
        
        oled_render();

        
      }
      break;
    case state_game_one:
        if(isr_update){
          isr_update = false;
          if(cw){
            offset++;
          }
          else {
            offset--;
          }
          if(offset == RING_COUNT){
            offset = 0;
          }
          if(offset == -1) {
            offset = (RING_COUNT - 1);
          }
          if(offset == random(RING_COUNT)){
            cw = !cw;
          }
          
          for(j=0; j<RING_COUNT; j++){
            neopixel.setPixelColor(j, 0, 0, 0);
          }
          neopixel.setPixelColor(0, 0, 255, 0);
          neopixel.setPixelColor(offset, 255, 0, 0); 
          neopixel.show();
    
          for(z=adc; z<9;z++){
            _delay_ms(20);
          }
        }
    break;
    case state_game_two:

      if(isr_update){
        isr_update = false;

        //beep
        tone(TONE_PIN, 300 + (offset * 50), 50);
  
        //display
        snprintf(oled_buffer, oled_buffer_size, "%d !", offset);
        oled_set(oled_buffer);
        oled_render();
  
        //leds
        for(i=0; i < RING_COUNT; i++){
          if(i < offset){
            neopixel.setPixelColor(i, 0, 0, 255);
          }
          else {
            neopixel.setPixelColor(i, 0, 0, 0);
          }
        }
        neopixel.show();
  
        //complete
        if(offset >= RING_COUNT){
            offset = 0;
            //reset
            for(i=0; i < RING_COUNT; i++){
              neopixel.setPixelColor(i, 0, 0, 0);
            }
            neopixel.show();
            
            tone_russia();  
            state = state_idle;
            break;
        }
      }
      break;
    case state_game_one_result:
    
       if(offset == 0) {
         oled_set("HIT!");

         tone_usa();
       }
       else {
         oled_set("MISS!");

         tone_usa();
       }
       oled_render();
       
       _delay_ms(1000);

       state = state_idle;
      break; 
  }
}

static void isr(void){
  noInterrupts();

  //Main button
  if(digitalRead(ISR_1)){
     switch(state){
       case state_idle:
          isr_update = true;
          state = state_countdown;
          break;
       default:
          Serial.println(".....");
          break;
     }
  }
  //Player one
  else if(digitalRead(ISR_2)){
     switch(state){
        case state_game_one:
          isr_update = true;
          
          tone(TONE_PIN, 1000, 200);
          state = state_game_one_result;
    
          break;
        case state_game_two: 
          isr_update = true;
          offset++;
          break;
        default:
          Serial.print("click: ");
          Serial.println(state);
     } 
  }
  else if(digitalRead(ISR_3)){
    Serial.println("_______________");
  }
  interrupts();
}


void tone_russia(void){
  const uint16_t notes[] = {
    NOTE_G4, NOTE_C5, NOTE_G4, NOTE_A4, NOTE_B4, NOTE_E4, NOTE_E4,
    NOTE_A4, NOTE_G4, NOTE_F4, NOTE_G4, NOTE_C4, NOTE_C4, NOTE_D4, 
    NOTE_D4, NOTE_E4, NOTE_F4, NOTE_F4, NOTE_G4, NOTE_A4, NOTE_B4,
    NOTE_C5, NOTE_D5
  };
  const uint8_t durations[] = {
    16, 8, 16, 32, 8, 16, 16, 
    8, 16, 32, 8, 16, 16, 8, 
    16, 32, 8, 16, 32, 8, 16, 
    16, 8
  };

  const size_t notes_count = sizeof(notes)/sizeof(uint16_t);
  uint8_t i;
  uint16_t duration;

  for(i = 0; i < notes_count; i++) {
    duration = 1500/durations[i];
    tone(TONE_PIN, notes[i], duration);
    delay(duration * 2);
    noTone(TONE_PIN);
  }
}
void tone_usa(void){
  const uint16_t notes[] = {
    NOTE_G4, NOTE_E4, NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5,
    NOTE_E5, NOTE_D5, NOTE_C5, NOTE_E4, NOTE_F4, NOTE_G4,
    NOTE_G4, NOTE_G4, NOTE_E5, NOTE_D5, NOTE_C5, NOTE_B4,
    NOTE_A4, NOTE_B4, NOTE_C5, NOTE_C5, NOTE_G4, NOTE_E4,
    NOTE_C4
  };
  const uint8_t durations[] = {
    8,32,8,8,8,4,
    16,32,8,8,8,4,
    16,16,8,8,8,4,
    16,32,8,8,8,8,
    8
  };

  const size_t notes_count = sizeof(notes)/sizeof(uint16_t);
  uint8_t i;
  uint16_t duration;

  for(i = 0; i < notes_count; i++) {
    duration = 1500/durations[i];
    tone(TONE_PIN, notes[i], duration);
    delay(duration * 2);
    noTone(TONE_PIN);
  }
}

