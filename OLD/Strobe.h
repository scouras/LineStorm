#ifndef __STROBE_H__
#define __STROBE_H__


#define ANALOG_LEDS     1
#define ANALOG_INVERT   0
#define DIGITAL_LEDS    0

#define LED_CHANNEL_RED           1
#define LED_CHANNEL_GREEN         2
#define LED_CHANNEL_BLUE          3

//#define LED_PWM_FREQUENCY   1048576
#define LED_PWM_FREQUENCY    131072
//#define LED_PWM_FREQUENCY     32768
#define LED_PWM_BITS              12
const uint32_t LED_PWM_RESOLUTION = pow(2, LED_PWM_BITS);
const uint32_t LED_PWM_SCALE = LED_PWM_RESOLUTION / 256;


#define US_PER_SECOND    1000000
#define MIN_PERIOD          1000.0  //   1 millisecond
#define MAX_PERIOD       1000000.0  //  10 seconds
#define MIN_FREQUENCY   US_PER_SECOND / MAX_PERIOD
#define MAX_FREQUENCY   US_PER_SECOND / MIN_PERIOD

#define FPS_RESET_PERIOD    10 * US_PER_SECOND  // reset FPS stats


#include "Cycle.h"

namespace Strobe {

  Pacer* pStrobeLimit; 

  long framesTime = 0;
  long framesVirt = 0;
  long framesReal = 0;
  CRGB color = CRGB(255, 255, 255);
  
  Cycle *hueCycle;
  Cycle *satCycle;
  Cycle *brtCycle;
 
  void testRainbow(); 
  //void testRainbowPots(); 
  void colorBars();
  
  void setup();
  void loop();
  void show();
  void show(CRGB color);

  double expScale(double min, double range, double base, double value);
  double expScalePeriod(double value, Cycle *cycle = NULL);
  double expScaleFrequency(double value, Cycle *cycle = NULL);

  //void checkPots();
  void reportCycles(bool now = false);

  void resetFPS();
  long getFPSReal();
  long getFPSVirt();

  void setHuePer(double value);
  void setHueFrq(double value);
  void setHueMod(double value);

  void setSatPer(double value);
  void setSatFrq(double value);
  void setSatMod(double value);

  void setBrtPer(double value);
  void setBrtFrq(double value);
  void setBrtMod(double value);
};


/*
void Strobe::testRainbowPots() {

  pStrobe->reportNow("-- Strobe Test Mode - Rainbow Pots\n");
 
  int r = 0;
  int g = 0;
  int b = 0;

  int l = 0;
  while(1) {

    r = PotWatcher::readNow(PIN_HUE_FRQ);
    g = PotWatcher::readNow(PIN_HUE_MOD);
    b = PotWatcher::readNow(PIN_BRT_FRQ);

    int scale = 4096 / 256;
    CRGB color = CRGB(r / scale, g / scale, b / scale);
    pStrobe->reportNow("---- Analog: %4d %4d %4d    RGB: %3d %3d %3d\n",
        r, g, b, color.r, color.g, color.b);

    Strobe::show(color);
    delay(100);
  }
} 
*/

void Strobe::testRainbow() {

  pStrobe->reportNow("-- Strobe Test Mode\n");
  
  uint8_t gHue = 0; // rotating "base color" used by many of the patterns
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
 
  int l = 0; 
  while(1) {
    CRGB color = leds[l];
    pStrobe->reportNow("---- Loop %d  RGB %3d %3d %3d\n",
        l, color.r, color.g, color.b);

    Strobe::show(color);
    //Strobe::show(CHSV(random8(), 255, 255));
    l = (l + 1) % NUM_LEDS;
    delay(100);
  }
} 


void Strobe::setup() {

  Serial.println(F("===== Initializing Strobe ====="));
  
  pStrobeLimit = new Pacer(US_PER_SECOND / FPS / 1000, true);

  hueCycle = new Cycle (LINEAR,  1000000.0,    1.0, "hue");
  //hueCycle = new Cycle (STAIRS,  1000000.0,    1.0, "hue");
  satCycle = new Cycle (CONST,   1000000.0,  255.0, "sat");
  brtCycle = new Cycle (SQUARE,   447000.0,  128.0, "brt");
  
  Serial.println(F("----- Cycles Made -----"));

  /*
  pHueFrq = PotWatcher::create(PIN_HUE_FRQ);
  pHueMod = PotWatcher::create(PIN_HUE_MOD);
  pBrtFrq = PotWatcher::create(PIN_BRT_FRQ);
  pBrtMod = PotWatcher::create(PIN_BRT_MOD);
  
  pHueFrq->addCallbackReal(setHueFrq);
  pHueMod->addCallbackReal(setHueMod);
  pBrtFrq->addCallbackReal(setBrtFrq);
  pBrtMod->addCallbackReal(setBrtMod);
  Serial.println(F("----- Pots Made -----"));
  */


  reportCycles(true);

  delay(100);
  resetFPS();
  delay(100);


  if (DIGITAL_LEDS) {
    pinMode(DIGITAL_LED_SCK, OUTPUT);
    pinMode(DIGITAL_LED_MOSI, OUTPUT);
    FastLED.addLeds<CHIPSET, DIGITAL_LED_MOSI, DIGITAL_LED_SCK, COLOR_ORDER>
          (leds, NUM_LEDS)
          //.setCorrection( TypicalSMD5050 );
          ;
    Serial.println(F("----- Digital LEDs Ready -----"));
  }
  FastLED.setBrightness( INITIAL_BRIGHTNESS );  
  delay(100);


  if (ANALOG_LEDS) {
    Serial.println(F("----- Init Analog LEDs -----"));
    delay(1000);
    //pinMode(ANALOG_LED_RED,   GPIO_MODE_OUTPUT_OD);
    //pinMode(ANALOG_LED_GREEN, GPIO_MODE_OUTPUT_OD);
    //pinMode(ANALOG_LED_BLUE,  GPIO_MODE_OUTPUT_OD);
  
    // Initialize channels 
    // channels 0-15, resolution 1-16 bits, freq limits depend on resolution
    // ledcSetup(uint8_t channel, uint32_t freq, uint8_t resolution_bits);

    pinMode(LED_RED, OUTPUT);
    Serial.printf(" ** Red Done\n"); //delay(1000);
    pinMode(LED_GREEN, OUTPUT);
    Serial.printf(" ** Green Done\n"); //delay(1000);
    pinMode(LED_BLUE, OUTPUT);
    Serial.printf(" ** Blue Done\n"); //delay(1000);

    ledcSetup(LED_CHANNEL_RED,   LED_PWM_FREQUENCY, LED_PWM_BITS);
    ledcSetup(LED_CHANNEL_GREEN, LED_PWM_FREQUENCY, LED_PWM_BITS);
    ledcSetup(LED_CHANNEL_BLUE,  LED_PWM_FREQUENCY, LED_PWM_BITS);  
  
    ledcAttachPin(LED_RED,   LED_CHANNEL_RED);
    ledcAttachPin(LED_GREEN, LED_CHANNEL_GREEN);
    ledcAttachPin(LED_BLUE,  LED_CHANNEL_BLUE);
    
    Serial.println(F("----- Analog LEDs Ready -----"));
  }

  Serial.println(F("----- Strobe Initialized xxxxx"));
  show(CHSV(random8(), 255, 255));

  Serial.println(F("----- Strobe Initialized -----"));

}


void Strobe::resetFPS() {
  //Serial.println("__reset");
  framesTime = NOW::Eus();
  framesVirt = 0;
  framesReal = 0;
}

long Strobe::getFPSReal() {
  return framesReal * US_PER_SECOND / (NOW::Dus(framesTime));
}

long Strobe::getFPSVirt() {
  return framesVirt * US_PER_SECOND / (NOW::Dus(framesTime));
}


void Strobe::loop() {
  pStrobe->report("-- Strobe %3dr %3dg %3db\n", color.r, color.g, color.b);

  if ((NOW::Eus() - framesTime) > FPS_RESET_PERIOD) { resetFPS(); }
  framesVirt++;

  if (!pStrobeLimit->ready()) {
    return;
  } else {
    pStrobeLimit->spend();
  }

  hueCycle->run(NOW::Dus());
  satCycle->run(NOW::Dus());
  brtCycle->run(NOW::Dus());

  uint8_t &hue = hueCycle->value;
  uint8_t &sat = satCycle->value;
  uint8_t &brt = brtCycle->value;

  //hue = ColorFromPalette(gCurrentPalette, hue);
  CRGB colorNext = CHSV(hue,sat,brt);
  
  reportCycles();
  if (color == colorNext) { return; }
  //("RGB(%3u,%3u,%3u)\n", hue, sat, brt);

  color = colorNext;
  //color = CRGB(255, 255, 255);
  //FastLED.showColor(color);
  //pStrobe->reportNow("---- New color %3dr %3dg %3db\n", color.r, color.g, color.b);
  Strobe::show();
  framesReal++;

};

void Strobe::show() { Strobe::show(color); }

void Strobe::show(CRGB color) {

  if (DIGITAL_LEDS) {
    FastLED.showColor(color);
  }

  color.nscale8(FastLED.getBrightness());

  if (ANALOG_LEDS) {
    int r = color.r * LED_PWM_SCALE;
    int g = color.g * LED_PWM_SCALE;
    int b = color.b * LED_PWM_SCALE;
  
    if (ANALOG_INVERT) {
      r = LED_PWM_RESOLUTION - r - 1;
      g = LED_PWM_RESOLUTION - g - 1;
      b = LED_PWM_RESOLUTION - b - 1;
    }

    ledcWrite(LED_CHANNEL_RED,   r);
    ledcWrite(LED_CHANNEL_GREEN, g);
    ledcWrite(LED_CHANNEL_BLUE,  b);

    pStrobe->report("---- Strobing %3dr %3dg %3db\n", r, g, b);
  }
}


// colorBars: flashes Red, then Green, then Blue, then Black.
// Helpful for diagnosing if you've mis-wired which is which.
void Strobe::colorBars()
{
  int pause = 1000;
  show( CRGB::White ); delay(pause);
  show( CRGB::Red );   delay(pause);
  show( CRGB::Green ); delay(pause);
  show( CRGB::Blue );  delay(pause);

  for (int h = 0; h < 255; h++) {
    show( CHSV(h, 100, 100) );  delay(pause/255);
  } 
  
  show( CRGB::Black ); delay(pause);
  show( CRGB::DarkGray ); delay(pause);
}


void Strobe::reportCycles(bool now) {

  bool ready = pStrobe->ready();
  //pStrobe->debug();
  if (now) { pStrobe->spend(); }
  if (not pStrobe->ready()) { return; }
  //Serial.print("Spending pStrobe\n");
  pStrobe->spend();
  //pStrobe->debug();
  hueCycle->report();
  satCycle->report();
  brtCycle->report();

}


// Determine log of the max value - log of the min value and rescale
// Then exponentiate

double Strobe::expScale(double min, double range, double base, double value) {
  double exponent = min + (value * range);
  double scaled = pow(base, exponent);
  return scaled;
}

static const double minPerLog = log10(MIN_PERIOD);
static const double maxPerLog = log10(MAX_PERIOD);
static const double rngPerLog = maxPerLog - minPerLog;

double Strobe::expScalePeriod(double value, Cycle *cycle) {

  //double exponent = minPerLog + (value * rngPerLog);
  //double period = pow(10.0, exponent);

  double period = expScale(minPerLog, rngPerLog, 10.0, value);
  if (cycle) { cycle->setPeriod(period); }
  return period;
}

static const double minFrqLog = log10(MIN_FREQUENCY);
static const double maxFrqLog = log10(MAX_FREQUENCY);
static const double rngFrqLog = maxFrqLog - minFrqLog;

double Strobe::expScaleFrequency(double value, Cycle *cycle) {
  //double exponent = minFrqLog + (value * rngFrqLog);
  //double frequency = pow(10.0, exponent);
  double frequency = expScale(minFrqLog, rngFrqLog, 10.0, value);
  if (cycle) { cycle->setFrequency(frequency); }
  return frequency;
}


void Strobe::setHuePer(double value) { expScalePeriod(value, hueCycle); }
void Strobe::setSatPer(double value) { expScalePeriod(value, satCycle); }
void Strobe::setBrtPer(double value) { expScalePeriod(value, brtCycle); }

void Strobe::setHueFrq(double value) { expScaleFrequency(value, hueCycle); }
void Strobe::setSatFrq(double value) { expScaleFrequency(value, satCycle); }
void Strobe::setBrtFrq(double value) { expScaleFrequency(value, brtCycle); }

void Strobe::setHueMod(double value) { hueCycle->setModulation(value); }
void Strobe::setSatMod(double value) { satCycle->setModulation(value); } 
void Strobe::setBrtMod(double value) { brtCycle->setModulation(value); }

#endif // __STROBE_H__
