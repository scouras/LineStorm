#ifndef __WAVEFORM_H__
#define __WAVEFORM_H__

#define PHASE_MAX        1.0
#define MODULATION_MAX  4096.0
#define DUTY_MAX           1.0
#define MILLION      1000000.0



/*****************************************************************************
 *                            MODULATED WAVEFORMS
 *----------------------------------------------------------------------------
 *
 * Standard, modulatable waveforms to give different shapes to anything.
 * PHASE and MODULATION parameters, as well as RETURN values, are in a 
 * [0.0,1.0) (doubl;e) range before wrapping. They are intended to be scaled
 * to usable values by their calling context.
 *
 ****************************************************************************/




class WaveForm {

  public:
    
    static enum Waveform { CONST, LINEAR, STAIRS, SQUARE, 
      SINE, CUBIC, QUAD, TRIANGLE};
    
    typedef double (*WaveFunction)(double, double);
    
    const char* name;
    const WaveFunction waveFunction;

}























































#endif // __WAVEFORM_H__
