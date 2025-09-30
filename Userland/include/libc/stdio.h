#ifndef _LIBC_STDIO_H_
#define _LIBC_STDIO_H_

#include <string.h>
#include <stdarg.h>

#define FD_STDIN  0
#define FD_STDOUT 1
#define FD_STDERR 2

void puts(const char * str);
void vprintf(const char * str, va_list args);
void printf(const char * str, ...);
void fprintf(int fd, const char * str, ...);
int vscanf(const char * format, va_list args);
int vsscanf(const char * buffer, const char * format, va_list args);
int sscanf(const char * str, const char * format, ...);
int scanf(const char * format, ...);
int getchar();
void putchar(const char c);

#endif