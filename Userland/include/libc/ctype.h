#ifndef _LIBC_CTYPE_H_
#define _LIBC_CTYPE_H_

#define tolower(a) ((a) >= 'A' && (a) <= 'Z' ? (a) + ('a' - 'A') : (a) )
#define toupper(a) ((a) >= 'a' && (a) <= 'z' ? (a) - ('a' - 'A') : (a) )

#endif