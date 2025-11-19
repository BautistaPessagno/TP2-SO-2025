// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <defs.h>
#include <lib.h>
#include <linkedListADT.h>
#include <memory_manager.h>
#include <processes.h>
#include <scheduler.h>
#include <semaphore_manager.h>
#include <stdint.h>
#include <stdlib.h>
#include <lib.h>

#define MAX_SEMAPHORES (1 << 12)

typedef struct Semaphore {
	uint32_t value;
	uint64_t mutex;				  // 0 libre, 1 ocupado (64-bit, matches _xchg)
	LinkedListADT semaphoreQueue; // LinkedListADT de pids
	LinkedListADT mutexQueue;	  // LinkedListADT de pids
} Semaphore;

static Semaphore *createSemaphore(uint32_t initialValue);
static void freeSemaphore(Semaphore *sem);
static void acquireMutex(Semaphore *sem);
static void resumeFirstAvailableProcess(LinkedListADT queue);
static void releaseMutex(Semaphore *sem);
static int up(Semaphore *sem);
static int down(Semaphore *sem);

typedef struct SemaphoreManagerCDT {
	Semaphore *semaphores[MAX_SEMAPHORES];
} SemaphoreManagerCDT;

SemaphoreManagerADT createSemaphoreManager() {
	SemaphoreManagerADT semManager = (SemaphoreManagerADT) SEMAPHORE_MANAGER_ADDRESS;
	for (int i = 0; i < MAX_SEMAPHORES; i++)
		semManager->semaphores[i] = NULL;
	return semManager;
}

static SemaphoreManagerADT getSemaphoreManager() {
	return (SemaphoreManagerADT) SEMAPHORE_MANAGER_ADDRESS;
}

int8_t semInit(uint16_t id, uint32_t initialValue) {
	SemaphoreManagerADT semManager = getSemaphoreManager();
	if (semManager->semaphores[id] != NULL)
		return -1;
	semManager->semaphores[id] = createSemaphore(initialValue);
	return 0;
}

int8_t semOpen(uint16_t id) {
	SemaphoreManagerADT semManager = getSemaphoreManager();
	return semManager->semaphores[id] == NULL ? -1 : 0;
}

int8_t semClose(uint16_t id) {
	SemaphoreManagerADT semManager = getSemaphoreManager();
	if (semManager->semaphores[id] == NULL)
		return -1;

	freeSemaphore(semManager->semaphores[id]);
	semManager->semaphores[id] = NULL;
	return 0;
}

int8_t semPost(uint16_t id) {
	SemaphoreManagerADT semManager = getSemaphoreManager();
	if (semManager->semaphores[id] == NULL)
		return -1;
	return up(semManager->semaphores[id]);
}

int8_t semWait(uint16_t id) {
	SemaphoreManagerADT semManager = getSemaphoreManager();
	if (semManager->semaphores[id] == NULL)
		return -1;
	return down(semManager->semaphores[id]);
}

static Semaphore *createSemaphore(uint32_t initialValue) {
    Semaphore *sem = (Semaphore *) mm_malloc(sizeof(Semaphore));
	sem->value = initialValue;
	sem->mutex = 0;
	sem->semaphoreQueue = createLinkedListADT();
	sem->mutexQueue = createLinkedListADT();
	return sem;
}

static void freeSemaphore(Semaphore *sem) {
	freeLinkedListADTDeep(sem->semaphoreQueue);
	freeLinkedListADTDeep(sem->mutexQueue);
    mm_free(sem);
}

static void acquireMutex(Semaphore *sem) {
    while (_xchg((volatile uint64_t *)&(sem->mutex), 1)) {
		uint16_t pid = getpid();
		appendElement(sem->mutexQueue, (void *) ((uint64_t) pid));
		setStatus(pid, BLOCKED);
		yield();
	}
}

static void resumeFirstAvailableProcess(LinkedListADT queue) {
    Node *current;
	while ((current = getFirst(queue)) != NULL) {
		removeNode(queue, current);
		uint16_t pid = (uint16_t) ((uint64_t) current->data);
        mm_free(current);
		if (processIsAlive(pid)) {
			setStatus(pid, READY);
			break;
		}
	}
}

static void releaseMutex(Semaphore *sem) {
	resumeFirstAvailableProcess(sem->mutexQueue);
	sem->mutex = 0;
}

static int up(Semaphore *sem) {
	acquireMutex(sem);
	sem->value++;
	// Si hay procesos esperando por el semáforo, reanudar uno
	resumeFirstAvailableProcess(sem->semaphoreQueue);
	releaseMutex(sem);
	yield();
	return 0;
}

static int down(Semaphore *sem) {
	acquireMutex(sem);
	while (sem->value == 0) {
		uint16_t pid = getpid();
		appendElement(sem->semaphoreQueue, (void *) ((uint64_t) pid));
		setStatus(pid, BLOCKED);
		releaseMutex(sem);
		yield();

		acquireMutex(sem);
	}
	sem->value--;
	releaseMutex(sem);

	return 0;
}

int8_t semDestroy(uint16_t id) {
	SemaphoreManagerADT semManager = getSemaphoreManager();
	if (semManager->semaphores[id] == NULL)
		return -1;
	Semaphore *sem = semManager->semaphores[id];
	if (isEmpty(sem->semaphoreQueue) == 0 || isEmpty(sem->mutexQueue) == 0) {
		return -1; // still in use
	}
	freeSemaphore(sem);
	semManager->semaphores[id] = NULL;
	return 0;
}
