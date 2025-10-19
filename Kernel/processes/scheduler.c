// Scheduler con política RR con prioridades
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <defs.h>
#include <memory_manager.h>
#include <linkedListADT.h>
#include <processes.h>
#include <scheduler.h>

// Estructura interna del scheduler
typedef struct Scheduler {
    Node *index[SCHED_MAX_PROCS];
    LinkedListADT ready[SCHED_READY_LEVELS];
    LinkedListADT blocked;
    uint16_t currentPid;
    uint32_t quantumTicks;
    uint32_t remainingTicks;
} Scheduler;

// Variables globales
static Scheduler *scheduler = NULL;
static uint8_t kill_foreground_flag = 0;
static void *idle_stack_pointer = NULL;

// Helpers para LinkedListADT
static void push_back(LinkedListADT list, void *data) {
    if (list != NULL && data != NULL) {
        // TODO: implementar push_back en LinkedListADT
        // Por ahora asumimos que existe
    }
}

static void *pop_front(LinkedListADT list) {
    if (list != NULL) {
        // TODO: implementar pop_front en LinkedListADT
        // Por ahora asumimos que existe
        return NULL;
    }
    return NULL;
}

static void remove_node(LinkedListADT list, Node *node) {
    if (list != NULL && node != NULL) {
        // TODO: implementar remove_node en LinkedListADT
        // Por ahora asumimos que existe
    }
}

// Inicialización del scheduler
void sched_init(uint32_t quantum_ticks) {
    if (scheduler != NULL) {
        return; // Ya inicializado
    }
    
    scheduler = mm_malloc(sizeof(Scheduler));
    if (scheduler == NULL) {
        return; // TODO: error handling
    }
    
    // Inicializar colas READY por prioridad
    for (int i = 0; i < SCHED_READY_LEVELS; i++) {
        scheduler->ready[i] = createLinkedListADT();
        if (scheduler->ready[i] == NULL) {
            // TODO: error handling
            return;
        }
    }
    
    // Inicializar cola BLOCKED
    scheduler->blocked = createLinkedListADT();
    if (scheduler->blocked == NULL) {
        // TODO: error handling
        return;
    }
    
    // Inicializar índice
    for (int i = 0; i < SCHED_MAX_PROCS; i++) {
        scheduler->index[i] = NULL;
    }
    
    scheduler->currentPid = 0;
    scheduler->quantumTicks = (quantum_ticks == 0) ? SCHED_DEFAULT_QUANTUM_TICKS : quantum_ticks;
    scheduler->remainingTicks = 0;
    
    kill_foreground_flag = 0;
}

// Obtener instancia del scheduler
void *sched_get(void) {
    return scheduler;
}

// Registrar proceso en el scheduler
int sched_register_process(Process *p) {
    if (scheduler == NULL || p == NULL || p->pid >= SCHED_MAX_PROCS) {
        return -1;
    }
    
    if (scheduler->index[p->pid] != NULL) {
        return -1; // PID duplicado
    }
    
    // Crear nodo para el proceso
    Node *node = mm_malloc(sizeof(Node));
    if (node == NULL) {
        return -1;
    }
    
    node->data = p;
    node->next = NULL;
    
    // Asignar al índice
    scheduler->index[p->pid] = node;
    
    // Encolar en cola READY de su prioridad
    if (p->priority < SCHED_READY_LEVELS) {
        push_back(scheduler->ready[p->priority], node);
        p->state = READY;
    } else {
        // Prioridad inválida, usar 0
        push_back(scheduler->ready[0], node);
        p->state = READY;
    }
    
    return 0;
}

// Cambiar estado de un proceso
int sched_set_status(uint16_t pid, ProcessState new_state) {
    if (scheduler == NULL || pid >= SCHED_MAX_PROCS || scheduler->index[pid] == NULL) {
        return -1;
    }
    
    Node *node = scheduler->index[pid];
    Process *p = (Process*)node->data;
    
    if (p == NULL) {
        return -1;
    }
    
    switch (new_state) {
        case READY:
            if (p->state == BLOCKED) {
                // Sacar de BLOCKED y encolar en READY
                remove_node(scheduler->blocked, node);
                push_back(scheduler->ready[p->priority], node);
                p->state = READY;
            }
            break;
            
        case BLOCKED:
            if (p->state == READY) {
                // Sacar de READY y encolar en BLOCKED
                remove_node(scheduler->ready[p->priority], node);
                push_back(scheduler->blocked, node);
                p->state = BLOCKED;
            }
            break;
            
        default:
            return -1; // Estados no permitidos
    }
    
    return 0;
}

// Cambiar prioridad de un proceso
int sched_set_priority(uint16_t pid, uint8_t prio) {
    if (scheduler == NULL || pid >= SCHED_MAX_PROCS || scheduler->index[pid] == NULL) {
        return -1;
    }
    
    if (prio >= SCHED_READY_LEVELS) {
        return -1;
    }
    
    Node *node = scheduler->index[pid];
    Process *p = (Process*)node->data;
    
    if (p == NULL) {
        return -1;
    }
    
    // Si está READY, reubicar en nueva cola
    if (p->state == READY) {
        remove_node(scheduler->ready[p->priority], node);
        p->priority = prio;
        push_back(scheduler->ready[prio], node);
    } else {
        // Solo actualizar prioridad, se aplicará cuando pase a READY
        p->priority = prio;
    }
    
    return 0;
}

// Planificación principal
void *schedule(void *prev_sp) {
    if (scheduler == NULL) {
        return idle_stack_pointer;
    }
    
    // Guardar SP del proceso saliente si estaba RUNNING
    if (scheduler->currentPid != 0) {
        Node *currentNode = scheduler->index[scheduler->currentPid];
        if (currentNode != NULL) {
            Process *currentProc = (Process*)currentNode->data;
            if (currentProc != NULL && currentProc->state == RUNNING) {
                currentProc->stackPos = prev_sp;
                
                // Si sigue vivo y no bloqueado, reencolar
                if (currentProc->state == RUNNING) {
                    currentProc->state = READY;
                    push_back(scheduler->ready[currentProc->priority], currentNode);
                }
            }
        }
    }
    
    // Verificar kill foreground
    if (kill_foreground_flag) {
        kill_foreground_flag = 0;
        // TODO: implementar lógica de kill foreground
    }
    
    // Buscar siguiente proceso (de mayor a menor prioridad)
    Node *nextNode = NULL;
    for (int prio = SCHED_READY_LEVELS - 1; prio >= 0; prio--) {
        nextNode = pop_front(scheduler->ready[prio]);
        if (nextNode != NULL) {
            break;
        }
    }
    
    if (nextNode == NULL) {
        // No hay procesos READY, usar idle
        scheduler->currentPid = 0;
        return idle_stack_pointer;
    }
    
    Process *nextProc = (Process*)nextNode->data;
    if (nextProc == NULL) {
        scheduler->currentPid = 0;
        return idle_stack_pointer;
    }
    
    // Configurar proceso entrante
    scheduler->currentPid = nextProc->pid;
    nextProc->state = RUNNING;
    scheduler->remainingTicks = scheduler->quantumTicks;
    
    return nextProc->stackPos;
}

// Obtener PID actual
uint16_t sched_getpid(void) {
    if (scheduler == NULL) {
        return 0;
    }
    return scheduler->currentPid;
}

// Forzar fin de quantum
void sched_yield(void) {
    if (scheduler == NULL) {
        return;
    }
    scheduler->remainingTicks = 0;
}

// Matar proceso específico
int sched_kill_process(uint16_t pid, int32_t ret) {
    if (scheduler == NULL || pid >= SCHED_MAX_PROCS || scheduler->index[pid] == NULL) {
        return -1;
    }
    
    Node *node = scheduler->index[pid];
    Process *p = (Process*)node->data;
    
    if (p == NULL) {
        return -1;
    }
    
    // Sacar de colas
    if (p->state == READY) {
        remove_node(scheduler->ready[p->priority], node);
    } else if (p->state == BLOCKED) {
        remove_node(scheduler->blocked, node);
    }
    
    // Marcar como ZOMBIE
    p->state = ZOMBIE;
    p->retValue = ret;
    
    // Cerrar file descriptors
    closeFileDescriptors(p);
    
    // Si era el proceso actual, forzar cambio
    if (pid == scheduler->currentPid) {
        sched_yield();
    }
    
    return 0;
}

// Matar proceso actual
int sched_kill_current(int32_t ret) {
    if (scheduler == NULL || scheduler->currentPid == 0) {
        return -1;
    }
    return sched_kill_process(scheduler->currentPid, ret);
}

// Matar proceso en foreground
void sched_kill_foreground(void) {
    kill_foreground_flag = 1;
}

// Obtener snapshots para ps()
ProcessSnapshotList *sched_get_snapshot(void) {
    if (scheduler == NULL) {
        return NULL;
    }
    
    // TODO: implementar recolección de snapshots
    // Por ahora retornamos NULL
    return NULL;
}

// ISR del timer
void sched_tick_isr(void) {
    if (scheduler == NULL) {
        return;
    }
    
    if (scheduler->remainingTicks > 0) {
        scheduler->remainingTicks--;
    }
    
    if (scheduler->remainingTicks == 0) {
        // Forzar context switch
        // TODO: implementar llamada a schedule() desde ISR
    }
}

#if 0
// Tests no ejecutables para verificar funcionalidad
static void test_scheduler_basic(void) {
    // Crear 3 procesos con prioridades (2,2,4)
    Process proc1, proc2, proc3;
    
    // Inicializar procesos
    initProcess(&proc1, 1, 0, (MainFunction)0x1000, NULL, "proc1", 2, NULL, 0);
    initProcess(&proc2, 2, 0, (MainFunction)0x2000, NULL, "proc2", 2, NULL, 0);
    initProcess(&proc3, 3, 0, (MainFunction)0x3000, NULL, "proc3", 4, NULL, 0);
    
    // Registrar en scheduler
    sched_register_process(&proc1);
    sched_register_process(&proc2);
    sched_register_process(&proc3);
    
    // Verificar que proc3 (prio 4) corre más frecuentemente
    // cuando las colas 4 y 2 compiten
    
    // Test block/unblock
    sched_set_status(2, PROC_BLOCKED); // Bloquear proc2
    // El siguiente pick no debe considerar proc2 hasta que esté READY
    
    // Test yield
    sched_yield(); // Fuerza rotación RR dentro de la misma cola
    
    // Test kill_current
    sched_kill_current(42); // Mata proceso actual y replanifica
    
    // Limpiar
    freeProcess(&proc1);
    freeProcess(&proc2);
    freeProcess(&proc3);
}
#endif
