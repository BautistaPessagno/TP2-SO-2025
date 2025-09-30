#ifndef _ANSI_COLORS_H_
#define _ANSI_COLORS_H_

#ifdef ANSI_4_BIT_COLOR_SUPPORT

#include <stdint.h>

void parseANSI(const char * string, int * i);

#endif

#endif