#ifndef SRTP_DEBUG_H
#define SRTP_DEBUG_H

#include <stdarg.h>
#include <stdio.h>

void debug(const char *format, ...) {

 #ifdef SRTP_DEBUG
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
 #endif

}

#endif // SRTP_DEBUG_H
