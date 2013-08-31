/**
 * @file srtp_debug.h
 * @brief debug utility
 * @author Sam Macpherson
 * @author Chris Ponticello
 *
 * Copyright 2013  Sam Macpherson, Chris Ponticello
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef SRTP_DEBUG_H
#define SRTP_DEBUG_H

#include <stdarg.h>
#include <stdio.h>

/**
 * @brief debug printf wrapper
 *
 * @param format variable arguments
 * @param ... variable arguments
 */
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

#endif // SRTP_DEBUG_H
