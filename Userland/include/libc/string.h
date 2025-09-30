#ifndef _LIBC_STRING_H_
#define _LIBC_STRING_H_

#include <stddef.h>

int strlen(const char * str);
int strcmp(char * str1, char * str2);
int strcasecmp(char * str1, char * str2);
void strcpy(char * dest, char * src);
void strncpy(char * dest, char * src, int n);
void perror(const char * s1);
char * strtok(char * s1, const char * s2);

#endif