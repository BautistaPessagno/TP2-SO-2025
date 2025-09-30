#ifndef LIB_H
#define LIB_H

#include <stdint.h>
#include <fonts.h>

#define EOF -1

void * memset(void * destination, int32_t character, uint64_t length);
void * memcpy(void * destination, const void * source, uint64_t length);
void printf(const char * string);

uint8_t getKeyboardBuffer(void);
uint8_t getKeyboardStatus(void);

uint8_t getSecond(void);
uint8_t getMinute(void);
uint8_t getHour(void);

#endif