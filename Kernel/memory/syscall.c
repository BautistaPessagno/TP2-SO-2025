#include <stdint.h>
#include <processes.h>
#include <scheduler.h>
#include <semaphore_manager.h>
#include <pipe_manager.h>

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
