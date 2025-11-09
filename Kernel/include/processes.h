#ifndef PROCESSES_H
#define PROCESSES_H

#include <stdint.h>
#include <stddef.h>

#define STACK_SIZE (1 << 13)
#define MAX_PRIORITY 4
#define STDIN  0
#define STDOUT 1
#define STDERR 2
#define DEV_NULL (-1)
#define BUILT_IN_DESCRIPTORS 3

typedef int (*MainFunction)(int, char**);

typedef enum {
    READY = 0,
    RUNNING = 1,
    BLOCKED = 2,
    ZOMBIE = 3,
    DEAD = 4
} ProcessState;

typedef struct Process {
    uint16_t pid;
    uint16_t parentPid;
    uint8_t priority;
    ProcessState state;
    void *stackBase;
    void *stackPos;
    char *name;
    char **argv;
    void *zombieChildren;  // LinkedListADT
    uint16_t waitingForPid;
    int32_t retValue;
    uint8_t unkillable;
    int16_t fileDescriptors[3];
} Process;

typedef struct ProcessSnapshot {
    uint16_t pid;
    uint16_t parentPid;
    void *stackBase;
    void *stackPos;
    uint8_t priority;
    ProcessState state;
    char *name;
    uint8_t foreground;
} ProcessSnapshot;

typedef struct ProcessSnapshotList {
	uint16_t length;
	ProcessSnapshot *snapshotList;
} ProcessSnapshotList;

extern void * _initialize_stack_frame(void (*entry)(void*, void*),
                                      void *func, void *stack_end, void *arg1, void *arg2);

// Minimal kernel-level helpers referenced by processes.c
int killCurrentProcess(int32_t retValue);

// Inicializa un proceso con todos sus componentes
void initProcess(Process *p,
                 uint16_t pid, uint16_t parentPid,
                 MainFunction code, char **args, const char *name,
                 uint8_t priority, const int16_t fileDescriptors[3],
                 uint8_t unkillable);

// Wrapper de entrada para todos los procesos
void processWrapper(void *fn, void *argv_ptr);

// Cierra todos los file descriptors de un proceso
void closeFileDescriptors(Process *p);

// Libera completamente un proceso y todos sus recursos
void freeProcess(Process *p);

// Verifica si un proceso está esperando a otro específico
int processIsWaiting(Process *p, uint16_t pidToWait);

// Obtiene snapshots de procesos zombies
int getZombiesSnapshots(int idx, ProcessSnapshot arr[], Process *proc);

// Carga un snapshot de un proceso
ProcessSnapshot * loadSnapshot(ProcessSnapshot *dst, const Process *src);

uint16_t createProcess(MainFunction code,
    char **args,
    const char *name,
    uint8_t priority,
    const int16_t fileDescriptors[3],
    uint8_t unkillable);

#endif
