// ----------------------------------------------------
//
// APC magazine - Arduino Masterclass
// Project #18 - Digital Audio Recorder v2.0
// Darren Yates - 11 April 2014
//
// Includes the SdFat library - https://code.google.com/p/sdfatlib/downloads/list
//
// ----------------------------------------------------

#include <SdFat.h>

#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

SdFat sd;
SdFile rec;

const int chipSelect = 4;
unsigned long fileSize = 0L;
unsigned long waveChunk = 16;
unsigned int waveType = 1;
unsigned int numChannels = 1;
unsigned long sampleRate = 22050;
unsigned long bytesPerSec = 22050;
unsigned int blockAlign = 1;
unsigned int bitsPerSample = 8;
unsigned long dataSize = 0L;
unsigned long recByteCount = 0L;
unsigned long recByteSaved = 0L;
const int btnStart = 6;
const int btnStop = 5;
const int ledStart = 2;
const int ledStop = 3;
int recPressed = 0;
int stopPressed = 0;
unsigned long oldTime = 0L;
unsigned long newTime = 0L;
byte buf00[512]; // buffer array 1
byte buf01[512]; // buffer array 2
byte byte1, byte2, byte3, byte4;
unsigned int bufByteCount;
byte bufWrite;

void setup() { // THIS RUNS ONCE

  Setup_timer2();
  Setup_ADC();

  pinMode(10, OUTPUT);
  pinMode(ledStart, OUTPUT);
  pinMode(ledStop, OUTPUT);
  pinMode(btnStop, INPUT_PULLUP);
  pinMode(btnStart, INPUT_PULLUP);
  
  if (sd.begin(chipSelect, SPI_FULL_SPEED)) { // initialise card on SPI to 8MHz SPI bus speed
    for (int dloop = 0; dloop < 4; dloop++) {
      digitalWrite(ledStart,!digitalRead(ledStart));
      delay(100);
    }
  } else { // if error, flash LED twice per second, until reset
    while(1) {
      digitalWrite(ledStart,!digitalRead(ledStart));
      delay(500);
    }
  }

}

void loop() { // THIS RUNS LOTS!

  if (digitalRead(btnStart) == LOW && recPressed == 0) {
    StartRec(); // launch StartRec method
  }
  if (digitalRead(btnStop) == LOW) {
    StopRec(); // launch StopRec method
  }
  
  if (recByteCount % 1024 == 512 && recPressed == 1) { rec.write(buf00,512); recByteSaved+= 512; } // save buf01 to card
  if (recByteCount % 1024 == 0 && recPressed == 1) { rec.write(buf01,512); recByteSaved+= 512; } // save buf02 to card

}

void StartRec() { // begin recording process

  digitalWrite(ledStart,HIGH);
  digitalWrite(ledStop,LOW);
  recByteCount = 0;
  recByteSaved = 0;
  recPressed = 1; // recording button has been pressed
  stopPressed = 0;
  writeWavHeader();
  sbi (TIMSK2, OCIE2A); // enable timer interrupt, start grabbing audio

}

void StopRec() { // stop recording process, update WAV header, close file
    
  cbi (TIMSK2, OCIE2A); // disable timer interrupt
  writeOutHeader();
  digitalWrite(ledStart,LOW); // turn off recording LED
  digitalWrite(ledStop,HIGH); // light stop LED
  recPressed = 0;
  
}
  
void writeOutHeader() { // update WAV header with final filesize/datasize

  rec.seekSet(4);
  byte1 = recByteSaved & 0xff;
  byte2 = (recByteSaved >> 8) & 0xff;
  byte3 = (recByteSaved >> 16) & 0xff;
  byte4 = (recByteSaved >> 24) & 0xff;  
  rec.write(byte1);  rec.write(byte2);  rec.write(byte3);  rec.write(byte4);
  rec.seekSet(40);
  rec.write(byte1);  rec.write(byte2);  rec.write(byte3);  rec.write(byte4);
  rec.close();
  
}

void writeWavHeader() { // write out original WAV header to file

  recByteSaved = 0;
  rec.open("rec00000.wav", O_CREAT | O_TRUNC | O_RDWR);
  rec.write("RIFF");
  byte1 = fileSize & 0xff;
  byte2 = (fileSize >> 8) & 0xff;
  byte3 = (fileSize >> 16) & 0xff;
  byte4 = (fileSize >> 24) & 0xff;  
  rec.write(byte1);  rec.write(byte2);  rec.write(byte3);  rec.write(byte4);
  rec.write("WAVE");
  rec.write("fmt ");
  byte1 = waveChunk & 0xff;
  byte2 = (waveChunk >> 8) & 0xff;
  byte3 = (waveChunk >> 16) & 0xff;
  byte4 = (waveChunk >> 24) & 0xff;  
  rec.write(byte1);  rec.write(byte2);  rec.write(byte3);  rec.write(byte4);
  byte1 = waveType & 0xff;
  byte2 = (waveType >> 8) & 0xff;
  rec.write(byte1);  rec.write(byte2); 
  byte1 = numChannels & 0xff;
  byte2 = (numChannels >> 8) & 0xff;
  rec.write(byte1);  rec.write(byte2); 
  byte1 = sampleRate & 0xff;
  byte2 = (sampleRate >> 8) & 0xff;
  byte3 = (sampleRate >> 16) & 0xff;
  byte4 = (sampleRate >> 24) & 0xff;  
  rec.write(byte1);  rec.write(byte2);  rec.write(byte3);  rec.write(byte4);
  byte1 = bytesPerSec & 0xff;
  byte2 = (bytesPerSec >> 8) & 0xff;
  byte3 = (bytesPerSec >> 16) & 0xff;
  byte4 = (bytesPerSec >> 24) & 0xff;  
  rec.write(byte1);  rec.write(byte2);  rec.write(byte3);  rec.write(byte4);
  byte1 = blockAlign & 0xff;
  byte2 = (blockAlign >> 8) & 0xff;
  rec.write(byte1);  rec.write(byte2); 
  byte1 = bitsPerSample & 0xff;
  byte2 = (bitsPerSample >> 8) & 0xff;
  rec.write(byte1);  rec.write(byte2); 
  rec.write("data");
  byte1 = dataSize & 0xff;
  byte2 = (dataSize >> 8) & 0xff;
  byte3 = (dataSize >> 16) & 0xff;
  byte4 = (dataSize >> 24) & 0xff;  
  rec.write(byte1);  rec.write(byte2);  rec.write(byte3);  rec.write(byte4);

}

void Setup_timer2() {

  TCCR2B = _BV(CS21);  // Timer2 Clock Prescaler to : 8
  TCCR2A = _BV(WGM21); // Interupt frequency  = 16MHz / (8 x 90 + 1) = 22191Hz
  OCR2A = 90; // Compare Match register set to 90

}

void Setup_ADC() {

  ADMUX = 0x65; // set ADC to read pin A5, ADLAR to 1 (left adjust)
  cbi(ADCSRA,ADPS2); // set prescaler to 8 / ADC clock = 2MHz
  sbi(ADCSRA,ADPS1);
  sbi(ADCSRA,ADPS0);
}

ISR(TIMER2_COMPA_vect) {

  sbi(ADCSRA, ADSC); // start ADC sample
  while(bit_is_set(ADCSRA, ADSC));  // wait until ADSC bit goes low = new sample ready
  recByteCount++; // increment sample counter
  bufByteCount++;
  if (bufByteCount == 512 && bufWrite == 0) { 
    bufByteCount = 0;
    bufWrite = 1;
  } else if (bufByteCount == 512 & bufWrite == 1) {
    bufByteCount = 0;
    bufWrite = 0;
  }

  if (bufWrite == 0) { buf00[bufByteCount] = ADCH; }
  if (bufWrite == 1) { buf01[bufByteCount] = ADCH; }
  

//  if (recByteCount % 1024 < 512) { // determine which buffer to store sample into
//    buf01[recByteCount % 512] = ADCH;
//  } else {
//    buf02[recByteCount % 512] = ADCH;
//  }

}
