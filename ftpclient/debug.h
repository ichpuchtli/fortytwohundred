#ifndef DEBUG_H
#define DEBUG_H
#include <stdarg.h>
#include <stdio.h>

int debug = 0;

void _debug(const char *format, va_list args) {
    fprintf(stderr, "Debug: ");
    
    vfprintf(stderr, format, args);
}

void d(const char *format, ...) {
    if (debug) {
        va_list args;
        va_start(args, format);
        _debug(format, args);
        va_end(args);
    }
}
#endif
