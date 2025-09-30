#include <string.h>
#include <stdlib.h>
#include <ctype.h>

int strlen(const char * str) {
    int i = 0;
    while (str[i] != 0) {
        i++;
    }
    return i;
}

int strcmp(char * str1, char * str2) {
    int i = 0;
    while (str1[i] != 0 && str2[i] != 0) {
        if (str1[i] != str2[i]) {
            return str1[i] - str2[i];
        }
        i++;
    }
    return str1[i] - str2[i];
}

int strcasecmp(char * str1, char * str2) {
    int i = 0;
    while (str1[i] != 0 && str2[i] != 0) {
        if (toupper(str1[i]) != toupper(str2[i])) {
            return str1[i] - str2[i];
        }
        i++;
    }
    return str1[i] - str2[i];
}

void strcpy(char * dest, char * src) {
    int i = 0;
    while (src[i] != 0) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = 0;
}

void strncpy(char * dest, char * src, int n) {
    int i = 0;
    while (src[i] != 0 && i < n) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = 0;
}

char * strtok(char * s1, const char * s2) {
    static char * last;

    if (s1 != NULL) {
        last = s1;
    } else {
        if (*last == 0) {
            return NULL;
        }
        s1 = last;
    }

    while (*last != 0) {
        int i = 0;
        while (s2[i] != 0) {
            if (*last == s2[i]) {
                *last = 0;
                last++;
                return s1;
            }
            i++;
        }
        last++;
    }

    return s1;
}
