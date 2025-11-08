#include <stdint.h>
#include <stdio.h>
#include <libsys/sys.h>
#include "tests/test_util.h"

int static fileDescriptors[3] = {0, 1, 2};

uint64_t max_value = 0;

#define MAX_PRIORITY 4


void zero_to_max() {
  uint64_t value = 0;

  while (value++ != max_value);

  printf("PROCESS %d DONE!\n", getpid());
}

int test_prio(int argc, char **argv) {

  if (argc < 1){
    printf("test_prio: ERROR invalid arguments\n");
    return -1;
  }
  uint64_t total_processes;
  if ((total_processes = satoi(argv[0])) <= 0)
    return -1;

    uint8_t prio[total_processes];

  //dado el max le asignamos un valor aleatorio entre 0 y max_value
  for (uint64_t i = 0; i < total_processes; i++) {
    prio[i] = GetUniform(MAX_PRIORITY);
  }

  int64_t pids[total_processes];
  char *ztm_argv[] = {0};
  uint64_t i;

  
  printf("SAME PRIORITY...\n");

  for (i = 0; i < total_processes; i++)
    pids[i] = createProcessWithFds(zero_to_max, ztm_argv, "zero_to_max", 0, fileDescriptors);

  // Expect to see them finish at the same time

  for (i = 0; i < total_processes; i++)
    waitpid(pids[i]);

  printf("SAME PRIORITY, THEN CHANGE IT...\n");

  for (i = 0; i < total_processes; i++) {
    pids[i] = createProcessWithFds(zero_to_max, ztm_argv, "zero_to_max", 0, fileDescriptors);
    nice(pids[i], prio[i]);
    printf("  PROCESS %d NEW PRIORITY: %d\n", pids[i], prio[i]);
  }

  // Expect the priorities to take effect

  for (i = 0; i < total_processes; i++)
    waitpid(pids[i]);

  printf("SAME PRIORITY, THEN CHANGE IT WHILE BLOCKED...\n");

  for (i = 0; i < total_processes; i++) {
    pids[i] = createProcessWithFds(zero_to_max, ztm_argv, "zero_to_max", 0, fileDescriptors);
    block(pids[i]);
    nice(pids[i], prio[i]);
    printf("  PROCESS %d NEW PRIORITY: %d\n", pids[i], prio[i]);
  }

  for (i = 0; i < total_processes; i++)
    unblock(pids[i]);

  // Expect the priorities to take effect

  for (i = 0; i < total_processes; i++)
    waitpid(pids[i]);
}
