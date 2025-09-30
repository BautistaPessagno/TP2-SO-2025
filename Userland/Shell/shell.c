#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include <sys.h>
#include <exceptions.h>

#ifdef ANSI_4_BIT_COLOR_SUPPORT
    #include <ansiColors.h>
#endif

static void * const snakeModuleAddress = (void*)0x500000;

#define MAX_BUFFER_SIZE 1024
#define HISTORY_SIZE 10

#define INC_MOD(x, m) x = (((x) + 1) % (m))
#define SUB_MOD(a, b, m) ((a) - (b) < 0 ? (m) - (b) + (a) : (a) - (b))
#define DEC_MOD(x, m) ((x) = SUB_MOD(x, 1, m))

static char buffer[MAX_BUFFER_SIZE];
static int buffer_dim = 0;

int clear(void);
int echo(void);
int exit(void);
int fontdec(void);
int font(void);
int help(void);
int history(void);
int man(void);
int snake(void);
int regs(void);
int time(void);

static void printPreviousCommand(enum REGISTERABLE_KEYS scancode);
static void printNextCommand(enum REGISTERABLE_KEYS scancode);

static uint8_t last_command_arrowed = 0;

typedef struct {
    char * name;
    int (*function)(void);
    char * description;
} Command;

/* All available commands. Sorted alphabetically by their name */
Command commands[] = {
    { .name = "clear",          .function = (int (*)(void))(unsigned long long)clear,           .description = "Clears the screen" },
    { .name = "divzero",        .function = (int (*)(void))(unsigned long long)_divzero,        .description = "Generates a division by zero exception" },
    { .name = "echo",           .function = (int (*)(void))(unsigned long long)echo ,           .description = "Prints the input string" },
    { .name = "exit",           .function = (int (*)(void))(unsigned long long)exit,            .description = "Command exits w/ the provided exit code or 0" },
    { .name = "font",           .function = (int (*)(void))(unsigned long long)font,            .description = "Increases or decreases the font size.\n\t\t\t\tUse:\n\t\t\t\t\t  + font increase\n\t\t\t\t\t  + font decrease" },
    { .name = "help",           .function = (int (*)(void))(unsigned long long)help,            .description = "Prints the available commands" },
    { .name = "history",        .function = (int (*)(void))(unsigned long long)history,         .description = "Prints the command history" },
    { .name = "invop",          .function = (int (*)(void))(unsigned long long)_invalidopcode,  .description = "Generates an invalid Opcode exception" },
    { .name = "regs",           .function = (int (*)(void))(unsigned long long)regs,            .description = "Prints the register snapshot, if any" },
    { .name = "man",            .function = (int (*)(void))(unsigned long long)man,             .description = "Prints the description of the provided command" },
    { .name = "snake",          .function = (int (*)(void))(unsigned long long)snake,           .description = "Launches the snake game" },
    { .name = "time",           .function = (int (*)(void))(unsigned long long)time,            .description = "Prints the current time" },
};

char command_history[HISTORY_SIZE][MAX_BUFFER_SIZE] = {0};
char command_history_buffer[MAX_BUFFER_SIZE] = {0};
uint8_t command_history_last = 0;

static uint64_t last_command_output = 0;

int main() {
    clear();

    registerKey(KP_UP_KEY, printPreviousCommand);
    registerKey(KP_DOWN_KEY, printNextCommand);

	while (1) {
        printf("\e[0mshell \e[0;32m$\e[0m ");

        signed char c;

        while(buffer_dim < MAX_BUFFER_SIZE && (c = getchar()) != '\n'){
            command_history_buffer[buffer_dim] = c;
            buffer[buffer_dim++] = c;
        }

        buffer[buffer_dim] = 0;
        command_history_buffer[buffer_dim] = 0;

        if(buffer_dim == MAX_BUFFER_SIZE){
            perror("\e[0;31mShell buffer overflow\e[0m\n");
            buffer[0] = buffer_dim = 0;
            while (c != '\n') c = getchar();
            continue ;
        };

        buffer[buffer_dim] = 0;
        
        char * command = strtok(buffer, " ");
        int i = 0;

        for (; i < sizeof(commands) / sizeof(Command); i++) {
            if (strcmp(commands[i].name, command) == 0) {
                last_command_output = commands[i].function();
                strncpy(command_history[command_history_last], command_history_buffer, 255);
                command_history[command_history_last][buffer_dim] = '\0';
                INC_MOD(command_history_last, HISTORY_SIZE);
                last_command_arrowed = command_history_last;
                break;
            }
        }

        // If the command is not found, ignore \n
        if ( i == sizeof(commands) / sizeof(Command) ) {
            if (command != NULL && *command != '\0') {
                fprintf(FD_STDERR, "\e[0;33mCommand not found:\e[0m %s\n", command);
            } else if (command == NULL) {
                printf("\n");
            }
        }
    
        buffer[0] = buffer_dim = 0;
    }

    __builtin_unreachable();
    return 0;
}

static void printPreviousCommand(enum REGISTERABLE_KEYS scancode) {
    clearInputBuffer();
    last_command_arrowed = SUB_MOD(last_command_arrowed, 1, HISTORY_SIZE);
    if (command_history[last_command_arrowed][0] != 0) {
        fprintf(FD_STDIN, command_history[last_command_arrowed]);
    }
}

static void printNextCommand(enum REGISTERABLE_KEYS scancode) {
    clearInputBuffer();
    last_command_arrowed = (last_command_arrowed + 1) % HISTORY_SIZE;
    if (command_history[last_command_arrowed][0] != 0) {
        fprintf(FD_STDIN, command_history[last_command_arrowed]);
    }
}

int history(void) {
    uint8_t last = command_history_last;
    DEC_MOD(last, HISTORY_SIZE);
    uint8_t i = 0;
    while (i < HISTORY_SIZE && command_history[last][0] != 0) {
        printf("%d. %s\n", i, command_history[last]);
        DEC_MOD(last, HISTORY_SIZE);
        i++;
    }
    return 0;
}

int time(void){
	int hour, minute, second;
    getDate(&hour, &minute, &second);
    printf("Current time: %xh %xm %xs\n", hour, minute, second);
    return 0;
}

int echo(void){
    for (int i = strlen("echo") + 1; i < buffer_dim; i++) {
        switch (buffer[i]) {
            case '\\':
                switch (buffer[i + 1]) {
                    case 'n':
                        printf("\n");
                        i++;
                        break;
                    case 'e':
                    #ifdef ANSI_4_BIT_COLOR_SUPPORT
                        i++;
                        parseANSI(buffer, &i); 
                    #else 
                        while (buffer[i] != 'm') i++; // ignores escape code, assumes valid format
                        i++;
                    #endif
                        break;
                    case 'r': 
                        printf("\r");
                        i++;
                        break;
                    case '\\':
                        i++;
                    default:
                        putchar(buffer[i]);
                        break;
                }
                break;
            case '$':
                if (buffer[i + 1] == '?'){
                    printf("%d", last_command_output);
                    i++;
                    break;
                }
            default:
                putchar(buffer[i]);
                break;
        }
    }
    printf("\n");
    return 0;
}

int help(void){
    printf("Available commands:\n");
    for (int i = 0; i < sizeof(commands) / sizeof(Command); i++) {
        printf("%s%s\t ---\t%s\n", commands[i].name, strlen(commands[i].name) < 4 ? "\t" : "", commands[i].description);
    }
    printf("\n");
    return 0;
}

int clear(void) {
    clearScreen();
    return 0;
}

int exit(void) {
    char * buffer = strtok(NULL, " ");
    int aux = 0;
    sscanf(buffer, "%d", &aux);
    return aux;
}

int font(void) {
    char * arg = strtok(NULL, " ");
    if (strcasecmp(arg, "increase") == 0) {
        return increaseFontSize();
    } else if (strcasecmp(arg, "decrease") == 0) {
        return decreaseFontSize();
    }
    
    perror("Invalid argument\n");
    return 0;
}

int man(void) {
    char * command = strtok(NULL, " ");

    if (command == NULL) {
        perror("No argument provided\n");
        return 1;
    }

    for (int i = 0; i < sizeof(commands) / sizeof(Command); i++) {
        if (strcasecmp(commands[i].name, command) == 0) {
            printf("Command: %s\nInformation: %s\n", commands[i].name, commands[i].description);
            return 0;
        }
    }

    perror("Command not found\n");
    return 1;
}

int regs(void) {
    const static char * register_names[] = {
        "rax", "rbx", "rcx", "rdx", "rbp", "rdi", "rsi", "r8 ", "r9 ", "r10", "r11", "r12", "r13", "r14", "r15", "rsp", "rip", "rflags"
    };

    int64_t registers[18];

    uint8_t aux = getRegisterSnapshot(registers);
    
    if (aux == 0) {
        perror("No register snapshot available\n");
        return 1;
    }

    printf("Latest register snapshot:\n");

    for (int i = 0; i < 18; i++) {
        printf("\e[0;34m%s\e[0m: %x\n", register_names[i], registers[i]);
    }

    return 0;
}

int snake(void) {
    return exec(snakeModuleAddress);
}
