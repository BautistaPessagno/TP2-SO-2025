// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

// Gestión del ciclo de vida de procesos (PCB)
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <defs.h>
#include <memory_manager.h>
#include <linkedListADT.h>
#include <pipe_manager.h>
#include <processes.h>
#include <scheduler.h>
#include <interrupts.h>

static uint16_t next_pid = 1;
// Simple PID reuse stack. When a process is fully destroyed,
// its PID is returned here and can be reused for future processes.
#define PID_POOL_MAX (1 << 12) /* aligned with scheduler's MAX_PROCESSES */
static uint16_t reusablePids[PID_POOL_MAX];
static uint16_t reusableCount = 0;

void releasePid(uint16_t pid) {
    if (pid == 0) {
        return;
    }
    if (reusableCount < PID_POOL_MAX) {
        reusablePids[reusableCount++] = pid;
    }
}

static int count_args(char **argv) {
    if (argv == NULL) {
        return 0;
    }
    
    int count = 0;
    while (argv[count] != NULL) {
        count++;
    }
    return count;
}

static void assignFileDescriptor(Process *p, uint8_t idx, int16_t fdValue, uint8_t mode) {
    if (p == NULL || idx >= 3) {
        return;
    }
    
    p->fileDescriptors[idx] = fdValue;
    
    if (fdValue >= BUILT_IN_DESCRIPTORS) {
        pipeOpenForPid(p->pid, fdValue, mode);
    }
}

static void closeOneFD(uint16_t pid, int16_t fdValue) {
    if (fdValue >= BUILT_IN_DESCRIPTORS) {
        pipeCloseForPid(pid, fdValue);
    }
}

void initProcess(Process *p,
                 uint16_t pid, uint16_t parentPid,
                 MainFunction code, char **args, const char *name,
                 uint8_t priority, const int16_t fileDescriptors[3],
                 uint8_t unkillable) {
    
    if (p == NULL || name == NULL || priority > MAX_PRIORITY) {
        return;
    }
    
    p->pid = pid;
    p->parentPid = parentPid;
    p->priority = priority;
    p->state = READY;
    p->unkillable = unkillable;
    p->waitingForPid = 0;
    p->retValue = 0;
    
    p->stackBase = mm_malloc(STACK_SIZE);
    if (p->stackBase == NULL) {
        return;
    }
    
    size_t nameLen = strlen(name) + 1;
    p->name = mm_malloc(nameLen);
    if (p->name == NULL) {
        mm_free(p->stackBase);
        return;
    }
    memcpy(p->name, name, nameLen);
    
    if (args != NULL) {
        int argc = count_args(args);
        size_t argvTableSize = (argc + 1) * sizeof(char*);
        size_t totalStringsSize = 0;
        
        for (int i = 0; i < argc; i++) {
            totalStringsSize += strlen(args[i]) + 1;
        }
        
        size_t totalSize = argvTableSize + totalStringsSize;
        char *contiguousBlock = mm_malloc(totalSize);
        if (contiguousBlock == NULL) {
            mm_free(p->stackBase);
            mm_free(p->name);
            return;
        }
        
        p->argv = (char**)contiguousBlock;
        char *stringArea = contiguousBlock + argvTableSize;
        
        for (int i = 0; i < argc; i++) {
            size_t strLen = strlen(args[i]) + 1;
            memcpy(stringArea, args[i], strLen);
            p->argv[i] = stringArea;
            stringArea += strLen;
        }
        p->argv[argc] = NULL;
    } else {
        p->argv = NULL;
    }
    
    p->zombieChildren = createLinkedListADT();
    if (p->zombieChildren == NULL) {
        mm_free(p->stackBase);
        mm_free(p->name);
        if (p->argv != NULL) {
            mm_free(p->argv);
        }
        return;
    }
    
    if (fileDescriptors != NULL) {
        assignFileDescriptor(p, STDIN, fileDescriptors[STDIN], READ);
        assignFileDescriptor(p, STDOUT, fileDescriptors[STDOUT], WRITE);
        assignFileDescriptor(p, STDERR, fileDescriptors[STDERR], WRITE);
    } else {
        p->fileDescriptors[STDIN] = STDIN;
        p->fileDescriptors[STDOUT] = STDOUT;
        p->fileDescriptors[STDERR] = STDERR;
    }
    
    void *stack_end = (char*)p->stackBase + STACK_SIZE;
    uintptr_t stack_end_aligned = ((uintptr_t)stack_end) & ~15ULL;
    stack_end = (void*)stack_end_aligned;
    
    p->stackPos = _initialize_stack_frame(processWrapper, (void*)code, stack_end, (void*)code, (void*)p->argv);
}

uint16_t createProcess(MainFunction code,
    char **args,
    const char *name,
    uint8_t priority,
    const int16_t fileDescriptors[3],
    uint8_t unkillable) {
        Process *p = (Process*)mm_malloc(sizeof(Process));
    if (p == 0) return -1;
uint16_t parent = sched_getpid(); // 0 if none yet
uint16_t pid;
if (reusableCount > 0) {
    pid = reusablePids[--reusableCount];
} else {
    pid = next_pid++;
}
initProcess(p, pid, parent, code, args, name, priority, fileDescriptors, unkillable);
    if (sched_register_process(p) == -1) {
        mm_free(p);
        return -1;
    }
    return pid;
}


void processWrapper(void *fn, void *argv_ptr) {
    MainFunction code = (MainFunction)fn;
    char **args = (char**)argv_ptr;
    
    int argc = count_args(args);
    int ret = code(argc, args);
    
    killCurrentProcess(ret);
    // Nunca volver a ejecutar código del proceso luego de solicitar su finalización.
    // En caso de que el scheduler no haya hecho el cambio aún, ceder CPU indefinidamente.
    for(;;) {
        sched_yield();
    }
}

void closeFileDescriptors(Process *p) {
    if (p == NULL) {
        return;
    }
    
    closeOneFD(p->pid, p->fileDescriptors[STDIN]);
    closeOneFD(p->pid, p->fileDescriptors[STDOUT]);
    closeOneFD(p->pid, p->fileDescriptors[STDERR]);
}

void freeProcess(Process *p) {
    if (p == NULL) {
        return;
    }
    
    if (p->zombieChildren != NULL) {
        freeLinkedListADTDeep((LinkedListADT)p->zombieChildren);
        p->zombieChildren = NULL;
    }
    
    if (p->stackBase != NULL) {
        mm_free(p->stackBase);
    }
    
    if (p->name != NULL) {
        mm_free(p->name);
    }
    
    if (p->argv != NULL) {
        mm_free(p->argv);
    }
}

int processIsWaiting(Process *p, uint16_t pidToWait) {
    if (p == NULL) {
        return 0;
    }
    
    return (p->waitingForPid == pidToWait && p->state == BLOCKED) ? 1 : 0;
}

int getZombiesSnapshots(int idx, ProcessSnapshot arr[], Process *proc) {
    if (arr == NULL || proc == NULL || proc->zombieChildren == NULL) {
        return idx;
    }
    
    LinkedListADT list = (LinkedListADT)proc->zombieChildren;
    begin(list);
    while (hasNext(list) == 1) {
        Process *z = (Process *)next(list);
        if (z != NULL) {
            loadSnapshot(&arr[idx], z);
            idx++;
        }
    }
    return idx;
}

ProcessSnapshot * loadSnapshot(ProcessSnapshot *dst, const Process *src) {
    if (dst == NULL || src == NULL) {
        return NULL;
    }
    
    dst->pid = src->pid;
    dst->parentPid = src->parentPid;
    dst->stackBase = src->stackBase;
    dst->stackPos = src->stackPos;
    dst->priority = src->priority;
    dst->state = src->state;
    
    if (src->name != NULL) {
        size_t nameLen = strlen(src->name) + 1;
        dst->name = mm_malloc(nameLen);
        if (dst->name != NULL) {
            memcpy(dst->name, src->name, nameLen);
        }
    } else {
        dst->name = NULL;
    }
    
    dst->foreground = (src->fileDescriptors[STDIN] == STDIN) ? 1 : 0;
    
    return dst;
}

#if 0
// Micro-test no ejecutable para verificar funcionalidad básica
static void test_processes_basic_usage(void) {
    Process testProcess;
    char *testArgs[] = {"echo", "hello", "world", NULL};
    
    int16_t fds[3] = {STDIN, STDOUT, STDERR};
    initProcess(&testProcess,
                1, 0,                    // pid=1, parentPid=0 (init)
                (MainFunction)0x123456,  // función dummy
                testArgs, "echo",        // args y nombre
                1, fds,                  // priority=1, FDs estándar
                0);                      // killable
    
    // processWrapper((void*)0x123456, testProcess.argv);
    // killCurrentProcess() -> ZOMBIE -> wait() -> freeProcess()
    
    freeProcess(&testProcess);
}
#endif
