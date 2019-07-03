
#define WIFI_ENABLED    1
#define DIGITAL_LEDS    0
#define ANALOG_LEDS     1
#define ANALOG_INVERT   0
//------------------------------------------------------------------ DEBUGGING

#undef ENABLE_MEMORY_PACER

#define PACER_MEMORY_INTERVAL      1000
#define PACER_MEMORY_ENABLED          0

#include "now.h"
//#include "log.h"
#include "pacer.h"
#include "utils.h"

//#define SERIAL_BAUD               115200
#define SERIAL_BAUD               921600
#define SERIAL_INTERVAL             1000
#define SERIAL_TIMEOUT                10
#define SERIAL_ENABLED                 1
Pacer *p;

#define PACER_STROBE_INTERVAL      1000
#define PACER_STROBE_ENABLED          1
#define DEBUG_WAVE_FUNCTIONS       false
Pacer *pStrobe;


//---------------------------------------------------------------- POT WATCHER

// ESP32 LineStorm PinMap
// https://docs.google.com/spreadsheets/d/1tu6Cmns8MrpVQubDZ7IvcXSoID7PiDEh30jdAbInfYY/edit#gid=0

#define ESC_LEFT_RPM    36  // 36
#define ESC_LEFT_PPM    26  // 37
#define ESC_RIGHT_RPM   38  // 38
#define ESC_RIGHT_PPM   25  // 39

#define TRELLIS_SDA     SDA // 21
#define TRELLIS_SCL     SCL // 22

//----------------------------------------------------------------------- LEDS

#include <FastLED.h>
#include "GradientPalettes.h"

#define FPS                    10000
#define NUM_LEDS                 100
#define INITIAL_BRIGHTNESS       64
#define CHIPSET               APA102
#define COLOR_ORDER              BGR


#define DIGITAL_LED_SCK          SCL
#define DIGITAL_LED_MOSI         SDA

#define LED_RED                   14
#define LED_GREEN                 12
#define LED_BLUE                  13

CRGB leds[NUM_LEDS];

#include "Strobe.h"
#include "Line.h"
//------------------------------------------------------------------- WIFI/OSC

#include "wifi_esp32.h"

//#include "controller.h"

//====================================================================== SETUP

void setup() {

  delay(2000);
  NOW::tick();

  //----- Serial
  Serial.begin(SERIAL_BAUD);
  Serial.setTimeout(SERIAL_TIMEOUT);
  while (!Serial) { delay(1); }
  Serial.print(F("\n\n===== BEGINNING SETUP =====\n"));

  //----- Pacers
  p       = new Pacer(SERIAL_INTERVAL,        SERIAL_ENABLED);
  pMemory = new Pacer(PACER_MEMORY_INTERVAL,  PACER_MEMORY_ENABLED);
  pStrobe = new Pacer(PACER_STROBE_INTERVAL,  PACER_STROBE_ENABLED);

  //----- Strobe
  //reportMemFree(true);
  Strobe::setup();
  delay(100);
  //Strobe::colorBars();
  //delay(100);

  //----- String
  Line::setup();

  //Line::calibrateESCs();
  
  
  changePalette(gCurrentPaletteNumber);
  Serial.printf("Total Palettes: %d + %d = %d\n", 
  paletteCount,
  gGradientPaletteCount,
  totalPaletteCount);

  ////---- WiFi
  setupWiFi();

  Serial.print(F("\n===== Setup Complete =====\n"));
  delay(100);
  
}

//======================================================================= LOOP
//#define PALETTE_CHANGE_INTERVAL 600000
//unsigned long lastPaletteChange = 0;

void loop() {

  NOW::tick();

  //if ((NOW - lastPaletteChange)/1000 >= PALETTE_CHANGE_INTERVAL) {
  //  changePalette( addmod8( gCurrentPaletteNumber, 1, totalPaletteCount) );
  //  lastPaletteChange = NOW;
  //}  

  checkOSC();  
  
  p->report("Loop %10lu   %6lu us\n",
    NOW::ticks,
    NOW::Dus()
  );    

  p->report("Frames %10dms %10dr %10dv    FPS %10dr %10dv\n",
    NOW::Dms(Strobe::framesTime),
    Strobe::framesReal,
    Strobe::framesVirt,
    Strobe::getFPSReal(),
    Strobe::getFPSVirt()
  );

  //reportMemMap();
  //reportMemFree();

  Strobe::loop();
  Line::loop();

  p->report("\n\n");

}
