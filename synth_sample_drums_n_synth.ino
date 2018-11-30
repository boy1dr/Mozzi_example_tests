/*  Example of playing sampled sounds,
 *  using Mozzi sonification library.
 *
 *  Demonstrates one-shot samples scheduled
 *  with EventDelay(), and fast random numbers with 
 *  xorshift96() and rand(), a more friendly wrapper for xorshift96().
 *
 *  Circuit: Audio output on digital pin 9 (on a Uno or similar), or 
 *  check the README or http://sensorium.github.com/Mozzi/
 *
 *  Mozzi help/discussion/announcements:
 *  https://groups.google.com/forum/#!forum/mozzi-users
 *
 *  Tim Barrass 2012.
 *  This example code is in the public domain.
 */

int DrumPattern[] = {5,4,4,4,7,4,4,4,5,4,4,4,7,4,4,4};
int DrumPattern2[] = {5,0,4,0,7,0,4,0,5,0,4,0,7,0,4,0};




#include <MozziGuts.h>
#include <Sample.h> // Sample template
#include <samples/bass.h> // wavetable data
#include <samples/snare.h> // wavetable data
#include <samples/hh.h> // wavetable data
#include <EventDelay.h>
#include <mozzi_rand.h>

//Synth
#include <Oscil.h> // oscillator 
#include <tables/cos2048_int8.h> // table for Oscils to play
#include <mozzi_analog.h> // fast functions for reading analog inputs 


#include <tables/sin2048_int8.h> // sine table for oscillator

#define CONTROL_RATE 64

// use: Sample <table_size, update_rate> SampleName (wavetable)
Sample <bass_NUM_CELLS, AUDIO_RATE> aBamboo1(bass_DATA);
Sample <snare_NUM_CELLS, AUDIO_RATE> aBamboo2(snare_DATA);
Sample <hh_NUM_CELLS, AUDIO_RATE> aBamboo3(hh_DATA);

//synth
const int LDR_PIN = 1; // set the input for the Light Dependent Resistor to analog pin 1

Oscil<COS2048_NUM_CELLS, AUDIO_RATE> aCarrier(COS2048_DATA);
Oscil<COS2048_NUM_CELLS, AUDIO_RATE> aModulator(COS2048_DATA);

Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin(SIN2048_DATA);


int carrier_freq = 55; // fundamental frequency
int mod_ratio = 6; // harmonics
int mod_freq = carrier_freq * mod_ratio;
long deviation; // carries control info from updateControl to updateAudio

int shareThing = 0;

const int KNOB_PIN = 0;
unsigned char volume;
// for scheduling audio gain changes
EventDelay <CONTROL_RATE>  kTriggerDelay;

int StepCount = 0;
int StepsTotal = 15;

int BeatCount = 1;

int getBPMms(int inBPM){
 return  (((60.0/inBPM)/4.0)*1000);
}

void setup(){
  delay(1000);
  
  setupFastAnalogRead(); // one way of increasing the speed of reading the input
  aCarrier.setFreq(carrier_freq); 
  aModulator.setFreq(mod_freq);
  
  startMozzi(CONTROL_RATE);
  
  aBamboo1.setFreq((float) bass_SAMPLERATE / (float) bass_NUM_CELLS); // play at the speed it was recorded at
  aBamboo2.setFreq((float) snare_SAMPLERATE / (float) snare_NUM_CELLS);
  aBamboo3.setFreq((float) hh_SAMPLERATE / (float) hh_NUM_CELLS);
  kTriggerDelay.set(getBPMms(120)); // countdown ms, within resolution of CONTROL_RATE
}


unsigned char randomByte(){
  return highByte(xorshift96());
}

unsigned char randomGain(){
  //return lowByte(xorshift96())<<1;
  return rand(200) + 55;
}

// referencing members from a struct is meant to be a bit faster than seperately
// ....haven't actually tested it here...
struct gainstruct{
  unsigned char gain1;
  unsigned char gain2;
  unsigned char gain3;
}
gains;


void updateControl(){
  aCarrier.setFreq(carrier_freq); 
  if(kTriggerDelay.ready()){
    
    if(StepCount>StepsTotal){
      StepCount=0; 
      if(BeatCount==1){BeatCount=0;}else{BeatCount=1;}
      
  }
    
    int knob_value = analogRead(KNOB_PIN); // value is 0-1023
    volume = knob_value>> 2;
  
    gains.gain1 = volume;
    gains.gain2 = volume;
    gains.gain3 = volume;
    
    if(BeatCount==0){
      if(bitRead(DrumPattern[StepCount], 0)==1){ aBamboo1.start(); }
      if(bitRead(DrumPattern[StepCount], 1)==1){ aBamboo2.start(); }
      if(bitRead(DrumPattern[StepCount], 2)==1){ aBamboo3.start(); }
    }else{
      if(bitRead(DrumPattern2[StepCount], 0)==1){ aBamboo1.start(); }
      if(bitRead(DrumPattern2[StepCount], 1)==1){ aBamboo2.start(); }
      if(bitRead(DrumPattern2[StepCount], 2)==1){ aBamboo3.start(); }
    }
    
    /*
    for (byte i=0; i<7; i++) {
      if(bitRead(DrumPattern(StepCount), i)){ aBamboo1.start(); }
    }
    */
    
    
    unsigned int light_level= analogRead(LDR_PIN); // value is 0-1024
    deviation = light_level - 400;
    aSin.setFreq((int)light_level);
    
    
    
  carrier_freq++;
    
    StepCount++;
    kTriggerDelay.start();
  }
}


int updateAudio(){
  
  if(carrier_freq>1000){carrier_freq=55;}
  long modulation = (long)deviation * aModulator.next(); 
  char out = aCarrier.phMod(modulation);
  
  
  int asig= (int) ((long) aBamboo1.next()*gains.gain1 +
    aBamboo2.next()*gains.gain2 +
    aBamboo3.next()*gains.gain3)/60; //16;
  //clip to keep audio loud but still in range
  if (asig > 243) asig = 243;
  if (asig < -244) asig = -244;
  
  if(shareThing==0){shareThing=1;return ((int)out * volume)>>8;}
  if(shareThing==1){shareThing=2; return asig;}
   if(shareThing==2){shareThing=0;return ((int)aSin.next() * volume)>>8;}
    
}


void loop(){
  audioHook();
}













