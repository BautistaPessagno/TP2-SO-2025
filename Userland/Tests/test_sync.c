// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <stdint.h>
#include <stdio.h>
#include <libsys/sys.h>
#include "tests/test_util.h"

#define SEM_ID 67
#define TOTAL_PAIR_PROCESSES 2

int64_t global; // shared memory

int16_t static fileDescriptors[3] = {0, 1, 2};

void slowInc(int64_t *p, int64_t inc) {
  uint64_t aux = *p;
  yield(); // This makes the race condition highly probable
  aux += inc;
  *p = aux;
}

int my_process_inc(int argc, char **argv) {
  uint64_t n;
  int8_t inc;
  int8_t use_sem;

  if (argc != 3)
    return -1;

  if ((n = satoi(argv[0])) <= 0)
    return -1;
  if ((inc = satoi(argv[1])) == 0)
    return -1;
  if ((use_sem = satoi(argv[2])) < 0)
    return -1;

  if (use_sem)
    if (semOpen(SEM_ID, 1) == -1) {
      printf("test_sync: ERROR opening semaphore\n");
      return -1;
    }

  uint64_t i;
  for (i = 0; i < n; i++) {
    if (use_sem)
      semWait(SEM_ID);
    slowInc(&global, inc);
    if (use_sem)
      semPost(SEM_ID);
  }


  return 0;
}

uint64_t test_sync(uint64_t argc, char *argv[]) { //{n, use_sem, 0}
  uint64_t pids[2 * TOTAL_PAIR_PROCESSES];

  if (argc != 2)
    return -1;

  char *argvDec[] = {argv[0], "-1", argv[1], NULL};
  char *argvInc[] = {argv[0], "1", argv[1], NULL};

  global = 0;

  uint64_t i;
  for (i = 0; i < TOTAL_PAIR_PROCESSES; i++) {
    pids[i] = createProcessWithFds(my_process_inc, argvDec, "my_process_inc", 4, fileDescriptors);
    pids[i + TOTAL_PAIR_PROCESSES] = createProcessWithFds(my_process_inc, argvInc, "my_process_inc", 4, fileDescriptors);
  }

  for (i = 0; i < TOTAL_PAIR_PROCESSES; i++) {
    waitpid(pids[i]);
    waitpid(pids[i + TOTAL_PAIR_PROCESSES]);
  }

  if (satoi(argv[1]) == 1)
    semClose(SEM_ID);

  printf("Final value: %d\n", global);

  return 0;
}
