#ifndef __POTENTIOMETER__
#define __POTENTIOMETER__

#include <ResponsiveAnalogRead.h>


#define POT_MIN                      0    // based on  N-bit ADC
#ifdef ESP32
#define POT_MAX                   4096    // based on 10-bit ADC 
#else
#define POT_MAX                   1024    // based on 10-bit ADC 
#endif

#define POT_ENABLE_SLEEP             1    // lock down idle pots
#define POT_SLEEPY_PENALTY          16    // reduce update frequency for sleeping
#define POT_ACTIVITY_THRESHOLD       4    // minimum change to be ACTIVE
                                          // based on background noise level
#define POT_DEFAULT_SNAP             0.4f // smoothing constant
#define ADC_MICROS                 128    // approximate time for complete read

#define POT_ENABLE_EDGE_SNAP      true
#define DEBUG_POT_CALLBACKS      false

typedef void (*fReal)(double);
typedef void (*fLong)(long);


// Allowed states for each Pot, in priority order, which affects the
// frequency of reads. Whenever the state is updated, a target for the next
// read time (usNextTarget) is determined based on ADC speed, PotState, and 
// POT_SLEEPY_PENALTY. 
//
// The first 3 states are always preemptive. Pots will sort first by the
// state itself, then by the time the state was set (which was stored in
// usNextTarget). 
//
// SLEEPY pots moved less than POT_ACTIVITY_THRESHOLD between their last 2
// readings, are less likely to be updated soon, so can be checked less often.
// ACTIVE and SLEEPY are the normal state, and sort by level, then in
// order of usNextTarget. The difference is how that target is calculated.
//
//    usNextTarget = usLastRead + ADC_MICROS * penalty
//
// The penalty is always 1 for ACTIVE Pots. For a SLEEPY Pot, the penalty
// doubles each time it is found unchanged, up to a maximum of 
// POT_SLEEPY_PENALTY. So in a 1.6 us period, 2 ACTIVE Pots might be read
// 7 times each while 2 SLEEPY pots were only read once each. When a
// SLEEPY Pot becomes ACTIVE, its penalty is immediately reset to 1.
// 
// The Pot currently RUNNING obviously doesn't need to be updated next. In
// fact, to keep things lively, a Pot will never be read twice in a row,
// unless it's the only Pot in the system. But then, why are you using this
// library?
//
// A freshly SATED Pot will be checked for activity on the last read, then
// assigned the appropriate status. The control loop should have moved on
// by the time it is returned to the pool.
//
// Because PotWatcher alternates Pots, the normal run mode will be swapping
// between ACTIVE pots, or mixing in SLEEPY pots if only one is ACTIVE. Even
// with multiple ACTIVE pots hogging the ADC, SLEEPY pots escalate to LATE
// pots once their target passes, and thus preempt ACTIVE ones. 


enum PotState {
  
  // Preemptive States
  INIT,     // 0    just created, needs initial value
  URGENT,   // 1    manually flagged
  LATE,     // 2    missed update target
 
  // Idle States
  ACTIVE,   // 3    recently updated, so check frequently
  SLEEPY,   // 4    inactive, so deprioritize

  // Running States
  SATED,    // 5    just updated, determine if sleepy or active
  RUNNING,  // 6    currently on the ADC

};

char* PotStateNames[] = {
  "Init",   "Urgent", "Late", 
  "Active", "Sleepy", 
  "Sated", "Running"};
  

class Pot : public ResponsiveAnalogRead {

  public:

    int _pin;           // because ResponsiveAnalogRead is dumb
    PotState state;
    int penalty;
    long usLastTrigger;  // 
    long usLastRead;     // 
    long usNextTarget;   // desired update based on activity
    long reads;
    char* name;
    bool hasOldChange;

    int scale_min = POT_MIN;
    int scale_max = POT_MAX;

    byte countCBReal = 0;
    byte countCBLong = 0;
    fReal *callbacksReal;
    fLong *callbacksLong;

    // Constructors and Setters
    Pot(int pin, bool sleepEnable = POT_ENABLE_SLEEP, 
        float snapMultiplier = POT_DEFAULT_SNAP);

    Pot& setScale(int min, int max);
    Pot& setName(char* name);
    PotState getState();
    char* getStateName();
    
    Pot& setState(PotState state, long target);
    Pot& setState(PotState state);
    PotState updateState();
    void runCallbacks();
   
    int getPin();
    bool isReady();
   
    // Core API
    int getScaledValue();
    int getRawScaledValue();
    double makeReal(int i);
    double getReal();
    double getRawReal();

    long getReadTime();
    long getNextTarget();

    int index; // for internal lookups
    void update();
    void update(int value);
    void report(bool now = false);

    void addCallbackReal(void (*cb)(double));
    void addCallbackLong(void (*cb)(long));



};




//=================================================== CONSTRUCTORS AND SETTERS


Pot::Pot(int pin, bool sleepEnable, float snapMultiplier)
  : ResponsiveAnalogRead(pin, sleepEnable, snapMultiplier)
  , _pin(pin)
  , penalty(1)
  , usLastTrigger(0)
  , usLastRead(0)
  , usNextTarget(0)
  , reads(0)
  , hasOldChange(false)

  { 
   
    setState(INIT);
    if (POT_ENABLE_EDGE_SNAP) { enableEdgeSnap(); }
    //sprintf(name, "%d", pin);
    ResponsiveAnalogRead::update();
    setAnalogResolution(POT_MAX);
}

Pot& Pot::setScale(int min, int max) {
  scale_min = min;
  scale_max = max;
  return *this;
}

Pot& Pot::setName(char* _name) {
  name = _name;
  return *this;
}

int Pot::getPin() { return _pin; }



//========================================================= STATE MANIPULATION

PotState Pot::getState() { return state; }

char* Pot::getStateName() { return PotStateNames[getState()]; }

Pot& Pot::setState(PotState state) {
  if (state < ACTIVE) {
    return setState(state, min(usNextTarget, NOW::Eus()));
  } else {
    return setState(state, usLastRead + ADC_MICROS * penalty);
  }
}

Pot& Pot::setState(PotState state, long target) {
  usNextTarget = target;
  this->state = state;
  return *this;
}


// Assess our current state, then sets the next target read time.
// Should only be called immediately after a Pot has been read.

PotState Pot::updateState() {

  switch (state) {
    // only flushed after being read
    case INIT:
    case URGENT:
    case LATE:
    case RUNNING:
      return state;
  }

  if (usNextTarget < NOW::Eus()) {
    setState(LATE, min(usNextTarget, NOW::Eus()));
    return state;
  }

  if (isSleeping()) {
    setState(SLEEPY);
  } else {
    penalty = 1;
    setState(ACTIVE);
  }
 
  return state;
}

//================================================================== CALLBACKS

void Pot::addCallbackReal(fReal f) {
  // allocate more space and copy existing pointers
  fReal *temp = new fReal[countCBReal+1];
  if (countCBReal) {
    for (int i = 0; i < countCBReal; i++) {
      temp[i] = callbacksReal[i]; 
    }
    delete[] callbacksReal;
  }
  callbacksReal = temp;

  callbacksReal[countCBReal++] = f;
}

void Pot::addCallbackLong(fLong f) {
  // allocate more space and copy existing pointers
  fLong *temp = new fLong[countCBLong+1];
  if (countCBLong) {
    for (int i = 0; i < countCBLong; i++) {
      temp[i] = callbacksLong[i]; 
    }
    delete[] callbacksLong;
  }
  callbacksLong = temp;

  callbacksLong[countCBLong++] = f;
}

void Pot::runCallbacks() {
  for (int i = 0; i < countCBReal; i++) {
    callbacksReal[i](getReal());
  }
  for (int i = 0; i < countCBLong; i++) {
    callbacksLong[i](getScaledValue());
  }
}





//===================================================================== UPDATE

// Disable direct ADC access, but prioritize next read
void Pot::update() {
  setState(URGENT);
}

void Pot::update(int value) {
  usLastRead = micros();
  ResponsiveAnalogRead::update(value);

  if (isSleeping()) {
    penalty *= 2;
  }
  if (penalty > POT_SLEEPY_PENALTY) {
    penalty = POT_SLEEPY_PENALTY;
  }
  
  updateState();
  reads++;

  if (hasChanged()) {
    hasOldChange = true;
    runCallbacks();
  }
}


bool Pot::isReady() { return state != INIT; }

int Pot::getRawScaledValue() {
  if (!isReady()) { return 0; }
  return map(getRawValue(), POT_MIN, POT_MAX, scale_min, scale_max);
}

int Pot::getScaledValue() {
  hasOldChange = false;
  int value = getValue();
  int con = constrain(value, POT_MIN, POT_MAX);
  //return map(getValue(), POT_MIN, POT_MAX, scale_min, scale_max);
  return getValue();
}

double Pot::makeReal(int i) {
  return (double)i / (double)POT_MAX;
}

double Pot::getReal() {
  return makeReal(getValue());
}

double Pot::getRawReal() {
  return makeReal(getRawValue());
}




long Pot::getReadTime() {
  return usLastRead - usLastTrigger;
}

long Pot::getNextTarget() {
  return usNextTarget - (long)NOW::Eus();
}

void Pot::report(bool now) {
  if (now) rPots->spend();
  rPots->report("Pot[%s]:  %-10s %8d  read %8ld us  due %8ld us \n",
      name,
      getStateName(),
      getValue(),
      getReadTime(),
      getNextTarget());

  if (DEBUG_POT_CALLBACKS) {
    for (int i = 0; i < countCBReal; i++) {
      rPots->report(" -- CB Real [%d] %p\n", i, callbacksReal[i]);
    }
    for (int i = 0; i < countCBLong; i++) {
      rPots->report(" -- CB Long [%d] %p\n", i, callbacksLong[i]);
    }
  }
}



#endif // POTENTIOMETER
