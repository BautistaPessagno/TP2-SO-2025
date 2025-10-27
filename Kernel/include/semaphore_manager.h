#ifndef _SEMAPHORE_MANAGER_H
#define _SEMAPHORE_MANAGER_H

#include <stdint.h>
#include <stddef.h>
typedef struct SemaphoreManagerCDT *SemaphoreManagerADT;

SemaphoreManagerADT createSemaphoreManager();
int8_t semInit(uint16_t id, uint32_t initialValue);
int8_t semOpen(uint16_t id);
int8_t semClose(uint16_t id);
int8_t semPost(uint16_t id);
int8_t semWait(uint16_t id);

// low-level atomic xchg used by semaphore mutex
uint64_t _xchg(volatile uint64_t *addr, uint64_t newValue);

#endif