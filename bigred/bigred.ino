#include <Adafruit_NeoPixel.h>

#define PIXEL_PIN 10
#define PIXEL_COUNT 8

Adafruit_NeoPixel bar = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

#define ISR_1 2

volatile bool isr_changed = false;
static uint8_t x;

void neopixel_fill(Adafruit_NeoPixel* leds, uint8_t red, uint8_t green, uint8_t blue, uint8_t interval){
  uint8_t i=0;
  for(i=0; i < leds->numPixels(); i++){
      leds->setPixelColor(i, red, green, blue);
      leds->show(); 
      delay(interval);
  }  
}

void setup(void){
  pinMode(ISR_1, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ISR_1), bigred_isr, FALLING);

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(9600);
  Serial.println("init");

  bar.begin();
  bar.show();
  
  for(x=0; x < bar.numPixels(); x++){
    bar.setPixelColor(x, 0, 0, 0);
    bar.show(); 
  }
}
void loop(void){
  if(isr_changed){
    isr_changed = false;
    
    neopixel_fill(&bar, 0, 0, 255, 100);
    delay(1000);
    neopixel_fill(&bar, 0, 0, 0, 0);
   }
}
static void bigred_isr(void){
  static bool fired = false;
  noInterrupts();
  
  if(fired == false && !digitalRead(ISR_1)){
    isr_changed = true;
    fired = true;
  }
  else if(digitalRead(ISR_1)){
    fired = false;
  }
  interrupts();
}

