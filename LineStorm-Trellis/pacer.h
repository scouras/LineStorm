#ifndef __PACER_H__
#define __PACER_H__

//#include <stdio.h>
//#include <string.h>
//#include "Print.h"
#include "now.h"


class Pacer {

  public:
    long interval;
    long spent;
    long next;
    bool enabled;
    void (*job)();

    Pacer(long i, bool e = false)
      : interval(i)
      , next(0)
      , enabled(e)
      , spent(0)
      { }

    Pacer& enable()             { enabled = true;  return *this; }
    Pacer& disable()            { enabled = false; return *this; }
    Pacer& setJob(void (*j)())  { job = j;         return *this; }

    // TODO: Should I check whether we're already spent and avoid respending?
    // Probably doesn't make a difference.
    void spend() {
      spent = NOW::Ems() + REPORT_SPEND_BUFFER;
      step();
    }

    bool ready() {
      if (not enabled        ) { return false; }
      if (spent >= NOW::Ems()) { return true;  }
      if (next  <= NOW::Ems()) { return true;  }
      return false;
    }

    void step() {
      next = NOW::Ems() + interval;
    }

    //================================================================ Logging
    
    /*
    void vreport(const char* format, va_list args) {
      vprintf(format, args);
    }
    */

    // Trigger pacer, so all subsequent checks this loop will also execute.
    void reportNow(const char *format, ... ) {
      va_list args;
      va_start(args, format);
      spend();
      vprintf(format, args);
      //report(format, args);
      va_end(args);
    }

    void report(const char *format, ... ) {
      if (not ready()) return;
      spend();
      va_list args;
      va_start(args, format);
      vprintf(format, args);
      //vreport(format, args);
      va_end(args);
    }

    //========================================================= Run a Function
    void run(void (*j)(), bool force = false) {
      if (force) spend();
      if (not ready()) return;
      spend();
      j();
    }
      
    void run(bool now = false) {
      run(this->job, now);
    }

    void debug() {
      printf("report %d r %d e   %ld next  %ld spent  %ld interval\n",
        ready(),
        enabled, 
        next,
        spent,
        interval);
    }
};

#endif // PACER

