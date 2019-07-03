#ifndef __ESC_H__
#define __ESC_H__

#include <ESP32Servo.h>

#define RPM_MIN                     0
#define RPM_MAX                  1000
#define RPM_THRESHOLD              10
#define RPM_INITIAL               100
#define THROTTLE_CHECK_INTERVAL   100

#define RPM_UPDATE_INTERVAL        10   // ms between throttle updates
#define RPM_SILENCE_INTERVAL     1000   // send command to keep ESC's quiet
#define RPM_SILENCE_VALUE          20   // rpm needed to convince the stupid motors to shut up
#define RPM_ACCELLERATION          10   // rpm/check_interval accelleration limit
long RPM_LIMIT                  = 1000;

// Standard PPM
#define PPM_MIN                  1000
#define PPM_MAX                  2000
//#define PPM_MIN                   800
//#define PPM_MAX                  2300

long ESC_PULSES_PER_REV = 14;


class ESC {

  public:

    Servo escServo;
    byte escPin;
    char* name;
    Pacer *rESC;

    double phase = 0;
    int RPMcurrent = RPM_INITIAL;
    int RPMtarget  = RPM_INITIAL;

    ESC(byte e, char* n = "esc");

    void attach();                      // attach servo
    void setup();                       // initialize

    void sendRPM(int rpm);              // send PPM pulse to ESC
    void sendRPM();
    void updateRPM();                   // updates RPM and sends
    void setRPM(int rpm);               // set absolute RPM
    void setThrottle(double throttle);  // [0-100%] of max RPM

    void loop();                        // check and update

    void silence();
    void calibrate();
};


//================================================== CONSTRUCTOR AND ACCESSORS

ESC::ESC(byte e, char* n)
      : escPin(e)
      , name(n)
      {
    pinMode(e, OUTPUT);
}


void ESC::setup() {
  rESC = new Pacer(THROTTLE_CHECK_INTERVAL, true);
  attach();
}

void ESC::attach() {
  escServo.attach(escPin, PPM_MIN, PPM_MAX);
  if (!escServo.attached()) {
    printf("!!!! Attach failed for %s\n", name);
  }
  setRPM(RPM_INITIAL);
}



//============================================================= RPM & THROTTLE

void ESC::sendRPM(int rpm) {
  int pulse = map(rpm, RPM_MIN, RPM_MAX, PPM_MIN, PPM_MAX);
  escServo.write(pulse);
  //escServo.writeMicroseconds(pulse);
  //rESC->report("ESC %4s: %4d rpm  / %4d ppm\n", name, rpm, pulse);
}

void ESC::sendRPM() { 
  sendRPM(RPMcurrent); 
}

// TODO: Do we need accelleration?
void ESC::updateRPM() { 

  if (not rESC->ready()) return;
  rESC->spend();
  
  long RPMdelta = RPMtarget - RPMcurrent;
  long step = (long)abs(RPMdelta);
  if (RPM_ACCELLERATION < step) { step = RPM_ACCELLERATION; }
  if (RPMdelta < 0) { step = -step; }
  RPMcurrent += step;
  sendRPM();
  //lastUpdate = NOW;  
  
  sendRPM();
}


void ESC::setRPM(int rpm) {
  RPMtarget = constrain(rpm, RPM_MIN, RPM_LIMIT);
  // Off means off; prevent sputtering
  if (RPMtarget <= RPM_THRESHOLD) {
    RPMtarget = 0;
  }
  updateRPM();
}

void ESC::setThrottle(double throttle) {
  throttle = constrain(throttle, 0.0, 100.0);
  double rpm = throttle * (double)RPM_LIMIT / 100.0;
  rESC->report("-- Converting throttle %.2f to rpm %.2f\n", throttle, rpm);
  setRPM(lround(rpm));
}



//======================================================================= LOOP

void ESC::loop() {
  if (not rESC->ready()) return;
  rESC->spend();
  silence();
}


//==================================================================== Utility
 
void ESC::silence() {
  static Pacer pSilence(RPM_SILENCE_INTERVAL, true);
  if (RPMcurrent) return; // ESC knows we're here
  if (not pSilence.ready()) return; // not time yet
  pSilence.step();

  sendRPM(RPM_SILENCE_VALUE);
  delay(10);
  sendRPM(0);
}



































#endif // __ESC_H__
