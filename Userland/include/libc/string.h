#ifndef _LIBC_STRING_H_
#define _LIBC_STRING_H_

#include <stddef.h>
#include <stdint.h>

int strlen(const char * str);
int strcmp(const char * str1, const char * str2);
int strcasecmp(const char * str1, const char * str2);
void strcpy(char * dest, const char * src);
void strncpy(char * dest, const char * src, int n);
void perror(const char * s1);
char * strtok(char * s1, const char * s2);
void * memset(void * destination, int32_t character, uint64_t length);

#endif