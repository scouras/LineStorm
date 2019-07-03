#ifndef __LOG_H__
#define __LOG_H__

#if defined(LOG)
#define log_print(...) LOG.print(__VA_ARGS__)
#define log_println(...) LOG.println(__VA_ARGS__)
#define log_printf(...) _log_printf(__VA_ARGS__)

#define LOG_BUFFER 512

const size_t _log_printf(const char *format, va_list args) {
  //Serial.println(F(" __ log va_list"));
  char temp[LOG_BUFFER];
  char* buffer = temp;
  size_t len = vsnprintf(temp, sizeof(temp), format, args);

  // if buffer is too small, reallocate and retry
  if (len > sizeof(temp) - 1) {
    buffer = (char *) malloc(len+1);
    //buffer = new char[len + 1];
    if (!buffer) {
        return 0;
    }
    vsnprintf(buffer, len + 1, format, args);
  }
  //len = write((const uint8_t*) buffer, len);
  log_print(buffer);
  if (buffer != temp) {
    free(buffer);
    //delete[] buffer;
  }
  return len;
}

const size_t _log_printf(const char *format, ...) {
  //Serial.println(F(" __ log ..."));
  va_list args;
  va_start(args, format);
  size_t len = _log_printf(format, args);
  va_end(args);
  return len;
}

#else
#define log_print(...)((void) 0)
#define log_println(...)((void) 0)
#define log_printf(...)((void) 0)
#endif

#endif
