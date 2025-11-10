#include <libc/stdio.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <exceptions.h>
#include <sys.h>

#ifdef ANSI_4_BIT_COLOR_SUPPORT
#include <ansiColors.h>
#endif

static void *const snakeModuleAddress = (void *)0x500000;

#define MAX_BUFFER_SIZE 1024
#define HISTORY_SIZE 10
#define MAX_ARGS 32
#define DEFAULT_PRIORITY 4
#define STDIN 0
#define STDOUT 1
#define STDERR 2

#ifndef DEV_NULL
#define DEV_NULL (-1)
#endif

#define INC_MOD(x, m) x = (((x) + 1) % (m))
#define SUB_MOD(a, b, m) ((a) - (b) < 0 ? (m) - (b) + (a) : (a) - (b))
#define DEC_MOD(x, m) ((x) = SUB_MOD(x, 1, m))

static char buffer[MAX_BUFFER_SIZE];
static int buffer_dim = 0;

static int cmd_clear(int argc, char **argv);
static int cmd_echo(int argc, char **argv);
static int cmd_exit(int argc, char **argv);
static int cmd_font(int argc, char **argv);
static int cmd_help(int argc, char **argv);
static int cmd_history(int argc, char **argv);
static int cmd_man(int argc, char **argv);
static int cmd_snake(int argc, char **argv);
static int cmd_regs(int argc, char **argv);
static int cmd_time(int argc, char **argv);

// External commands implemented in processes.c
int cmd_mem(int argc, char **argv);
int cmd_ps(int argc, char **argv);
int cmd_loop(int argc, char **argv);
int cmd_kill(int argc, char **argv);
int cmd_nice(int argc, char **argv);
int cmd_block(int argc, char **argv);
int cmd_cat(int argc, char **argv);
int cmd_wc(int argc, char **argv);
int cmd_filter(int argc, char **argv);
int cmd_mvar(int argc, char **argv);

// External commands implemented in Tests/
int test_mm(int argc, char **argv);
int test_processes(int argc, char **argv);
int test_sync(int argc, char **argv);
int test_prio(int argc, char **argv);

static void printPreviousCommand(enum REGISTERABLE_KEYS scancode);
static void printNextCommand(enum REGISTERABLE_KEYS scancode);
static uint8_t stripBackgroundMarker(void);
static void saveCommandToHistory(uint8_t run_in_background);

static uint8_t last_command_arrowed = 0;

typedef struct {
  char *name;
  int (*function)(int, char **);
  char *description;
} Command;

/* All available commands. Sorted alphabetically by their name */
Command commands[] = {
    {.name = "clear",
     .function = cmd_clear,
     .description = "Clears the screen"},
    {.name = "divzero",
     .function = (int (*)(int, char **))(unsigned long long)_divzero,
     .description = "Generates a division by zero exception"},
    {.name = "echo",
     .function = cmd_echo,
     .description = "Prints the input string"},
    {.name = "exit",
     .function = cmd_exit,
     .description = "Command exits w/ the provided exit code or 0"},
    {.name = "font",
     .function = cmd_font,
     .description =
         "Increases or decreases the font size.\n\t\t\t\tUse:\n\t\t\t\t\t  + "
         "font increase\n\t\t\t\t\t  + font decrease"},
    {.name = "help",
     .function = cmd_help,
     .description = "Prints the available commands"},
    {.name = "test_mm",
     .function = test_mm,
     .description = "Runs the memory test. Usage: test_mm [max_memory]"},
    {.name = "test_processes",
     .function = test_processes,
     .description =
         "Runs the processes test. Usage: test_processes [max_processes]"},
    {.name = "test_sync",
     .function = test_sync,
     .description = "Runs the sync test. Usage: test_sync [n] [use_sem]"},
    {.name = "test_prio",
     .function = test_prio,
     .description = "Runs the priority test. Usage: test_prio [max_value] "
                    "[max_prio (optional)]"},
    {.name = "history",
     .function = cmd_history,
     .description = "Prints the command history"},
    {.name = "invop",
     .function = (int (*)(int, char **))(unsigned long long)_invalidopcode,
     .description = "Generates an invalid Opcode exception"},
    {.name = "regs",
     .function = cmd_regs,
     .description = "Prints the register snapshot, if any"},
    {.name = "man",
     .function = cmd_man,
     .description = "Prints the description of the provided command"},
    {.name = "snake",
     .function = cmd_snake,
     .description = "Launches the snake game"},
    {.name = "time",
     .function = cmd_time,
     .description = "Prints the current time"},
    {.name = "mem",
     .function = cmd_mem,
     .description = "Prints memory usage: total, used, free"},
    {.name = "mvar",
     .function = cmd_mvar,
     .description =
         "Simula una MVar: mvar [escritores] [lectores] (default 2/2)"},
    {.name = "ps",
     .function = cmd_ps,
     .description = "Lists processes: pid, ppid, prio, state, fg/bg, stack"},
    {.name = "loop",
     .function = cmd_loop,
     .description = "Prints its pid periodically (usage: loop [periodMs])"},
    {.name = "kill",
     .function = cmd_kill,
     .description = "Kills a process by pid (usage: kill [pid])"},
    {.name = "nice",
     .function = cmd_nice,
     .description =
         "Changes a process priority (usage: nice [pid] [prio 0-4])"},
    {.name = "block",
     .function = cmd_block,
     .description = "Toggles process block/unblock (usage: block [pid])"},
    {.name = "cat",
     .function = cmd_cat,
     .description = "Reads from stdin and writes to stdout until newline"},
    {.name = "wc",
     .function = cmd_wc,
     .description = "Counts lines, words, bytes from stdin until newline"},
    {.name = "filter",
     .function = cmd_filter,
     .description = "Filters out vowels from stdin until newline"},
};

char command_history[HISTORY_SIZE][MAX_BUFFER_SIZE] = {0};
char command_history_buffer[MAX_BUFFER_SIZE] = {0};
uint8_t command_history_last = 0;

static uint64_t last_command_output = 0;

int main() {
  clearScreen();

  registerKey(KP_UP_KEY, printPreviousCommand);
  registerKey(KP_DOWN_KEY, printNextCommand);

  while (1) {
    printf("\e[0mshell \e[0;32m$\e[0m ");

    signed char c;

    while (buffer_dim < MAX_BUFFER_SIZE && (c = getchar()) != '\n') {
      command_history_buffer[buffer_dim] = c;
      buffer[buffer_dim++] = c;
    }

    buffer[buffer_dim] = 0;
    command_history_buffer[buffer_dim] = 0;

    if (buffer_dim == MAX_BUFFER_SIZE) {
      perror("\e[0;31mShell buffer overflow\e[0m\n");
      buffer[0] = buffer_dim = 0;
      while (c != '\n')
        c = getchar();
      continue;
    };

    buffer[buffer_dim] = 0;

    // checkea si hay un & al final y lo elimina
    uint8_t run_in_background = stripBackgroundMarker();

    // Detect simple two-stage pipeline: left | right
    char line_copy[MAX_BUFFER_SIZE];
    strncpy(line_copy, command_history_buffer, MAX_BUFFER_SIZE - 1);
    line_copy[MAX_BUFFER_SIZE - 1] = 0;
    char *pipe_pos = 0;
    {
      int __i = 0;
      while (line_copy[__i] != 0) {
        if (line_copy[__i] == '|') {
          pipe_pos = &line_copy[__i];
          break;
        }
        __i++;
      }
    }

    if (pipe_pos != NULL) {
      // Split into left and right, trim leading/trailing spaces
      *pipe_pos = 0;
      char *left = line_copy;
      char *right = pipe_pos + 1;
      // Trim left
      while (*left == ' ')
        left++;
      char *end = left + strlen(left);
      while (end > left && *(end - 1) == ' ') {
        *(--end) = 0;
      }
      // Trim right
      while (*right == ' ')
        right++;
      end = right + strlen(right);
      while (end > right && *(end - 1) == ' ') {
        *(--end) = 0;
      }

      // Tokenize both sides
      char *argvL[MAX_ARGS] = {0}, *argvR[MAX_ARGS] = {0};
      int argcL = 0, argcR = 0;
      char *tok = strtok(left, " ");
      while (tok != NULL && argcL < MAX_ARGS - 1) {
        argvL[argcL++] = tok;
        tok = strtok(NULL, " ");
      }
      argvL[argcL] = NULL;
      tok = strtok(right, " ");
      while (tok != NULL && argcR < MAX_ARGS - 1) {
        argvR[argcR++] = tok;
        tok = strtok(NULL, " ");
      }
      argvR[argcR] = NULL;

      if (argcL == 0 || argcR == 0) {
        // Invalid pipeline, ignore silently
        printf("\n");
        buffer[0] = buffer_dim = 0;
        continue;
      }

      // Find commands
      int leftIdx = -1, rightIdx = -1;
      for (int ii = 0; ii < sizeof(commands) / sizeof(Command); ii++) {
        if (strcmp(commands[ii].name, argvL[0]) == 0)
          leftIdx = ii;
        if (strcmp(commands[ii].name, argvR[0]) == 0)
          rightIdx = ii;
      }
      if (leftIdx == -1) {
        fprintf(FD_STDERR, "\e[0;33mCommand not found:\e[0m %s\n", argvL[0]);
        printf("\n");
        buffer[0] = buffer_dim = 0;
        continue;
      }
      if (rightIdx == -1) {
        fprintf(FD_STDERR, "\e[0;33mCommand not found:\e[0m %s\n", argvR[0]);
        printf("\n");
        buffer[0] = buffer_dim = 0;
        continue;
      }

      int16_t p = pipeGet();
      if (p < 0) {
        // If pipe cannot be created, abort silently
        printf("\n");
        buffer[0] = buffer_dim = 0;
        continue;
      }

      int16_t fdsL[3] = {run_in_background ? DEV_NULL : STDIN, p, STDERR};
      int16_t fdsR[3] = {p, run_in_background ? DEV_NULL : STDOUT, STDERR};

      char **argsL = (argcL > 1) ? &argvL[1] : NULL;
      char **argsR = (argcR > 1) ? &argvR[1] : NULL;

      int32_t pidL =
          createProcessWithFds(commands[leftIdx].function, argsL,
                               commands[leftIdx].name, DEFAULT_PRIORITY, fdsL);
      int32_t pidR =
          createProcessWithFds(commands[rightIdx].function, argsR,
                               commands[rightIdx].name, DEFAULT_PRIORITY, fdsR);

      if (!run_in_background) {
        if (pidL >= 0)
          waitpid(pidL);
        if (pidR >= 0)
          waitpid(pidR);
      }

      saveCommandToHistory(run_in_background);

      buffer[0] = buffer_dim = 0;
      continue;
    }

    // Parse argv from the full command line (preserved in
    // command_history_buffer)
    char *argv[MAX_ARGS] = {0};
    int argc = 0;
    char *token = strtok(command_history_buffer, " ");
    while (token != NULL && argc < MAX_ARGS - 1) {
      argv[argc++] = token;
      token = strtok(NULL, " ");
    }
    argv[argc] = NULL;

    if (argc == 0) {
      printf("\n");
      buffer[0] = buffer_dim = 0;
      continue;
    }

    char *command = argv[0];
    char **command_args = argc > 1 ? &argv[1] : NULL;
    int i = 0;

    for (; i < sizeof(commands) / sizeof(Command); i++) {
      if (strcmp(commands[i].name, command) == 0) {
        // Run the command as a separate process
        int16_t fds[3] = {run_in_background ? DEV_NULL : STDIN,
                          run_in_background ? DEV_NULL : STDOUT, STDERR};
        int32_t pid =
            createProcessWithFds(commands[i].function, command_args,
                                 commands[i].name, DEFAULT_PRIORITY, fds);
        if (pid >= 0) {
          if (run_in_background) {
            last_command_output = 0;
          } else {
            last_command_output = waitpid(pid);
          }
        } else {
          perror("\e[0;31mFailed to create process\e[0m\n");
          last_command_output = (uint64_t)-1;
        }
        saveCommandToHistory(run_in_background);
        break;
      }
    }

    // If the command is not found, ignore \n
    if (i == sizeof(commands) / sizeof(Command)) {
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

static uint8_t stripBackgroundMarker(void) {
  int idx = buffer_dim - 1;
  while (idx >= 0 && (command_history_buffer[idx] == ' ' ||
                      command_history_buffer[idx] == '\t')) {
    command_history_buffer[idx] = 0;
    buffer[idx] = 0;
    idx--;
  }

  uint8_t run_in_background = 0;
  if (idx >= 0 && command_history_buffer[idx] == '&') {
    run_in_background = 1;
    command_history_buffer[idx] = 0;
    buffer[idx] = 0;
    idx--;
    while (idx >= 0 && (command_history_buffer[idx] == ' ' ||
                        command_history_buffer[idx] == '\t')) {
      command_history_buffer[idx] = 0;
      buffer[idx] = 0;
      idx--;
    }
  }

  buffer_dim = idx + 1;
  buffer[buffer_dim] = 0;
  command_history_buffer[buffer_dim] = 0;
  return run_in_background;
}

static void saveCommandToHistory(uint8_t run_in_background) {
  const size_t copy_limit = 255;
  size_t original_len = buffer_dim;
  size_t stored_len = original_len;

  if (run_in_background && stored_len + 2 < MAX_BUFFER_SIZE) {
    command_history_buffer[stored_len++] = ' ';
    command_history_buffer[stored_len++] = '&';
    command_history_buffer[stored_len] = 0;
  }

  strncpy(command_history[command_history_last], command_history_buffer,
          copy_limit);
  command_history[command_history_last][stored_len] = '\0';
  INC_MOD(command_history_last, HISTORY_SIZE);
  last_command_arrowed = command_history_last;

  command_history_buffer[original_len] = 0;
  buffer_dim = original_len;
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

static int cmd_history(int argc, char **argv) {
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

static int cmd_time(int argc, char **argv) {
  int hour, minute, second;
  getDate(&hour, &minute, &second);
  printf("Current time: %xh %xm %xs\n", hour, minute, second);
  return 0;
}

static int cmd_echo(int argc, char **argv) {
  for (int i = 0; i < argc; i++) {
    printf("%s", argv[i]);
    if (i + 1 < argc)
      printf(" ");
  }
  printf("\n");
  return 0;
}

static int cmd_help(int argc, char **argv) {
  printf("Available commands:\n");
  for (int i = 0; i < sizeof(commands) / sizeof(Command); i++) {
    printf("%s%s\t ---\t%s\n", commands[i].name,
           strlen(commands[i].name) < 4 ? "\t" : "", commands[i].description);
  }
  printf("\n");
  return;
}

static int cmd_clear(int argc, char **argv) {
  clearScreen();
  return 0;
}

static int cmd_exit(int argc, char **argv) {
  int aux = 0;
  if (argc > 0) {
    sscanf(argv[0], "%d", &aux);
  }
  return aux;
}

static int cmd_font(int argc, char **argv) {
  if (argc < 1) {
    perror("Invalid argument\n");
    return 1;
  }
  if (strcasecmp(argv[0], "increase") == 0) {
    return increaseFontSize();
  } else if (strcasecmp(argv[0], "decrease") == 0) {
    return decreaseFontSize();
  }

  perror("Invalid argument\n");
  return 0;
}

static int cmd_man(int argc, char **argv) {
  char *command = argc > 0 ? argv[0] : NULL;

  if (command == NULL) {
    perror("No argument provided\n");
    return 1;
  }

  for (int i = 0; i < sizeof(commands) / sizeof(Command); i++) {
    if (strcasecmp(commands[i].name, command) == 0) {
      printf("Command: %s\nInformation: %s\n", commands[i].name,
             commands[i].description);
      return 0;
    }
  }

  perror("Command not found\n");
  return 1;
}

static int cmd_regs(int argc, char **argv) {
  const static char *register_names[] = {
      "rax", "rbx", "rcx", "rdx", "rbp", "rdi", "rsi", "r8 ", "r9 ",
      "r10", "r11", "r12", "r13", "r14", "r15", "rsp", "rip", "rflags"};

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

static int cmd_snake(int argc, char **argv) { return exec(snakeModuleAddress); }
