#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include <stddef.h>
#include <processes.h>

// Constantes del scheduler
#define SCHED_READY_LEVELS 5
#define SCHED_MAX_PROCS    4096
#define SCHED_DEFAULT_QUANTUM_TICKS 4

// Lista de snapshots para ps()
typedef struct ProcessSnapshotList {
    ProcessSnapshot *snapshots;
    uint16_t count;
    uint16_t capacity;
} ProcessSnapshotList;

// Inicialización y acceso
void sched_init(uint32_t quantum_ticks);
void *sched_get(void);

// Creación/registro de proceso en READY
int sched_register_process(Process *p);

// Cambios de estado explícitos
int sched_set_status(uint16_t pid, ProcessState new_state);
int sched_set_priority(uint16_t pid, uint8_t prio);

// Planificación (llamada por ISR de timer o por kernel loop)
void *schedule(void *prev_sp);

// Syscalls/helpers usados por userland o kernel
uint16_t sched_getpid(void);
void sched_yield(void);
int sched_kill_process(uint16_t pid, int32_t ret);
int sched_kill_current(int32_t ret);

// Foreground / Ctrl-C
void sched_kill_foreground(void);

// Snapshots para ps()
ProcessSnapshotList *sched_get_snapshot(void);

// Timer/Quantum
void sched_tick_isr(void);

// Idle stack configuration (used by ISR when no READY tasks)
void sched_set_idle_stack(void *idle_sp);

// Compatibility shims for legacy IPC code that expects these names
static inline uint16_t getpid(void) { return sched_getpid(); }
static inline void yield(void) { sched_yield(); }
static inline int setStatus(uint16_t pid, ProcessState st) { return sched_set_status(pid, st); }
static inline int processIsAlive(uint16_t pid) { (void)pid; return 1; }

#endif