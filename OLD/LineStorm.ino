/*  ********************* Controller for Line Storm ************************** 
 *
 * Version 0.1 - TODO
 * Translate basic functions into I2C for Master
 * * Adafruit_KeyPad
 * * Adafruit_ADXL343
 * * Adafruit_NeoPixel
 *
 *
 * ======================================================================== */

#define VERBOSE true
#ifdef VERBOSE
  #define LOG Serial
#endif
#include <now.h>
#include <pacer.h>

// ================================================================ NEOTRELLIS

#include <Adafruit_ADXL343.h>
#include <Adafruit_NeoTrellisM4.h>
Adafruit_NeoTrellisM4 trellis = Adafruit_NeoTrellisM4();
Adafruit_ADXL343 adxl = Adafruit_ADXL343(123, &Wire1);


// =================================================================== FASTLED
// FastLED doesn't support the NeoTrellis yet. And we have some crazy 
// ZeroDMA NeoPixel thing to use. BUT we can use all FastLED functios so long
// as we don't actually call addLeds() or show(). Will take some overhead
// copying over the FastLED values at regular intervals intead.
//
// With 800kHz data / 24bits per pixel / 32 pixels, we could theoretically hit
// ~1kFPS. But NeoPixels only PWM at "no less than 400 Hz" (potentially
// 100kHz PWM clock), so no reason to drive that fast. We'll limit to 100 fps.

#define INITIAL_BRIGHTNESS   128
#define PACER_LED_INTERVAL    10
#define PACER_LED_ENABLED   true
Pacer *pLEDs;

#include <FastLED.h>
const CRGB colorInit = CRGB::Magenta;
const CRGB colorOn   = CRGB::Red;
const CRGB colorOff  = CRGB::Blue;

const uint8_t LED_COLS = 8;
const uint8_t LED_ROWS = 4;
const uint8_t LED_KEYS = LED_COLS + LED_ROWS;
CRGB leds[LED_KEYS];

// Convert FastLED to NeoPixel_ZeroDMA
void showLEDs() { 
  if (! pLEDs->ready()) return;
  for (int i = 0; i < LED_KEYS; i++) {
    CRGB l = leds[i];
    trellis.setPixelColor(i, l.r, l.g, l.b);
  }
  trellis.show();
}



// ======================================================================= I2C
#include <Wire.h>
#include <I2C_Anything.h>

const byte I2C_MASTER      = 23;
const byte I2C_CONTROLLER  = 42;


// ==============================================================

boolean pressed[LED_KEYS] = {false};        // Pressed state for each button


typedef struct {
  float modX;
  float modY;
  float modZ;

  float r;
  float g;
  float b;

  float h;
  float s;
  float v;

  float hex;

} Button;

Button buttons;


unsigned long accelReadTime = 0L; // Keypad polling timer
unsigned long accelInterval = 100; // Limit accellerometer readings to 1 per ms
int keysPressed = 0;


// ##################################################################### SETUP

void setup(){

  //--- Serial
  if (VERBOSE) {
    Serial.begin(115200);
    while (!Serial);
  }
  log_header(1, "NeoTrellis Controller Setup");

  //--- Trellis
  trellis.begin();
  trellis.setBrightness(INITIAL_BRIGHTNESS);
  trellis.fill(colorInit);

  //--- I2C
  Wire.begin(I2C_CONTROLLER); // join i2c bus (address optional for master)
  Wire.setClock(400000L);

  //--- ADXL
  if(!adxl.begin()) {
    log_printf("!! No accelerometer found");
  }
  log_printf("Accellerometer ID: %d", adxl.getDeviceID());

  log_header(2, "Setup Complete");
}



void send_key(uint8_t key, uint8_t state) {
  Wire.beginTransmission(I2C_MASTER); // transmit to device #8
  I2C_writeAnything("k");             // Key
  I2C_writeAnything(key);             // sends one byte
  I2C_writeAnything(state);           // sends one byte
  Wire.endTransmission();             // stop transmitting
}


// Sensor accellerometer events contain both xyz and orientation calculations
// https://github.com/adafruit/Adafruit_Sensor/blob/master/Adafruit_Sensor.h
//

void send_acceleration(float x, float y, float z) {
  Wire.beginTransmission(I2C_MASTER); // transmit to device #8
  I2C_writeAnything("a");             // Accelleration
  I2C_writeAnything(x);               // x
  I2C_writeAnything(y);               // t
  I2C_writeAnything(z);               // z
  Wire.endTransmission();             // stop transmitting
}

void send_orientation(float roll, float pitch) {
  Wire.beginTransmission(I2C_MASTER); // transmit to device #8
  I2C_writeAnything("o");             // Orientation
  I2C_writeAnything(roll);            //
  I2C_writeAnything(pitch);           //
  Wire.endTransmission();             //
}



//// Trellis ADXL343 Orientation
// Assume Trellis is flat, connectors pointing away from you, buttons up.
// +X: Forward
// +Y: Right
// +Z: Down
// Chip upside-down on board, so Z points down, thus is positive in normal
// flat orientation.

// Roll:   Rotation around the longitudinal axis (the plane body, 'X axis'). 
// φ / x   Roll is positive and increasing when moving downward. 
// phi     -90°<=roll<=90°
// Pitch:  Rotation around the lateral axis (the wing span, 'Y axis'). 
// θ / y   Pitch is positive and increasing when moving upwards. 
// theta   -180°<=pitch<=180°)
// Yaw:    Angle between the longitudinal axis (the plane body) and magnetic north, 
// ψ / y   measured clockwise when viewing from the top of the device. 
// psi     0-359°


// It turns out that Adafruit makes sad libraries. None of their accellerometer
// libraries actually populate these variables, only XYZ. Also "heaading" is 
// appropriate for a magnetometer, whereas yaw is what you calculate for a 
// 3-axis accellerometer. Actually accellerometers can't measure it (reliably)
// either, alas. So we're going to ignore their library for this.

//// Descriptive Orientation Code ("up", "down", ...)
// https://www.arduino.cc/en/Tutorial/Genuino101CurieIMUAccelerometerOrientation


// Stolen from Dr. Robot
// https://www.nxp.com/files-static/sensors/doc/app_note/AN3461.pdf

//three axis acceleration data
int x, y, z;

//Roll & Pitch are the angles which rotate by the axis X and y 
float roll = 0.00, pitch = 0.00, yaw = 0.00;

// rad2deg -> x *= 57.2958

void get_orientation(float x, float y, float z, 
    float &roll, float &pitch, float &yaw){
  // roll = atan2(y, z) * 57.2958;
  // pitch = atan2((-x) , sqrt(y * y + z * z)) * 57.2958;

  yaw   = atan2(z, sqrt(x*x + y*y)) * 57.2958
  roll  = atan2(x, sqrt(y*y + z*z)) * 57.2958
  pitch = atan2(y, sqrt(x*x + z*z)) * 57.2958
 
  // Chip is actually upside down on the board, so offset roll into a more
  // intuitive coordinate system. When flat on table, roll == pitch == 0; 
  // roll += 180.0;
  // if (roll > 180.0) { roll -= 360.0; }
}

// NOTE: We're just not doing to deal with this yet. ^^



// ###################################################################### LOOP

void loop() {
  trellis.tick();

  unsigned long t = millis();


  //------------------------------------------------------------------- KeyPad
  while (trellis.available()){
    keypadEvent e = trellis.read();
    uint8_t i = e.bit.KEY;
    if (e.bit.EVENT == KEY_JUST_PRESSED) {
      Serial.print("Key On "); Serial.println(i);
      pressed[i] = true;     
      send_key(i, KEY_JUST_PRESSED);
      trellis.setPixelColor(i, colorOn);  
    }
    else if (e.bit.EVENT == KEY_JUST_RELEASED) {
      Serial.print("Key Off "); Serial.println(i);
      pressed[i] = false;
      send_key(i, KEY_JUST_RELEASED);
      trellis.setPixelColor(i, colorOff);  
    }
  }



  //----------------------------------------------------------- Accellerometer

  int xbend = 0;
  int ybend = 0;
  bool changed = false;
  
  if ((t - accelReadTime) >= accelInterval) {

    // Check for accelerometer
    sensors_event_t event;
    adxl.getEvent(&event);
    
    int x = adxl.getX();
    int y = adxl.getY();
    int z = adxl.getZ();

    get_orientation(x, y, z, roll, pitch);

    send_acceleration(x, y, z);
    send_orientation(roll, pitch);

    /* Display the results (acceleration is measured in m/s^2) */
    Serial.print("X: "); Serial.print(event.acceleration.x); Serial.print(" / ");
                         Serial.print(x);                    Serial.print("  ");
    Serial.print("Y: "); Serial.print(event.acceleration.y); Serial.print(" / ");
                         Serial.print(y);                    Serial.print("  ");
    Serial.print("Z: "); Serial.print(event.acceleration.z); Serial.print(" / ");
                         Serial.print(z);                    Serial.print("  ");
    Serial.print("R: "); Serial.print(roll);
    Serial.print("P: "); Serial.print(pitch);
    Serial.println();

    accelReadTime= t;
  }
}



// HSV->RGB conversion based on GLSL version
// expects hsv channels defined in 0.0 .. 1.0 interval



float fract(float x) { return x - int(x); }

float mix(float a, float b, float t) { return a + (b - a) * t; }

float step(float e, float x) { return x < e ? 0.0 : 1.0; }

float* hsv2rgb(float h, float s, float b, float* rgb) {
  rgb[0] = b * mix(1.0, constrain(abs(fract(h + 1.0) * 6.0 - 3.0) - 1.0, 0.0, 1.0), s);
  rgb[1] = b * mix(1.0, constrain(abs(fract(h + 0.6666666) * 6.0 - 3.0) - 1.0, 0.0, 1.0), s);
  rgb[2] = b * mix(1.0, constrain(abs(fract(h + 0.3333333) * 6.0 - 3.0) - 1.0, 0.0, 1.0), s);
  return rgb;
}

float* rgb2hsv(float r, float g, float b, float* hsv) {
  float s = step(b, g);
  float px = mix(b, g, s);
  float py = mix(g, b, s);
  float pz = mix(-1.0, 0.0, s);
  float pw = mix(0.6666666, -0.3333333, s);
  s = step(px, r);
  float qx = mix(px, r, s);
  float qz = mix(pw, pz, s);
  float qw = mix(r, px, s);
  float d = qx - min(qw, py);
  hsv[0] = abs(qz + (qw - py) / (6.0 * d + 1e-10));
  hsv[1] = d / (qx + 1e-10);
  hsv[2] = qx;
  return hsv;
}
