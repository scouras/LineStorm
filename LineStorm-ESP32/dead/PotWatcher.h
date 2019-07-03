#ifndef __POTWATCHER__
#define __POTWATCHER__


#define ADC_REFERENCE     DEFAULT
#define ADC_RESOLUTION         10
#define MAX_POTS  8    // Max channels on your ADC, 
                       // used to allocate the cabinet


#ifdef NANO
//#define ASYNC_ADC_ENABLED
#endif

// https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/blob/master/esp32/cores/esp32/esp32-hal-adc.h
#ifdef ESP32
#undef ASYNC_ADC_ENABLED
//#define EXTERNAL_ADC
#define ADC_RESOLUTION   12          // measured resolution
#define ADC_WIDTH        12          // reported resolution
#define ADC_ATTENUATION  ADC_11db
#define ADC_CYCLES           8

#define POT_BUFFER        64
#define POT_MIN         POT_BUFFER 
#define POT_MAX         (4096 - POT_BUFFER)
#endif


// Only works on AVR. Need alternate solution for ESPs
#ifdef ASYNC_ADC_ENABLED
#include "pollingAnalogRead.h"
#endif

#include "Potentiometer.h"

#ifdef EXTERNAL_ADC
#include <Adafruit_MCP3008.h>
Adafruit_MCP3008 adc;
#endif

/* ************************************************************
 *  PotWatcher is a smart, smooth, non-blocking, guardian of
 *  your ADC, intended for use when a number of analog devices
 *  need prioritized access without slowing your main loop().
 *  
 *  A single PotWatcher is set up, and users request virtual
 *  Pots from it, which are queued in the system. PotWatcher
 *  keeps the ADC busy by choosing a Pot, queuing up a read,
 *  polling until completion, and then updating the Pot's value.
 ************************************************************ */


namespace PotWatcher {

  enum ADCState { FREE, BUSY };

  // Timers
  long lastTrigger  = 0;
  long lastRead     = 0;
  long lastValue    = 0;
  long reads        = 0;

  ADCState adcState = FREE;               // busy or free
  Pot* cur;                               // Pot currently being read

  Pot* cabinet[MAX_POTS];                 // Where we keep the pots
  int count = 0;                          // Current number of pots
  
  //PotWatcher();
  void setup();

  bool canRead();
  void triggerRead();
  long getReading();
  
  void trigger();
  long read();
  long readNow(Pot* pot);
  long readNow(byte pin);

  void loop();                            // Run the core loop
  Pot* create(int pin, char* name=NULL);  // Create and store a new Pot
  Pot* pickPot();                         // pick next Pot

  void reportState(bool now = false);
};


void PotWatcher::setup() {

  #ifdef ESP32
    analogReadResolution(ADC_RESOLUTION);
    analogSetWidth(ADC_WIDTH);
    analogSetCycles(ADC_CYCLES);
    analogSetAttenuation(ADC_ATTENUATION);
  #endif

  #ifdef ASYNC_ADC_ENABLED
    pollingAnalogReference(ADC_REFERENCE);
  #else
    // um, need equivalent for other architectures...
  #endif
  reportState(true);
  Serial.println(F("----- PotWatcher Initialized-----"));
}


//============================================================ CREATE NEW POTS

Pot* PotWatcher::create(int pin, char* name) {

  if (count == MAX_POTS) { 
    Serial.print(F("CABINET IS FULL OF POTS! RETURNING SOMETHING STUPID"));
    return NULL;
  }

  for (int i = 0; i < count; i++) {
    if (cabinet[i]->getPin() == pin) {
      return cabinet[i];
    }
  }

  Pot *p = new Pot(pin);
  if (not name) {
    name = new char[3];
    sprintf(name, "%02d", pin);
  }
  p->setName(name);
#ifdef ESP32
  adcAttachPin(pin);
#else
  pinMode(pin, INPUT);
#endif
  readNow(p);

  cabinet[count++] = p;
  return p;
}

//========================================================= CHIP SPECIFIC CODE

bool PotWatcher::canRead() {
  #ifdef ASYNC_ADC_ENABLED
    return isAnalogReadingAvailable();
  #else
    return true;
  #endif
}

void PotWatcher::triggerRead() {
  #ifdef ASYNC_ADC_ENABLED
    triggerAnalogRead(pin);
  #else
    //nothing to do...
  #endif
}

long PotWatcher::getReading() {
  #ifdef ASYNC_ADC_ENABLED
    return getAnalogReading();
  #else
    if (cur == NULL) { return -1; }
    return analogRead(cur->getPin());
  #endif
}

//*********************************************************** CORE UPDATE LOOP
// * Check if ADC just finished (retrieve value)
// * Check is ADC is busy (wait)
// * Pick a Pot and run ADC


long PotWatcher::read() {
  if (not canRead()) { return -1; }

  if (adcState != FREE) {
    adcState = FREE;
    lastRead = micros();
    lastValue = getReading();
    reads++;
    cur->setState(SATED, lastRead);
    cur->update(lastValue);
  }
    
  if (0) {
    rPots->reportNow("PW read [%02d] %-8s <- %8d %8d\n", 
        cur->getPin(), cur->getStateName(), lastValue, cur->getValue());
  }

  return lastValue;
}


long PotWatcher::readNow(Pot* pot) {
  long value = readNow(pot->getPin());
  lastRead = micros();
  lastValue = getReading();
  reads++;  
  cur = pot;
  cur->setState(SATED, lastRead);
  pot->update(value);
  return value;
}

long PotWatcher::readNow(byte pin) {
  //Serial.println(F("----- Force Reading Pot -----"));
  while (adcState != FREE) {
    read();
    delayMicroseconds(ADC_MICROS);
  }
  return analogRead(pin);
}

void PotWatcher::trigger() {
  if (!count) { return; }
  pickPot();
  adcState = BUSY;
  lastTrigger = micros();
  triggerRead();

  cur->setState(RUNNING);
  cur->usLastTrigger = lastTrigger;
  
  if (0) {
    rPots->reportNow("PW trig [%02d] %-8s -> %-8d\n", 
        cur->getPin(), cur->getStateName(), cur->getValue());
  }
}




void PotWatcher::loop() {
  reportState();
  read();
  if (adcState == BUSY) return;
  trigger();
}



//============================================================== PICK NEXT POT

// SO YOU THINK YOU CAN POT time.
Pot* PotWatcher::pickPot() {

  rPicker->report("----- PICK POT ----- %d\n", count);

  // Trivial cases
  if (count == 0) {
    rPicker->report("No pots\n");
    return NULL;
  } else if (count == 1) {
    rPicker->report("One pots\n");
    cur = cabinet[0];
    return cur;
  }

  Pot* pot = NULL;

  for (int i = 0; i < count; i++) {
    Pot *p = cabinet[i];
      
    rPicker->report(" ++ pot[%d] %s %s %d\n", i, 
        p->name, p->getStateName(), p->usNextTarget);
    
    
    if ((long)p < 0x3ff00000 || p == (void*) 0xff) {
      reportNow("**** P %10p  Pot %10p   Cur %10p\n", p, pot, cur);
      reportNow("!!!! p[i] is bad #%d @ %p \n", i, p);
      reportState(true);
      delay(10000);
    }

    // check that p isn't overdue
    if (p->usNextTarget < NOW::Eus()) {
      p->updateState();
    }

    // Take the first pot we come across
    if (pot == NULL) {
      rPicker->report(" -- First pot!\n");
      pot = p;
      continue;
    }

    // don't run same pot 2x in a row when we have an alternative
    if ((long)pot < 0x3ff00000 || pot == (void*) 0xff) {
      reportState(true);
      delay(10000);
    }
    if (pot->getPin() == cur->getPin()) {
      rPicker->report(" -- Repeat pot!  cur %s  pot %s  p %s\n",
          cur->name, pot->name, p->name);
      pot = p;
      continue;
    }

    // out prioritized by state
    // except when comparing ACTIVE/SLEEPY
    if (p->state < pot->state) {
      rPicker->report(" -- Priority %d %-8s vs. %d %-8s\n", 
        pot->state, pot->getStateName(),
        p->state,     p->getStateName());
      pot = p;
      continue;
    }

    // equal states, sort by time
    if (pot->state == p->state) {
      if (p->usNextTarget < pot->usNextTarget) {
        rPicker->report(" -- Older %d %8d vs. %d %8d\n",
          pot->usNextTarget,
          p->usNextTarget);
        pot = p;
        continue;
      }
    }

    // someone must be spying on this poor pot
    rPicker->report(" -- p skipped this loop\n");
  }

  rPicker->report(" !! Picked pot %d because %s %d\n",
    pot->name, pot->getStateName(), pot->getValue());
  
  cur = pot;
  return pot;
}


//=================================================================  REPORTING

void PotWatcher::reportState(bool now) {
  /*
  rPots->report(
    "PW: %4d  %-10s  Reading %6d %6d in %8ld us\n\n", 
    cur->getPin(), 
    cur->getStateName(),
    lastValue,
    cur->getValue(), 
    lastRead - lastTrigger
  );
  */

  //rPots->debug();

  //if (rPots->ready()) now = true;

  for (int i = 0; i < count; i++) {
    cabinet[i]->report(now);
  }
}








/* ************************************************************
 *  Configure ADC Prescaler
 ************************************************************ */

/*
  #ifndef cbi
  #define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
  #endif
  #ifndef sbi
  #define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
  #endif

  void configureADCPrescaler16() {
    // Set the Prescaler to 16 (16000KHz/16 = 1MHz)
    // WARNING: Above 200KHz 10-bit results are not reliable.
    //ADCSRA |= B00000100;
    sbi(ADCSRA, ADPS2);
    cbi(ADCSRA, ADPS1);
    cbi(ADCSRA, ADPS0);
    
    // Set ADIE in ADCSRA (0x7A) to enable the ADC interrupt.
    // Without this, the internal interrupt will not trigger.
    //ADCSRA |= B00001000;
    sbi(ADCSRA,ADIE);
  }
*/




#endif // POTWATCHER
