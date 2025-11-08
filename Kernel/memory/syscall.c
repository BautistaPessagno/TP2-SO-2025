#include <stdint.h>
#include <processes.h>
#include <scheduler.h>
#include <semaphore_manager.h>
#include <pipe_manager.h>
#include <memory_manager.h>
#include <lib.h>
#include <fonts.h>

#define COLUMN_GAP 2

static const char *stateToString(ProcessState state) {
  switch (state) {
    case READY: return "READY";
    case RUNNING: return "RUN";
    case BLOCKED: return "BLKD";
    case ZOMBIE: return "ZOMB";
    case DEAD: return "DEAD";
    default: return "UNK";
  }
}

static uint32_t decimalLength(uint64_t value) {
  uint32_t len = 1;
  while (value >= 10) {
    value /= 10;
    len++;
  }
  return len;
}

static uint32_t hexLength(uint64_t value) {
  uint32_t len = 1;
  while (value >= 16) {
    value >>= 4;
    len++;
  }
  return len;
}

static void printSpaces(uint32_t count) {
  static const char blanks[] = "                ";
  const uint32_t chunk = sizeof(blanks) - 1;
  while (count >= chunk) {
    printToFd(STDOUT, blanks, chunk);
    count -= chunk;
  }
  if (count > 0) {
    printToFd(STDOUT, blanks, count);
  }
}

static void printTextColumn(const char *text, uint32_t len, uint32_t width) {
  if (text != 0 && len > 0) {
    printToFd(STDOUT, text, len);
  }
  if (width > len) {
    printSpaces(width - len);
  }
}

static void printDecColumn(uint64_t value, uint32_t width) {
  uint32_t len = decimalLength(value);
  printDec(value);
  if (width > len) {
    printSpaces(width - len);
  }
}

static void printHexColumn(uint64_t value, uint32_t width) {
  uint32_t len = hexLength(value);
  printHex(value);
  if (width > len) {
    printSpaces(width - len);
  }
}

int64_t my_getpid() {
  return sched_getpid();
}

int64_t my_create_process(MainFunction code, char **args, const char *name, uint8_t priority, const int16_t fileDescriptors[3]) {
  return createProcess(code, args, name, priority, fileDescriptors, 0);
}

int64_t my_nice(uint64_t pid, uint64_t newPrio) {
  return sched_set_priority(pid, newPrio);
}

int64_t my_kill(uint64_t pid) {
  return sched_kill_process(pid, 0);
}

int64_t my_block(uint64_t pid) {
  return sched_set_status(pid, BLOCKED);
}

int64_t my_unblock(uint64_t pid) {
  return sched_set_status(pid, READY);
}

int64_t my_sem_init(char *sem_id, uint64_t initialValue) {
  return semInit(sem_id, initialValue);
}

int64_t my_sem_open(char *sem_id, uint64_t initialValue) {
  return semOpen(sem_id);
}

int64_t my_sem_wait(char *sem_id) {
  return semWait(sem_id);
}

int64_t my_sem_post(char *sem_id) {
  return semPost(sem_id);
}

int64_t my_sem_close(char *sem_id) {
  return semClose(sem_id);
}

int64_t my_sem_destroy(char *sem_id) {
  return semDestroy(sem_id);
}

int64_t my_yield() {
  sched_yield();
  return 0;
}

int64_t my_wait(int64_t pid) {
  return waitpid(pid);
}

void *my_malloc(uint64_t size) {
    return mm_malloc((size_t)size);
  }
  
int64_t my_free(void *ptr) {
    mm_free(ptr);
    return 0;
}

int64_t my_mm_state(MMState *state) {
  if (state == 0) return -1;
  MMState s = mm_state();
  *state = s;
  return 0;
}

int64_t my_print_ps(void) {
  ProcessSnapshotList *list = getProcessSnapshot();
  if (list == 0 || list->snapshotList == 0) return -1;

  const char *pidHeader = "PID";
  const char *ppidHeader = "PPID";
  const char *prioHeader = "PRIO";
  const char *stateHeader = "STATE";
  const char *fgHeader = "FG";
  const char *stackBaseHeader = "STACK_BASE";
  const char *stackPtrHeader = "STACK_PTR";
  const char *nameHeader = "NAME";
  const char *idleName = "<idle>";

  uint32_t pidLen = (uint32_t)strlen(pidHeader);
  uint32_t ppidLen = (uint32_t)strlen(ppidHeader);
  uint32_t prioLen = (uint32_t)strlen(prioHeader);
  uint32_t stateLen = (uint32_t)strlen(stateHeader);
  uint32_t fgLen = (uint32_t)strlen(fgHeader);
  uint32_t stackBaseLen = (uint32_t)strlen(stackBaseHeader);
  uint32_t stackPtrLen = (uint32_t)strlen(stackPtrHeader);
  uint32_t nameLen = (uint32_t)strlen(nameHeader);

  for (uint16_t i = 0; i < list->length; i++) {
    ProcessSnapshot *ps = &list->snapshotList[i];

    uint32_t len = decimalLength(ps->pid);
    if (len > pidLen) pidLen = len;

    len = decimalLength(ps->parentPid);
    if (len > ppidLen) ppidLen = len;

    len = decimalLength(ps->priority);
    if (len > prioLen) prioLen = len;

    const char *stateStr = stateToString(ps->state);
    len = (uint32_t)strlen(stateStr);
    if (len > stateLen) stateLen = len;

    len = (uint32_t)strlen(ps->foreground ? "FG" : "BG");
    if (len > fgLen) fgLen = len;

    len = hexLength((uint64_t)ps->stackBase);
    if (len > stackBaseLen) stackBaseLen = len;

    len = hexLength((uint64_t)ps->stackPos);
    if (len > stackPtrLen) stackPtrLen = len;

    const char *nameStr = ps->name != 0 ? ps->name : idleName;
    len = (uint32_t)strlen(nameStr);
    if (len > nameLen) nameLen = len;
  }

  const uint32_t pidWidth = pidLen + COLUMN_GAP;
  const uint32_t ppidWidth = ppidLen + COLUMN_GAP;
  const uint32_t prioWidth = prioLen + COLUMN_GAP;
  const uint32_t stateWidth = stateLen + COLUMN_GAP;
  const uint32_t fgWidth = fgLen + COLUMN_GAP;
  const uint32_t stackBaseWidth = stackBaseLen + COLUMN_GAP;
  const uint32_t stackPtrWidth = stackPtrLen + COLUMN_GAP;

  printTextColumn(pidHeader, (uint32_t)strlen(pidHeader), pidWidth);
  printTextColumn(ppidHeader, (uint32_t)strlen(ppidHeader), ppidWidth);
  printTextColumn(prioHeader, (uint32_t)strlen(prioHeader), prioWidth);
  printTextColumn(stateHeader, (uint32_t)strlen(stateHeader), stateWidth);
  printTextColumn(fgHeader, (uint32_t)strlen(fgHeader), fgWidth);
  printTextColumn(stackBaseHeader, (uint32_t)strlen(stackBaseHeader), stackBaseWidth);
  printTextColumn(stackPtrHeader, (uint32_t)strlen(stackPtrHeader), stackPtrWidth);
  printTextColumn(nameHeader, (uint32_t)strlen(nameHeader), 0);
  printToFd(STDOUT, "\n", 1);

  for (uint16_t i = 0; i < list->length; i++) {
    ProcessSnapshot *ps = &list->snapshotList[i];
    printDecColumn(ps->pid, pidWidth);
    printDecColumn(ps->parentPid, ppidWidth);
    printDecColumn(ps->priority, prioWidth);

    const char *stateStr = stateToString(ps->state);
    printTextColumn(stateStr, (uint32_t)strlen(stateStr), stateWidth);

    const char *fgStr = ps->foreground ? "FG" : "BG";
    printTextColumn(fgStr, (uint32_t)strlen(fgStr), fgWidth);

    printHexColumn((uint64_t)ps->stackBase, stackBaseWidth);
    printHexColumn((uint64_t)ps->stackPos, stackPtrWidth);

    const char *nameStr = ps->name != 0 ? ps->name : idleName;
    printTextColumn(nameStr, (uint32_t)strlen(nameStr), 0);
    printToFd(STDOUT, "\n", 1);
  }

  return 0;
}
