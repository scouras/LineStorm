#ifndef __NOW_H__
#define __NOW_H__

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
 
  inline long Eus() { return us; }
  inline long Ems() { return us / MICROS_PER_MILLI;   }
  inline long Es () { return us / MICROS_PER_SECOND;  }
  inline long Em () { return us / MICROS_PER_MINUTE;  }
  inline long Eh () { return us / MICROS_PER_HOUR;    }

  inline long Dus() { return delta; }
  inline long Dms() { return delta / MICROS_PER_MILLI;  }
  inline long Ds()  { return delta / MICROS_PER_SECOND; }
  inline long Dm()  { return delta / MICROS_PER_MINUTE; }
  inline long Dh()  { return delta / MICROS_PER_HOUR;   }

  inline long Dus(long d) { return (us-d); }
  inline long Dms(long d) { return (us-d) / MICROS_PER_MILLI;  }
  inline long Ds(long d)  { return (us-d) / MICROS_PER_SECOND; }
  inline long Dm(long d)  { return (us-d) / MICROS_PER_MINUTE; }
  inline long Dh(long d)  { return (us-d) / MICROS_PER_HOUR;   }

  void tick () {
    long now = micros();
    ticks++;
    delta = now - us;
    us = now;
  }

  char* strElapsedMS(char* buf = NULL, bool now = false) {
    //if (now) { tick(); }
    if (buf == NULL) {
      buf = buffer_elapsed;
    }
    sprintf(buf, "[%02lu:%02lu] ", Em(), Es() % 60);
    return buf;
  }
};

#endif // __NOW_H__
