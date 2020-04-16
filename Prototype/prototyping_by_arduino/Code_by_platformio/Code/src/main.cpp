#include <Arduino.h>
#include "SPI.h"          
#include <Wire.h>  
#include <ECG_wave.h>   

// various constants used by the waveform generator
#define INIT      0
#define IDLE      1
#define QRS       2
#define FOUR      4
#define THREE     3
#define TWO       2
#define ONE       1

// global variables used by the program
unsigned int  NumSamples = sizeof(y_data) / 2;            // number of elements in y_data[] above
unsigned int  QRSCount = 0;                               // running QRS period msec count
unsigned int  IdleCount = 0;                              // running Idle period msec count
unsigned long IdlePeriod = 0;                             // idle period is adjusted by pot to set heart rate
unsigned int  State = INIT;                               // states are INIT, QRS, and IDLE
unsigned int  DisplayCount = 0;                           // counts 50 msec to update the 7-segment display
unsigned int  tcnt2;                                      // Timer2 reload value, globally available
float         BeatsPerMinute;                             // floating point representation of the heart rate
unsigned int  Bpm;                                        // integer version of heart rate (times 10)
unsigned int  BpmLow;                                     // lowest heart rate allowed (x10)
unsigned int  BpmHigh;                                    // highest heart rate allowed (x10)
int           Value;                                      // place holder for analog input 0
unsigned long BpmValues[32] = {0, 0, 0, 0, 0, 0, 0, 0,    // holds 32 last analog pot readings
                               0, 0, 0, 0, 0, 0, 0, 0,    // for use in filtering out display jitter
                               0, 0, 0, 0, 0, 0, 0, 0,    // for use in filtering out display jitter
                               0, 0, 0, 0, 0, 0, 0, 0};   // for use in filtering out display jitter
unsigned long BpmAverage = 0;                             // used in a simple averaging filter
unsigned char Index = 0;                                  // used in a simple averaging filter  
unsigned int  DisplayValue = 0;                           // filtered Beats Per Minute sent to display

void  DTOA_Send(unsigned short DtoAValue);

void setup()   {   
   Serial.begin(9600);
  // Configure the output ports (1 msec intrerrupt indicator and D/A SPI support)
  pinMode(10, OUTPUT);                    // D/A converter chip select       (low to select chip) 
  pinMode(11, OUTPUT);                    // SDI data 
  pinMode(13, OUTPUT);                    // SCK clock

  // initial state of SPI interface
  SPI.begin();                            // wake up the SPI bus.
  SPI.setDataMode(0);                     // mode: CPHA=0, data captured on clock's rising edge (low→high)
  SPI.setClockDivider(SPI_CLOCK_DIV64);   // system clock / 64
  SPI.setBitOrder(MSBFIRST);              // bit 7 clocks out first
  
  // establish the heart rate range allowed
  // BpmLow  = 300 (30 bpm x 10)
  // BpmHigh = (60.0 / (NumSamples * 0.001)) * 10  = (60.0 / .543) * 10 = 1104  (110.49 x 10)
  BpmLow = 300;
  BpmHigh = (60.0 / ((float)NumSamples * 0.001)) * 10;

  // First disable the timer overflow interrupt while we're configuring
  TIMSK2 &= ~(1<<TOIE2);

  // Configure timer2 in normal mode (pure counting, no PWM etc.)
  TCCR2A &= ~((1<<WGM21) | (1<<WGM20));
  TCCR2B &= ~(1<<WGM22);

  // Select clock source: internal I/O clock   
  ASSR &= ~(1<<AS2);

  // Disable Compare Match A interrupt enable (only want overflow)   
  TIMSK2 &= ~(1<<OCIE2A);

  // Now configure the prescaler to CPU clock divided by 128   
  TCCR2B |= (1<<CS22)  | (1<<CS20); // Set bits
  TCCR2B &= ~(1<<CS21);             // Clear bit

  // We need to calculate a proper value to load the timer counter.
  // The following loads the value 131 into the Timer 2 counter register
  // The math behind this is:
  // (CPU frequency) / (prescaler value) = 125000 Hz = 8us.
  // (desired period) / 8us = 125.
  // MAX(uint8) + 1 - 125 = 131;
  // 
  // Save value globally for later reload in ISR  /
  tcnt2 = 131; 

  // Finally load end enable the timer   
  TCNT2 = tcnt2;
  TIMSK2 |= (1<<TOIE2);
}


void loop() {

  // read from the heart rate pot (Analog Input 0)  
  Value = 1023; //change this **************************
 
  // map the Analog Input 0 range (0 .. 1023) to the Bpm range (300 .. 1104)
  Bpm = map(Value, 0, 1023, BpmLow, BpmHigh);

  // To lessen the jitter or bounce in the display's least significant digit,
  // a moving average filter (32 values) will smooth it out.
  BpmValues[Index++] = Bpm;                       // add latest sample to eight element array
  if (Index == 32) {                              // handle wrap-around
    Index = 0;
  }
  BpmAverage = 0;
  for (int  i = 0;  i < 32; i++) {                // summation of all values in the array
    BpmAverage += BpmValues[i];
  }
  BpmAverage >>= 5;                               // Divide by 32 to get average
  
  // now update the 4-digit display - format: XXX.X
  // since update is a multi-byte transfer, disable interrupts until it's done
  noInterrupts();
  DisplayValue = BpmAverage;
  interrupts();  
  
  // given the pot value (beats per minute) read in, calculate the IdlePeriod (msec)
  // this value is used by the Timer2 1.0 msec interrupt service routine
  BeatsPerMinute = (float)Bpm / 10.0;
  noInterrupts();
  IdlePeriod = (unsigned int)((float)60000.0 / BeatsPerMinute) - (float)NumSamples;
  interrupts();
  
  delay(20);
}

ISR(TIMER2_OVF_vect) {
  
  // Reload the timer   
  TCNT2 = tcnt2;
  
  // state machine
  switch (State) {
    
    case INIT:

      // zero the QRS and IDLE counters
      QRSCount = 0;
      IdleCount = 0;
      DisplayCount = 0;
    
      // set next state to QRS  
      State = QRS;
    break;
  
    case QRS:

      // output the next sample in the QRS waveform to the D/A converter 
      DTOA_Send(y_data[QRSCount]);
      
      // advance sample counter and check for end
      QRSCount++;
      if (QRSCount >= NumSamples) {
        // start IDLE period and output first sample to DTOA
        QRSCount = 0;
        DTOA_Send(y_data[0]);
        State = IDLE;
      }
    break;
  
    case IDLE:

      // since D/A converter will hold the previous value written, all we have
      // to do is determine how long the IDLE period should be.
    
      // advance idle counter and check for end
      IdleCount++;
      
      // the IdlePeriod is calculated in the main loop (from a pot)
      if (IdleCount >= IdlePeriod) {
        IdleCount = 0;
        State = QRS;
      }  
    break;
  
    default:
    break;
  }
 
  // output to serial every 50 msec
  DisplayCount++;
  if (DisplayCount >= 50) {
    DisplayCount = 0;
    Serial.println(DisplayValue);
  } 
}  

void  DTOA_Send(unsigned short DtoAValue) {
  
  byte Data = 0;
  // select the D/A chip (low)
  digitalWrite(10, 0);    // chip select low
  
  // send the high byte first 0011xxxx
  Data = highByte(DtoAValue);
  Data = 0b00001111 & Data;
  Data = 0b00110000 | Data;
  SPI.transfer(Data);
  
  // send the low byte next xxxxxxxx
  Data = lowByte(DtoAValue);
  SPI.transfer(Data);
 
  // all done, de-select the chip (this updates the D/A with the new value) 
  digitalWrite(10, 1);    // chip select high
}