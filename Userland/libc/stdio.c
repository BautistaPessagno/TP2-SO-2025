#include <stdio.h>
#include <string.h>
#include <syscalls.h>

#ifdef ANSI_4_BIT_COLOR_SUPPORT
    #include <ansiColors.h>
#endif

static char buffer[64] = {0};

static uint32_t uintToBase(uint64_t value, char * buffer, uint32_t base);
static void printBase(int fd, int num, int base);
// static void printFloat(int fd, float num);

void puts(const char * str) {
    printf(str);
    printf("\n");
}

void vfprintf(int fd, const char * format, va_list args) {
    int i = 0;
    while (format[i] != 0) {
        switch (format[i]) {
        case '\e':
        #ifdef ANSI_4_BIT_COLOR_SUPPORT
            sys_write(fd, &format[i], 0); // "writes" (ignored because of count=0) \e char to account for fd changes
            parseANSI(format, &i);
            break ;
        #else
            while(format[i] != 'm') i++; // ignore ANSI escape codes, assumes valid \e[X,Ym format
            i++;
            break ;
        #endif
        case '%':
            i++;
            switch (format[i]) {
                case 'x': printBase(fd, va_arg(args, int), 16); break ;
                case 'd': printBase(fd, va_arg(args, int), 10); break ;
                case 'o': printBase(fd, va_arg(args, int), 8); break ;
                case 'b': printBase(fd, va_arg(args, int), 2); break ;
                // case 'f': printFloat(fd, va_arg(args, double)); break ;
                case 'c': {
                    char c = (char) va_arg(args, int);
                    sys_write(fd, &c, 1);
                    break ;
                }
                case 's': fprintf(fd, va_arg(args, char *)); break ;
                case '%': sys_write(fd, "%", 1); break ;
            }
            i++;
            break ;
        default:
            sys_write(fd, &format[i], 1);
            i++;
            break ;
        }
    }
}

void printf(const char * format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(FD_STDOUT, format, args);
    va_end(args);
}

void fprintf(int fd, const char * str, ...) {
    va_list args;
    va_start(args, str);
    vfprintf(fd, str, args);
    va_end(args);
}

int vscanf(const char * format, va_list args) {
    int i = 0;
    int args_read = 0;
    char c;
    while (format[i] != 0) {
        switch (format[i]) {
            case '%':
                i++;
                switch (format[i]) {
                    case 'd':
                        int64_t num = 0;
                        uint8_t negative = 0;

                        c = getchar();

                        if (c != '-' && (c < '0' || c > '9')) {
                            while ((c = getchar()) != '\n'); // empty input buffer
                            break ;
                        };

                        do {
                            if (c == '-')
                                negative = 1;
                            else
                                num = num * 10 + c - '0';
                        } while (((c = getchar()) >= '0' && c <= '9') || (num == 0 && c == '-'));

                        *va_arg(args, int *) = num * (negative ? -1 : 1);
                        args_read++;
                        break;
                    case 'c':
                        *va_arg(args, char *) = getchar();
                        args_read++;
                        break;
                    case 's':
                        char * str = va_arg(args, char *);
                        while ((c = getchar()) != ' ' && c != '\n') {
                            *str = c;
                            str++;
                        }
                        *str = 0;
                        args_read++;
                        break;
                }
        }
        i++;
    }
    return args_read;               
}

int vsscanf(const char * buffer, const char * format, va_list args) {
    int form_i = 0;
    int buf_i = 0;
    int args_read = 0;
    while (format[form_i] != 0) {
        switch (format[form_i]) {
            case '%':
                form_i++;
                switch (format[form_i]) {
                    case 'd' : {
                        int num = 0;
                        uint8_t read_num = 0, negative = 0;

                        if (buffer[buf_i] != '-' && (buffer[buf_i] < '0' || buffer[buf_i] > '9')) break;

                        if (buffer[buf_i] == '-') { negative = 1; buf_i++; };
                        
                        while (buffer[buf_i] >= '0' && buffer[buf_i] <= '9') {
                            num = num * 10 + buffer[buf_i] - '0';
                            buf_i++;
                            read_num = 1;
                        }

                        *va_arg(args, int *) = num * (negative ? -1 : 1);
                        args_read += read_num;
                        break;
                    }
                    case 'c': {
                        *va_arg(args, char *) = buffer[buf_i++];
                        args_read++;
                        break;
                    }
                    case 's': {
                        char * str = va_arg(args, char *);
                        while (buffer[buf_i] != ' ' && buffer[buf_i] != '\n' && buffer[buf_i] != 0) {
                            *(str++) = buffer[buf_i];
                            str++;
                            buf_i++;
                        }
                        *str = 0;
                        args_read++;
                        break;
                    }
                    default:
                        break;
                }
        }
        form_i++;
    }
    return args_read;
}

int sscanf(const char * str, const char * format, ...) {
    va_list args;
    va_start(args, format);
    int aux = vsscanf(str, format, args);
    va_end(args);
    return aux;
}

int scanf(const char * format, ...) {
    va_list args;
    va_start(args, format);
    int aux = vscanf(format, args);
    va_end(args);
    return aux;
}

void perror(const char * s1) {
    fprintf(FD_STDERR, s1);
}

int getchar(void) {
    signed char c[1];
    while(sys_read(FD_STDIN, c, 1) == -1);
    return c[0];
}

void putchar(const char c) {
    sys_write(FD_STDOUT, &c, 1);
};

static uint32_t uintToBase(uint64_t value, char * buffer, uint32_t base)
{
	char *p = buffer;
	char *p1, *p2;
	uint32_t digits = 0;

	//Calculate characters for each digit
	do
	{
		uint32_t remainder = value % base;
		*p++ = (remainder < 10) ? remainder + '0' : remainder + 'A' - 10;
		digits++;
	}
	while (value /= base);

	// Terminate string in buffer.
	*p = 0;

	//Reverse string in buffer.
	p1 = buffer;
	p2 = p - 1;
	while (p1 < p2)
	{
		char tmp = *p1;
		*p1 = *p2;
		*p2 = tmp;
		p1++;
		p2--;
	}

	return digits;
}

static void printBase(int fd, int num, int base) {
    if (num < 0) fprintf(fd, "-");
    uintToBase(num, buffer, base);
    fprintf(fd, buffer);
}
