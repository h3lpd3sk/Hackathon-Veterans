#include "avr/io.h"
#include "util/delay.h"
#include <stdint.h>
#include "notes.h"

#define SSD1306_128_64
#undef SSD1306_128_32
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

Adafruit_SSD1306 display1(-1);
#if (SSD1306_LCDHEIGHT != 64)
   #error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

#include <Adafruit_NeoPixel.h>

#define OLED_ADDR1 0x3C
#define OLED_ADDR2 0x3D
#define ADC_1 A0
#define ADC_2 A1
#define TONE_PIN 7
#define ISR_1 19
#define ISR_2 2
#define ISR_3 3
#define RING_PIN1 6
#define RING_PIN2 8

#define BAR_PIN1 10
#define BAR_PIN2 9

#define RING_COUNT 24
#define BAR_COUNT 8
#define MAX_LIFE 40
#define oled_buffer_size 16

Adafruit_NeoPixel neopixel;


typedef enum{
  state_idle,
  state_countdown,
  state_game_one,
  state_game_two,
  state_roundover,
  state_gameover
} state_t;

typedef struct{
  Adafruit_NeoPixel* ring;
  Adafruit_NeoPixel* bar;
  int8_t offset;
  uint32_t round_score;
  uint8_t diff;
  uint8_t cw;
  uint8_t tick;
  int8_t life;
  uint8_t isr_update;
  char* oled_buffer;
} player_t;

static player_t players[2];

state_t game_states[2] = {
  state_game_one,
  state_game_two
};

/////////////////////
volatile state_t state = state_idle; 


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

static Adafruit_NeoPixel bar0 = Adafruit_NeoPixel(BAR_COUNT, BAR_PIN1, NEO_GRB + NEO_KHZ800);
static Adafruit_NeoPixel bar1 = Adafruit_NeoPixel(BAR_COUNT, BAR_PIN2, NEO_GRB + NEO_KHZ800);

static char buff_a[oled_buffer_size];
static char buff_b[oled_buffer_size];

  //setup players
  player_t* p1 = &players[0];
  p1->diff = 1;
  p1->cw = true;
  p1->tick = 0;
  p1->isr_update = false;
  p1->round_score = -1;
  p1->life = MAX_LIFE;
  p1->ring = &pix0;
  p1->ring->begin();
  p1->ring->show();
  p1->oled_buffer = &buff_a[0];

  p1->bar = &bar0;
  p1->bar->begin();
  p1->bar->show();
  //players[0] = p1;

  player_t* p2 = &players[1];
  p2->diff = 1;
  p2->cw = true;
  p2->tick = 0;
  p2->isr_update = false;
  p2->round_score = -1;
  p2->life = MAX_LIFE;
  p2->ring = &pix1;
  p2->ring->begin();
  p2->ring->show();
  p2->oled_buffer = &buff_b[0];

  p2->bar = &bar1;
  p2->bar->begin();
  p2->bar->show();
  //players[1] = p2;
  
  display1.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR1);
  display1.clearDisplay();
  display1.display();
}

void update_lifebar()
{
  uint8_t val = map(players[0].life, 0, MAX_LIFE, 0, 8);
  uint8_t j;
  for(j=0; j<BAR_COUNT; j++){
    if(j >= val){
      players[0].bar->setPixelColor(j, 0, 0, 0);
    } else {
      players[0].bar->setPixelColor(j, 255, 0, 0);
    }
  }
   players[0].bar->show();
  
  val = map(players[1].life, 0, MAX_LIFE, 0, 8);
  for(j=0; j<BAR_COUNT; j++){
    if(j >= val){
      players[1].bar->setPixelColor(j, 0, 0, 0);
    } else {
      players[1].bar->setPixelColor(j, 0, 0, 255);
    }
  }
  players[1].bar->show();

  Serial.print("Life P1: ");
  Serial.println(players[0].life);

  Serial.print("Life P2: ");
  Serial.println(players[1].life);
}

String oled_buffer_1;
String oled_buffer_2;

void oled_render(){

  
  String b1 = String(players[0].oled_buffer);
  if(b1 != oled_buffer_1){
  
    //
    display1.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR1);
    display1.clearDisplay();
    display1.setTextSize(2);
    display1.setTextColor(WHITE);
       
    display1.setCursor(64 - (snprintf(players[0].oled_buffer, oled_buffer_size, players[0].oled_buffer)*6),32 - 6);
    display1.print(players[0].oled_buffer);
    display1.display();
  
    oled_buffer_1 = String(players[0].oled_buffer);
  }

  b1 = String(players[1].oled_buffer);
  if(b1 != oled_buffer_2){

   display1.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR2);

   display1.clearDisplay();
   display1.setTextSize(2);
   display1.setTextColor(WHITE);
   
   display1.setCursor(64 - (snprintf(players[0].oled_buffer, oled_buffer_size, players[1].oled_buffer)*6),32 - 6);
   display1.print(players[1].oled_buffer);
   display1.display();

   oled_buffer_2 = String(players[1].oled_buffer);
  }
}

//lelijk var voor game1
uint32_t game2start = 0;
uint8_t finished = false;

volatile uint8_t reset_game = true;

void loop(void)                     
{
  uint8_t z;
  uint8_t j;
  
  switch(state){
    case state_gameover:
      snprintf(players[0].oled_buffer, oled_buffer_size, "GAME OVER");
      snprintf(players[1].oled_buffer, oled_buffer_size, "GAME OVER");
      
                 
      oled_render();
      reset_game = true;
      break;
      
    case state_idle:
      snprintf(players[0].oled_buffer, oled_buffer_size, "PLACE CARD");
      snprintf(players[1].oled_buffer, oled_buffer_size, "PLACE CARD");

      oled_render();

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

      players[0].isr_update = false;
      players[1].isr_update = false;

      if(reset_game)
      {
        reset_game = false;
        players[0].life = MAX_LIFE;
        players[1].life = MAX_LIFE;
        update_lifebar();
      }
      
      _delay_ms(500);
      break;
    case state_countdown:
      state = game_states[random(0,2)];
      if(state == state_game_one)
      {
        tone_game1_start();
      }

      if(state == state_game_two)
      {
        tone_game2_start();
      }
      
      for(i=3;i>0;i--){
         snprintf(players[0].oled_buffer, oled_buffer_size, "%d !", i);
         snprintf(players[1].oled_buffer, oled_buffer_size, "%d !", i);
         oled_render();

         tone(TONE_PIN, 1000/i, 200*(4-i));
         
         _delay_ms(750);
      }

      
      
      if(state == state_game_one)
      {
        snprintf(players[0].oled_buffer, oled_buffer_size, "HIT TARGET");
        snprintf(players[1].oled_buffer, oled_buffer_size, "HIT TARGET");
        oled_render(); 
        players[0].offset = random(4, RING_COUNT-4);
        players[1].offset = random(4, RING_COUNT-4);
      }

      if(state == state_game_two)
      {
        snprintf(players[0].oled_buffer, oled_buffer_size, "TAP FAST!");
      snprintf(players[1].oled_buffer, oled_buffer_size, "TAP FAST!");
        oled_render();
        game2start = millis();
      }

      finished = false;
      
      break;
    case state_roundover:
      tone_game_end();
      
      if(players[0].round_score < players[1].round_score)
      {
        players[1].life -= players[0].diff + players[1].diff;
      }

      if(players[1].round_score < players[0].round_score)
      {
        
        players[0].life -= players[0].diff + players[1].diff;
      }

      if(players[0].round_score == players[1].round_score)
      {
        players[1].life -= players[0].diff;
        players[0].life -= players[1].diff;
      }

      if(players[1].life <= 0)
      {
        players[1].life = 0;
        update_lifebar();
        //game over
        tone_russia();
        state = state_gameover;
      } else if(players[0].life <= 0)
      {
        players[0].life = 0;
        update_lifebar();
        //game over
        tone_usa();
        state = state_gameover;
      } else {
        update_lifebar();
        _delay_ms(1000);
        state = state_idle;
      }
      break;
    case state_game_one:
    //update for player 1 and 2
          finished = true;
          for(i=0; i < 2; i++){
            players[i].tick++;
            
            if(players[i].round_score == -1) {
              finished = false;
            if(players[i].isr_update){
              players[i].isr_update = false;
              //tone(TONE_PIN, 1000, 200);
              tone_button();
              
              if(players[i].offset == 0){
                snprintf(players[i].oled_buffer, oled_buffer_size, "HIT!");
                oled_render();
        
               }
               else {
                 snprintf(players[i].oled_buffer, oled_buffer_size, "MISS!");
                 oled_render();
               }

               if(players[i].offset >= RING_COUNT / 2)
               {
                players[i].round_score = RING_COUNT - players[i].offset; 
               } else {
                players[i].round_score = players[i].offset;
               }

               
            } else {
              finished = false;
                  if(players[i].tick >= (12 - players[i].diff)) {
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
              players[i].isr_update = false;
              
            }
          }

          if(finished) {
              _delay_ms(500);
               state = state_roundover;
          }
          
    break;
    case state_game_two:
        finished = false;
        
        for(i=0; i < 2; i++){
          if(players[i].round_score == -1){
              
            
              if(players[i].isr_update){
                    players[i].isr_update = false;
                    players[i].offset++;
    
                    uint8_t final = RING_COUNT + players[i].diff;
                    uint8_t fill = map(players[i].offset, 0, final, 0, RING_COUNT);
                    
                    Serial.println(fill);
                    
                    //beep
                    if(fill <= RING_COUNT){
                      tone(TONE_PIN, 300 + (players[i].offset * 50), 50);
                    }

                    //leds
                    for(j=0; j < RING_COUNT; j++){
                      if(j <= fill){
                        if(i == 0){
                          players[i].ring->setPixelColor(j, 255, 0, 0);
                        }

                        if(i == 1){
                          players[i].ring->setPixelColor(j, 0, 0, 255);
                        }
                      }
                      else {
                        players[i].ring->setPixelColor(j, 0, 0, 0);
                      }
                    }
                    players[i].ring->show();
              
                    //complete
                    if(fill >= RING_COUNT-1){
                        finished = true;
                        players[i].round_score = millis() - game2start;

                        //display
                        snprintf(players[i].oled_buffer, oled_buffer_size, "%d", players[i].round_score);
                        oled_render();
                        
                        break;
                    }
    
            }
          } else {
              players[i].isr_update = false;
              
            }
      }

      if(finished) {
         for(i=0; i < 2; i++){
          if(players[i].round_score == -1){
            players[i].round_score = millis();
          }
         }
              _delay_ms(1000);
               state = state_roundover;
          }
          
      break;
  }
}

void set_difficulty(void) {
  //set difficulty (0-1024)
  uint8_t adc = (analogRead(ADC_1) / (1024/10)) + 1;
  if(adc > 10) {
    adc = 10;
  }

  players[1].diff = adc;

  adc = (analogRead(ADC_2) / (1024/10)) + 1;
  if(adc > 10) {
    adc = 10;
  }

  players[0].diff = adc;
  
}
static void isr_main(void){
  noInterrupts();


  Serial.println("bukker");
  switch(state){
     case state_idle:
        state = state_countdown;
        break;
     case state_gameover:
        state = state_idle;
        break;
     default:
        break;
  }
  interrupts();
}

static void isr_p1(void){
  noInterrupts();
  if(state == state_game_one || state == state_game_two){
   players[0].isr_update = true;
        Serial.print("click: p1"); 
  }
  
  interrupts();
}

static void isr_p2(void){
  noInterrupts();
  if(state == state_game_one || state == state_game_two){
    players[1].isr_update = true;
    Serial.print("click: p2");
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

void tone_game_end(void){
  const uint16_t notes[] = {
    NOTE_C3,
    NOTE_B5,
    NOTE_C6
  };

  const uint8_t durations[] = {
    8,
    32,
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

void tone_game1_start(void){
  const uint16_t notes[] = {
    NOTE_C5,
    NOTE_E5,
    NOTE_F5
  };

  const uint8_t durations[] = {
    8,
    32,
    32
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

void tone_game2_start(void){
  const uint16_t notes[] = {
    NOTE_D6,
    NOTE_C6,
    NOTE_D6
  };

  const uint8_t durations[] = {
    32,
    32,
    32
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

void tone_button(void){
  const uint16_t notes[] = {
    NOTE_F4,
    NOTE_C5,
    NOTE_G5
  };

  const uint8_t durations[] = {
    32,
    32,
    32
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

