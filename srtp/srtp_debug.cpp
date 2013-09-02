#include "srtp_debug.h"

void debug(const char *format, ...) {

 #ifdef SRTP_DEBUG
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
 #else
  (void) format;
 #endif

}
