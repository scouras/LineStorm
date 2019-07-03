#ifndef __REPORT__
#define __REPORT__

#include "Now.h"

#define LOG Serial
#include <log.h>


/////////////////////////////////////////////////// PREFERRED LOGGING FUNCTION



void report(const char *format, va_list argp) {
  /*
  char* elapsed = NOW::strElapsedMS();
  size_t len = strlen(elapsed) + strlen(format) + 1;
  char* combo = new char[len];
  strcpy(combo, elapsed);
  strcat(combo, format);
  _log_printf(combo, argp);
  */
  _log_printf(format, argp);
  //free(combo);
}

void reportNow(const char *format, va_list argp) {
  report(format, argp);
}

void report(const char *format, ...) {
  va_list args;
  va_start(args, format);
  report(format, args);
  va_end(args);
}

void reportNow(const char *format, ...) {
  va_list args;
  va_start(args, format);
  reportNow(format, args);
  va_end(args);
}



//////////////////////////////////////////////////////// RATE-LIMITED REPORTER

class Reporter {

  public:
    long interval;
    long spent;
    long next;
    bool enabled;
    void (*job)();

    Reporter(long i, bool e = false)
      : interval(i)
      , next(0)
      , enabled(e)
      , spent(0)
      { }

    inline Reporter& enable()             { enabled = true;  return *this; }
    inline Reporter& disable()            { enabled = false; return *this; }
    inline Reporter& setJob(void (*j)())  { job = j;         return *this; }

    inline void spend() {
      spent = NOW::Ems() + REPORT_SPEND_BUFFER;
      step();
    }

    inline bool ready() {
      if (not enabled        ) { return false; }
      if (spent >= NOW::Ems()) { return true;  }
      if (next  <= NOW::Ems()) { return true;  }
      return false;
    }

    inline void step() {
      next = NOW::Ems() + interval;
    }

    //================================================================ Logging

    // Log with exact timing by forcing NOW to update
    inline void reportNow(const char *format, ... ) {
      //NOW::tick();
      spend();
      va_list args;
      va_start(args, format);
      ::reportNow(format, args);
      va_end(args);
    }

    inline void report(const char *format, ... ) {
      if (not ready()) return;
      spend();
      va_list args;
      va_start(args, format);
      ::report(format, args);
      va_end(args);
    }

    //========================================================= Run a Function
    inline void run(void (*j)(), bool now = false) {
      if (now) {
        //NOW::tick();
        spend();
      }
      if (not ready()) return;
      spend();
      j();
    }
      
    inline void run(bool now = false) {
      run(this->job, now);
    }

    void debug() {
      log_printf("report %d r %d e   %ld next  %ld spent  %ld interval\n",
        ready(),
        enabled, 
        next,
        spent,
        interval);
    }
};

#endif // REPORT
