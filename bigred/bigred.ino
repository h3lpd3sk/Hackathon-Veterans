#define ISR_1 2

volatile bool isr_changed = false;

void setup(void){
  pinMode(ISR_1, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ISR_1), isr, FALLING);

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(9600);
}
void loop(void){
  if(isr_changed){
    isr_changed = false;
    
    digitalWrite(LED_BUILTIN, HIGH);
    _delay_ms(250);
    digitalWrite(LED_BUILTIN, LOW);
  }
}
static void isr(void){
  noInterrupts();
  
  //Big red
  if(!digitalRead(ISR_1)){
    isr_changed = true;
  }
  interrupts();
}

