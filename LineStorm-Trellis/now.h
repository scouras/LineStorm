#ifndef __NOW_H__
#define __NOW_H__

/* ******************************* NOW ***************************************
* Provides a synchronized time for each execution of loop() in whichever SI 
* unit is convenient. Also trackes elapsed time between loops.
*
* Time is tracked in microseconds with a signed long (4 bytes)
*   2^32 / 2 = 2,147,483,648 us        or       24.8 days
* This is for high-speed real-time operations, so 25 days is plenty.
*************************************************************************** */

#define BUFFER_ELAPSED               16

#define MICROS_PER_MILLI           1000
#define MICROS_PER_SECOND       1000000
#define MICROS_PER_MINUTE      60000000
#define MICROS_PER_HOUR       360000000

#define REPORT_SPEND_BUFFER           1

namespace NOW {
    
  long ticks = 0;
  long us    = micros();
  long delta = 0;
  
  char buffer_elapsed[BUFFER_ELAPSED];
 
  // Total Elapsed Since Boot
  inline long Eus() { return us; }
  inline long Ems() { return us / MICROS_PER_MILLI;   }
  inline long Es () { return us / MICROS_PER_SECOND;  }
  inline long Em () { return us / MICROS_PER_MINUTE;  }
  inline long Eh () { return us / MICROS_PER_HOUR;    }

  // Elapsed Since Last Loop
  inline long Dus() { return delta; }
  inline long Dms() { return delta / MICROS_PER_MILLI;  }
  inline long Ds()  { return delta / MICROS_PER_SECOND; }
  inline long Dm()  { return delta / MICROS_PER_MINUTE; }
  inline long Dh()  { return delta / MICROS_PER_HOUR;   }

  // Elapsed Between a Reference Time and Now
  inline long Dus(long d) { return (us-d); }
  inline long Dms(long d) { return (us-d) / MICROS_PER_MILLI;  }
  inline long Ds(long d)  { return (us-d) / MICROS_PER_SECOND; }
  inline long Dm(long d)  { return (us-d) / MICROS_PER_MINUTE; }
  inline long Dh(long d)  { return (us-d) / MICROS_PER_HOUR;   }

  // Call at the beginning of every loop (or similar regular milestone)
  void tick () {
    long now = micros();
    ticks++;
    delta = now - us;
    us = now;
  }

  /** Human Readable Time Elapsed
   *
   *  @param buf      optional char* buffer, or we'll return one
   *  @param micros   elapsed micros, default NOW
   *
   *  !! Not implemented yet !!
   *  @param max      highest needed interval, default auto
   *  @param min      smallest interval, default seconds
   *
   */
  const char* HRT(char* buf = NULL, long micros = 0) {
                  //char max = NULL, char min = NULL) {

    if (buf == NULL) buf = buffer_elapsed;

    sprintf(buf, "[%02lu:%02lu] ", Dm(micros), Ds(micros) % 60);
    return buf;
  }
};

#endif // NOW
