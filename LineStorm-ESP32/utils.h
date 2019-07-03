#ifndef __LINE_ART_UTILITIES__
#define __LINE_ART_UTILITIES__

//////////////////////////////////////////////////////////////////////////////
//                          HANDY REPORTERS
//////////////////////////////////////////////////////////////////////////////


//===================================================================== MEMORY

#ifdef ENABLE_MEMORY_REPORT
#include <MemoryUsage.h>
#endif

Pacer *pMemory;

void reportMemFree(bool now = false) {
  #ifdef ENABLE_MEMORY_REPORT
  if (now) { pMemory->spend(); }
  pMemory->report("Memory Free: %4d  Stack: %4d\n", mu_freeRam(), mu_StackCount());
  #endif
}

void reportMemMap(bool now = false) {
  #ifdef ENABLE_MEMORY_REPORT
  if (now) { pMemory->spend(); }
  Serial.println();
  SRamDisplay();
  Serial.println();
  #endif
}

#endif // LINE_ART_UTILITIES
