#ifndef _SCHEDULER_H
#define _SCHEDULER_H
#include <processes.h>
#include <stdint.h>

typedef struct SchedulerCDT *SchedulerADT;

SchedulerADT createScheduler();
// New scheduler API (wrappers) used by kernel and process subsystem
void sched_init(uint8_t maxPriority);
uint16_t sched_getpid();
void sched_yield();
int32_t sched_kill_process(uint16_t pid, int32_t retValue);
int32_t sched_set_priority(uint16_t pid, uint8_t newPriority);
int8_t sched_set_status(uint16_t pid, uint8_t newStatus);
int sched_register_process(Process *p);
int32_t waitpid(uint16_t pid);
void *sched_tick_isr(void *prevStackPointer);

int32_t killProcess(uint16_t pid, int32_t retValue);
int32_t killCurrentProcess(int32_t retValue);
uint16_t getpid();
ProcessState getProcessStatus(uint16_t pid);
ProcessSnapshotList *getProcessSnapshot();
int32_t setPriority(uint16_t pid, uint8_t newPriority);
int8_t setStatus(uint16_t pid, uint8_t newStatus);
int32_t processIsAlive(uint16_t pid);
void yield();
int8_t changeFD(uint16_t pid, uint8_t position, int16_t newFd);
int16_t getCurrentProcessFileDescriptor(uint8_t fdIndex);
void killForegroundProcess();
int32_t killProcessNoZombie(uint16_t pid, int32_t retValue);
#endif
