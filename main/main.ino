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
#define RING_PIN1 6
#define RING_PIN2 8

#define RING_COUNT 24
#define BAR_COUNT 8

Adafruit_NeoPixel neopixel;


typedef enum{
  state_idle,
  state_countdown,
  state_game_one,
  state_game_two,
  state_game_one_result
} state_t;

typedef struct{
  Adafruit_NeoPixel* ring;
  Adafruit_NeoPixel* bar;
  int8_t offset;
  int8_t round_score;
  uint8_t diff;
  uint8_t cw;
  uint8_t tick;
  uint8_t isr_update;
} player_t;

static player_t players[2];

state_t game_states[2] = {
  state_game_one,
  state_game_two
};

/////////////////////
volatile state_t state = state_idle; 

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
  attachInterrupt(digitalPinToInterrupt(ISR_1), isr_p1, FALLING);
  attachInterrupt(digitalPinToInterrupt(ISR_2), isr_main, FALLING);
  attachInterrupt(digitalPinToInterrupt(ISR_3), isr_p2, FALLING);

static Adafruit_NeoPixel pix0 = Adafruit_NeoPixel(RING_COUNT, RING_PIN1, NEO_GRB + NEO_KHZ800);
static Adafruit_NeoPixel pix1 = Adafruit_NeoPixel(RING_COUNT, RING_PIN2, NEO_GRB + NEO_KHZ800);

  //setup players
  player_t* p1 = &players[0];
  p1->diff = 1;
  p1->cw = true;
  p1->tick = 0;
  p1->isr_update = false;
  p1->round_score = -1;
  p1->ring = &pix0;
  p1->ring->begin();
  p1->ring->show();
  //players[0] = p1;

  player_t* p2 = &players[1];
  p2->diff = 1;
  p2->cw = true;
  p2->tick = 0;
  p2->isr_update = false;
  p2->round_score = -1;
  p2->ring = &pix1;
  p2->ring->begin();
  p2->ring->show();
  //players[1] = p2;
  
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  display.display();
}

void oled_render(String text){
//     display.clearDisplay();
//     display.setTextSize(2);
//     display.setTextColor(WHITE);
//     display.setCursor(64 - (text.length()*6),32 - 6);
//     display.print(text);
//     display.display();
Serial.println(text);
}

//lelijk var voor game1
uint32_t game2start = 0;
uint8_t finished = false;

void loop(void)                     
{
  uint8_t z;
  uint8_t j;
  
  switch(state){
    case state_idle:
      //oled_render("PLACE CARD");

      for(j=0; j<RING_COUNT; j++){
        players[0].ring->setPixelColor(j, 0, 0, 0);
        players[1].ring->setPixelColor(j, 0, 0, 0);
      }
      players[0].ring->show();
      players[1].ring->show();
      set_difficulty();
      //reset player vars
      players[0].offset = 0;
      players[1].offset = 0;
      players[0].round_score = -1;
      players[1].round_score = -1;
      players[0].tick = 0;
      players[1].tick = 0;
      
      _delay_ms(500);
      break;
    case state_countdown:
      
      for(i=3;i>0;i--){
         snprintf(oled_buffer, oled_buffer_size, "%d !", i);
         oled_render(oled_buffer);

         tone(TONE_PIN, 1000/i, 200*(4-i));
         
         _delay_ms(750);
      }

      state = game_states[random(0,2)];
      state = state_game_one;
      
      if(state == state_game_one)
      {
        oled_render("HIT TARGET"); 
      }

      if(state == state_game_two)
      {
        oled_render("TAP FAST!"); 
        game2start = millis();
      }

      finished = false;
      
      break;
    case state_game_one:
    //update for player 1 and 2
          finished = false;
          for(i=0; i < 2; i++){
            players[i].tick++;
            
            if(players[i].round_score == -1) {
              finished = false;
            if(players[i].isr_update){
              players[i].isr_update = false;
              Serial.println("x");
              tone(TONE_PIN, 1000, 200);
              Serial.println("z");
              if(players[i].offset == 0){
                 oled_render("HIT!");
               }
               else {
                 oled_render("MISS!");
               }
               Serial.println("y");

               if(players[i].offset >= RING_COUNT / 2)
               {
                players[i].round_score = RING_COUNT - players[i].offset; 
               } else {
                players[i].round_score = players[i].offset;
               }

              
            } else {
                  if(players[i].tick >= (10 - players[i].diff)) {
                    players[i].tick = 0;
                    if(players[i].cw){
                      players[i].offset++;
                    }
                    else {
                      players[i].offset--;
                    }
                    if(players[i].offset == RING_COUNT){
                      players[i].offset = 0;
                    }
                    if(players[i].offset == -1) {
                      players[i].offset = (RING_COUNT - 1);
                    }
                    if(players[i].offset == random(RING_COUNT)){
                      players[i].cw = !players[i].cw;
                    }
                    
                    for(j=0; j<RING_COUNT; j++){
                      players[i].ring->setPixelColor(j, 0, 0, 0);
                    }
                    players[i].ring->setPixelColor(0, 0, 255, 0);
                    players[i].ring->setPixelColor(players[i].offset, 255, 0, 0); 
                    players[i].ring->show(); 
            
                  }
                   _delay_ms(10);
              }
            } else {
              finished = true;
              players[i].isr_update = false;
              
            }
          }

          if(finished) {
              _delay_ms(1000);
               state = state_idle;
          }
          
    break;
    case state_game_two:
        finished = false;
        
        for(i=0; i < 2; i++){
          if(players[i].round_score == -1){
              finished = false;
            
              if(players[i].isr_update){
                    players[i].isr_update = false;
                    players[i].offset++;
    
                    uint8_t final = RING_COUNT + players[i].diff;
                    uint8_t fill = map(players[i].offset, 0, final, 0, RING_COUNT);
    
                    Serial.println(fill);
                    
                    //beep
                    tone(TONE_PIN, 300 + (players[i].offset * 50), 50);

                    //leds
                    for(j=0; j < RING_COUNT; j++){
                      if(j < fill){
                        players[i].ring->setPixelColor(j, 0, 0, 255);
                      }
                      else {
                        players[i].ring->setPixelColor(j, 0, 0, 0);
                      }
                    }
                    players[i].ring->show();
              
                    //complete
                    if(players[i].offset >= RING_COUNT){
                        
                        players[i].round_score = game2start - millis();

                        //display
                        snprintf(oled_buffer, oled_buffer_size, "%d!", players[i].round_score);
                        oled_render(oled_buffer);
                        
                        break;
                    }
    
            }
          } else {
              finished = true;
              players[i].isr_update = false;
              
            }
      }

      if(finished) {
              _delay_ms(1000);
               state = state_idle;
          }
          
      break;
  }
}

void set_difficulty(void) {
  //set difficulty (0-1024)
  uint8_t adc = (analogRead(ADC_1) / (1024/8)) + 1;
  if(adc > 8) {
    adc = 8;
  }

  players[1].diff = adc;

  adc = (analogRead(ADC_2) / (1024/8)) + 1;
  if(adc > 8) {
    adc = 8;
  }

  players[0].diff = adc;
  
}

static void isr_main(void){
  noInterrupts();

Serial.print("click: main"); 
Serial.print(state); 

  //Main button
  //if(digitalRead(ISR_1)){
     switch(state){
       case state_idle:
          state = state_countdown;
          break;
       default:
          break;
     }
  //}
  //Player input
  interrupts();
  
}

static void isr_p1(void){
  noInterrupts();
//else if(digitalRead(ISR_2) || digitalRead(ISR_3)){
    players[0].isr_update = true;
        Serial.print("click: p1"); 
  
  
  interrupts();
}

static void isr_p2(void){
  noInterrupts();
//else if(digitalRead(ISR_2) || digitalRead(ISR_3)){
    players[1].isr_update = true;
    Serial.print("click: p2");
  
  
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

