#include <stdint.h>
#include <memory_manager.h>

int64_t my_getpid();
int64_t my_create_process(MainFunction code, char **args, const char *name, uint8_t priority, const int16_t fileDescriptors[3]);
int64_t my_nice(uint64_t pid, uint64_t newPrio);
int64_t my_kill(uint64_t pid);
int64_t my_block(uint64_t pid);
int64_t my_unblock(uint64_t pid);
int64_t my_sem_init(uint16_t sem_id, uint64_t initialValue);
int64_t my_sem_open(uint16_t sem_id, uint64_t initialValue);
int64_t my_sem_wait(uint16_t sem_id);
int64_t my_sem_post(uint16_t sem_id);
int64_t my_sem_close(uint16_t sem_id);
int64_t my_sem_destroy(uint16_t sem_id);
int64_t my_yield();
int64_t my_wait(int64_t pid);
// Extra utilities for userland
int64_t my_mm_state(MMState *state);
int64_t my_print_ps(void);
void *my_malloc(uint64_t size);
int64_t my_free(void *ptr);
int64_t my_pipe_get(void);