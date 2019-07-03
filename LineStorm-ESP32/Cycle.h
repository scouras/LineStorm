#ifndef __CYCLE_H__
#define __CYCLE_H__

#define PHASE_MAX        256.0
#define MODULATION_MAX  4096.0
#define DUTY_MAX           1.0
#define MILLION      1000000.0

#include <FastLED.h>

//////////////////////////////////////////////////////////////////////////////
//                               WAVEFORMS
//----------------------------------------------------------------------------
//
// Standard waveforms and boundary conditions, followed by functions and 
// defaults relevant to each waveform.
//
//////////////////////////////////////////////////////////////////////////////

typedef uint8_t (*WaveFunction)(double, double);

// Constant value, set by y
inline uint8_t wConstant(double phase, double mod = 255.0) {
  return mod + 0;
}

// Linear multiple of x
inline uint8_t wLinear(double phase, double mod = 1.0) {
  return lround(fmod(phase * mod * 16.0, 256.0));
}

// Stepped progression
// TODO: Needs to remember last phase and increment when dTheta has 
// surpassed the given step size.
inline uint8_t wStairs(double phase, double mod = 1.0) {
  //static double nextPhase = 0.0;
  //static ExpScale ES(1, 256);
  //double stepsize = ES.scale(mod);
  double stepsize = pow(2.0, round(mod * 8.0));
  double stair = phase - fmod(phase, stepsize);
  if (DEBUG_WAVE_FUNCTIONS) {
    pStrobe->report(" -- wSquare %d theta %d%% mod   %d size  %d stair\n",
        (int)phase, (int)(mod * 100.0), (int)stepsize, (int)stair);
  }
  return stair;
}

// Stepped progression
inline uint8_t wSquare(double phase, double mod = 0.5) {
  uint8_t duty = mod * (PHASE_MAX-1);
  if (DEBUG_WAVE_FUNCTIONS) {
    pStrobe->report(" -- wSquare %d theta %d%% mod %u duty\n",
        (int)phase, (int)(mod * 100.0), duty);
  }
  return squarewave8(lround(phase), duty);
}

// Stepped progression
inline uint8_t wQuad(double phase, double mod = 1024.0) {
  return quadwave8(phase);
}

// Stepped progression
inline uint8_t wCubic(double phase, double mod = 1024.0) {
  return cubicwave8(phase);
}

// Stepped progression
inline uint8_t wTriangle(double phase, double mod = 1024.0) {
  return triwave8(phase);
}

// Stepped progression
inline uint8_t wSin(double phase, double mod = 1024.0) {
  return sin8(phase);
}



enum Waveform { CONST, LINEAR, STAIRS, SQUARE, QUAD, CUBIC, TRIANGLE, SINE };

struct Wave {
  const char* name;
  WaveFunction waveFunction;
};

Wave waveDefaults[] = {
  { "CONSTANT",   wConstant },
  { "LINEAR",     wLinear   },
  { "STAIRS",     wStairs   },
  { "SQUARE",     wSquare   },
  { "QUAD",       wQuad     },
  { "CUBIC",      wCubic    },
  { "TRIANGLE",   wTriangle },
  { "SINE",       wSin      },
};






//////////////////////////////////////////////////////////////////////////////
//                                  CYCLE
//----------------------------------------------------------------------------
//
// You spin me right round baby.
//
// TODO: Make everything longs for now, and optimize later.
//
//
// The authoritative, defining parameters for the cycle are:
//    phase
//    wave.waveFunction
//    period
//    arcs
//    bound
//
// All other parameters are convenience precalculations, and can be rederived.
//////////////////////////////////////////////////////////////////////////////

// A microsecond precision oscillator

class Cycle {

  public:
    //===== Parameters

    const char* name;
    Wave wave;
    double phase;      // 360 deg is 65536 steps
    uint8_t value;    // transformed value

    double period;     // microseconds / loop
    double frequency;  // Hz

    double modulation; // tunable parameter sent to each waveFunction

    //===== Constructors and Setters

    Cycle(Waveform w = SQUARE, double us = MILLION, 
        double mod = 0.0, const char* n = "cycle");

    inline Cycle& setName(const char* n);
    inline Cycle& setPeriod(double us);         // period in microseconds
    inline Cycle& setFrequency(double Hz);      // frequency in hertz
    inline Cycle& setModulation(double mod);    // function modulation
    inline Cycle& setWave(Waveform w);         // set by waveform name
    
    //===== Methods

    // Map angle theta (or phase) based on duty cycle.
    /*
    double dutyRatio();
    double dutyRatio(long theta);
    long dutyMap();
    long dutyMap(long theta);
    */

    // Step forward a number of microseconds through the cycle
    void run(long us);

    void report();

};


//////////////////////////////////////////////////// CONSTRUCTOR AND ACCESSORS

// Copy a wave into place for later editing

Cycle::Cycle(Waveform w, double us, double mod, const char* n)
  : value(0)
  , phase(0.0)
  , wave(waveDefaults[w])
  , modulation(mod)
  {
    setPeriod(us);
    if (n != NULL) { setName(n); }
}


Cycle& Cycle::setName(const char* n) {
  name = n;
  return *this;
}

Cycle& Cycle::setFrequency(double Hz) {
  frequency = Hz;
  period = MILLION / frequency;
  return *this;
}

Cycle& Cycle::setPeriod(double us) {
  period = us;
  frequency = 1.0 / MILLION / period;
  return *this;
}

Cycle& Cycle::setModulation(double mod) {
  modulation = mod;
  return *this;
}

Cycle& Cycle::setWave(Waveform w) {
  wave = waveDefaults[w];
  return *this;
}




/*
double Cycle::dutyRatio(long theta) {

  double d = (double)duty;
  double t = (double)theta;
  double l = (double)DUTY_MAX;

  if (theta < duty) {
    return t / d / 2.0;
  } else {
    return (t-d) / (l-d) / 2.0 + 0.5;
  }
}
*/

void Cycle::run(long us) {
  phase += (double)us * frequency * PHASE_MAX / MILLION; 
  while (phase >= PHASE_MAX) {
    phase -= PHASE_MAX;
  }
  //phase = fmod(phase, PHASE_MAX);
  value = wave.waveFunction(phase, modulation);
}

static char* string_test1 = "foo1";
static char* string_test2 = "foo2\0";
static char char_buffer[100] = "foo3";
static String string_test4 = String("foo4");


void Cycle::report() {
  /* Going to try the exponential version for a bit
  pStrobe->report("Cycle %6s: %8s %8.2f Hz %8.2f ms  %8.3f mod  %5.2f -> %3u\n",
      name, 
      wave.name, 
      frequency, 
      period / 1000.0, 
      modulation, 
      phase, 
      value);
  */
  pStrobe->report("Cycle %6s: %8s %8.2e Hz %8.2e ms  %8.3e mod  %5.2e -> %3u\n",
      name, 
      wave.name, 
      frequency, 
      period / 1000.0, 
      modulation, 
      phase, 
      value);
}


#endif // __CYCLE_H__
