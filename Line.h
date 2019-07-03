#ifndef __LINE_H__
#define __LINE_H__

#include "Cycle.h"
#include "ESC.h"

#define LINE_THROTTLE_UPDATE_INTERVAL 100000    // 100 millis
#define LINE_CHAOS_SCALE                   0.1  // 
#define LINE_CHAOS_RATE                    0.01 // 

namespace Line {

  enum ControlMode { DIRECT, CHAOS };

  ControlMode mode = DIRECT;
  double RPMtarget    = 0.0;
  double RPMcurrent   = 0.0;
  double chaos        = 0.0;

  Pacer *pThrottle;
  
  ESC escL(ESC_LEFT_PPM,  "escL");
  ESC escR(ESC_RIGHT_PPM, "escR");

  // Inputs differ by mode, so abstract names
  //Pot *pESCL;
  //Pot *pESCR;
  
  void setup();
  void loop();
  void loop_direct();
  void loop_chaos();

  void setThrottleLeft(double value);
  void setThrottleRight(double value);

  void calibrateESCs();
}


//======================================================== SETUP AND ACCESSORS

void Line::setup() {

  pThrottle = new Pacer(LINE_THROTTLE_UPDATE_INTERVAL, true);
  
  //pESCL = PotWatcher::create(PIN_ESC_LEFT);
  //pESCR = PotWatcher::create(PIN_ESC_RIGHT);

  //pESCL->addCallbackReal(readpESCL);
  //pESCR->addCallbackReal(readpESCR);
  
  escL.setup();
  escR.setup();

  Serial.println(F("----- Line Initialized -----"));

}



//======================================================================= LOOP
void Line::loop() {

  if (not pThrottle->ready()) return
  pThrottle->step();

  escL.updateRPM();
  escR.updateRPM();

  if      (mode == DIRECT) loop_direct();
  else if (mode == CHAOS ) loop_chaos();

}






//============================================================== CONTROL MODES

// map pots directly to throttle for ESCs
void Line::loop_direct() {
  //setThrottleLeft(pESCL->getReal());
  //setThrottleRight(pESCR->getReal());
}

void Line::setThrottleLeft(double value) {
  escL.setThrottle(value);  
}

void Line::setThrottleRight(double value) {
  escR.setThrottle(value);  
}

void Line::loop_chaos() {

  loop_direct();
  return;

  //double throttle = pESCL->getReal();
  //double chaos = pESCR->getReal();

}




// automated routine to establish PPM boundaries with ESC


/*
#define CALIBRATION_PAUSE 5000
//#define CALIBRATION_RPM 100

void Line::calibrateESCs() {

  Serial.printf("\n\nRECALIBRATING ESCS\n\n");

  escL.setRPM(0);
  escR.setRPM(0);
  Serial.printf("Set RPM to 0.\n");
  delay(CALIBRATION_PAUSE);

  escL.escServo.detach();
  escR.escServo.detach();
  Serial.printf("Detached servos.\n");
  delay(CALIBRATION_PAUSE);

  escL.escServo.attach();
  escR.escServo.attach();
  Serial.printf("Reattached Servos\n");
  //delay(CALIBRATION_PAUSE);

  Serial.printf("Motors should be beeping up a storm...\n");
  escL.escServo.write(PPM_MAX);
  escR.escServo.write(PPM_MAX);
  Serial.printf("Calibrating HIGH - Sent MAX PPM\n");
  delay(100);

  escL.escServo.write(PPM_MIN);
  escR.escServo.write(PPM_MIN);
  Serial.printf("Sent MIN PPM and waiting\n");
  delay(CALIBRATION_PAUSE);
  
  Serial.printf("\n");
  Serial.printf("\n");
  
  escL.setRPM(0);
  escR.setRPM(0);
  Serial.printf("Set RPM to 0.\n");
  
  Serial.printf("\n\nCALIBRATION COMPLETE\n\n");
}
*/



#endif // __LINE_H__
