// super famicom mouse by takuya matsubara

// SFC connnector
//  ---------+------------+
// |(7)(6)(5)|(4)(3)(2)(1)|
//  ---------+------------+
// (1)VCC : 5V
// (2)CLK : clock (rising edge=change data)
// (3)P/S : latch (low=latch / high=loading)
// (4)DAT : data
// (7)GND : ground

// P/S
// +-+
// | |  LATCH
// + +---------------------------------------------------------------------------------------------------

// CLK
// -----+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+---
//      |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
//     

//  DAT(mouse)
//     0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
//   +-----------------------+--+--+-----+--------+  +-----------------------+-----------------------+
//   | 0  0  0  0  0  0  0  0| R| L|SPEED| 0  0  0| 1|YD Y6 Y5 Y4 Y3 Y2 Y1 Y0|XD X6 X5 X4 X3 X2 X1 X0|
// --+                       +--+--+-----+        +--+-----------------------+-----------------------+---

//  DAT(gamepad)
//   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//   | B  Y SL ST UP DW LT RT  A  X  L  R| 0  0  0  0|
// --+--+--+--+--+--+--+--+--+--+--+--+--+           +--

#define POSX A0   // stick X
#define POSY A1   // stick Y

#define INVERTX 1 // X INVERT
#define INVERTY 0 // Y INVERT

#define LED 8 //debug

#define CONE_PORT PORTB
#define CONE_PIN  PINB
#define CONE_DDR  DDRB

#define BIT_CLK   (1<<1)
#define BIT_PS    (1<<2)
#define BIT_DAT   (1<<3)
#define BIT_BTNL  (1<<4)
#define BIT_BTNR  (1<<5)

volatile int adc0;  //  AD count(stick X)
volatile int adc1;  //  AD count(stick Y)
volatile unsigned char intr=0;

//-------------------------------    
// ADC ISR 
ISR (ADC_vect) 
{
  int tempcnt;
  tempcnt = (int)ADCL; 
  tempcnt += ((int)ADCH) << 8;

  if(intr & 1){
//    digitalWrite(LED, HIGH);   ////debug  LED on
    adc1 = tempcnt;
    ADMUX = bit(REFS0) | 0; //AD channel 0
  }else{
//    digitalWrite(LED, LOW);   ////debug  LED off
    adc0 = tempcnt;
    ADMUX = bit(REFS0) | 1; //AD channel 1
  }
  intr++;
  ADCSRA |= bit(ADSC);  //AD start
}
     
//-------------------------------    
void setup() {
  Serial.begin(115200);

  CONE_DDR |= BIT_DAT;
  CONE_PORT |= BIT_PS;    //pullup
  CONE_PORT |= BIT_CLK;   //pullup
  CONE_PORT |= BIT_DAT;   // DAT=0
  CONE_PORT |= BIT_BTNL;
  CONE_PORT |= BIT_BTNR;

//  pinMode(LED, OUTPUT); //debug
//  digitalWrite(LED, LOW);    // LED off 

// AD init
// ADEN: ADC enable
// ADIF: ADC interrupt flag
// ADIE: ADC interrupt enable
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  ADCSRA = bit(ADEN) | bit(ADIE) | bit(ADIF) | 7; //sampling clk/128 
  ADCSRB = 0;
  ADMUX = bit(REFS0) | 0; //AD channel 0
  sei();
  ADCSRA |= bit(ADSC);  //AD start
}

//-------------------------------    
#define CENTER 512

void loop() {
  unsigned char bitcnt;
  unsigned long work;
  unsigned long mask;
  unsigned char loopcnt=0;
  int movecnt;
   
  while(1){
    mask = 0x80000000;  //mouse mask 32bit
    work = 0x00010000;  //mouse data

#if INVERTX==0
    movecnt = (adc0 - CENTER)/128;
#else
    movecnt = (CENTER - adc0)/128;
#endif
    
   if(movecnt < 0){
      movecnt = -movecnt;
      work |= 0x80+movecnt; //left
    }else if(movecnt > 0){
      work |= movecnt; //right
    }

#if INVERTY==0
    movecnt = (adc1 - CENTER)/128;
#else
    movecnt = (CENTER - adc1)/128;
#endif

    if(movecnt < 0){
      movecnt = -movecnt;
      work |= 0x8000+(movecnt<<8); //up
    }else if(movecnt > 0){
      work |= movecnt<<8; //down
    }

    if((CONE_PIN & BIT_BTNL)==0){ //L button
      work |= 0x00400000;
    }
    if((CONE_PIN & BIT_BTNR)==0){ //R button
      work |= 0x00800000;
    }

    while((CONE_PIN & BIT_PS)==0);
    while((CONE_PIN & BIT_PS)!=0);
    cli();

    for(bitcnt=0; bitcnt<32; bitcnt++){
      if(work & mask){
        CONE_PORT &= ~BIT_DAT;  // DAT=1
      }else{
        CONE_PORT |= BIT_DAT;   // DAT=0
      }
      work <<= 1;
      while((CONE_PIN & BIT_CLK)!=0);
      while((CONE_PIN & BIT_CLK)==0);
      //rising edge
      
      if((CONE_PIN & BIT_PS)!=0)break; // end of latch
    }
    CONE_PORT |= BIT_DAT;   // DAT=0
   
    loopcnt++;
    sei();
  }
}
