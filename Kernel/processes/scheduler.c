// Scheduler con política RR con prioridades
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <defs.h>
#include <memory_manager.h>
#include <linkedListADT.h>
#include <processes.h>
#include <scheduler.h>
#include <lib.h>  // Para strlen

// Estructura interna del scheduler
typedef struct Scheduler {
    Node *index[SCHED_MAX_PROCS];
    LinkedListADT ready[SCHED_READY_LEVELS];
    LinkedListADT blocked;
    uint16_t currentPid;
    uint32_t quantumTicks;
    uint32_t remainingTicks;
    uint16_t nextUnusedPid;
    uint16_t qtyProcesses;
} Scheduler;

#define MAX_PROCESSES 1 << 12
#define IDLE_PID 0
// Variables globales
static Scheduler *scheduler = NULL;
static uint8_t kill_foreground_flag = 0;
static void *idle_stack_pointer = NULL;
void sched_set_idle_stack(void *idle_sp) {
    idle_stack_pointer = idle_sp;
}
// Kernel-facing minimal helper to allow process exit from wrapper
int killCurrentProcess(int32_t ret) {
    return sched_kill_current(ret);
}

// Helpers para LinkedListADT
static void push_back(LinkedListADT list, Node *node) {
    if (list != NULL && node != NULL) {
        appendNode(list, node);
    }
}

static Node *pop_front(LinkedListADT list) {
    if (list != NULL) {
        return popFront(list);
    }
    return NULL;
}

static void remove_node(LinkedListADT list, Node *node) {
    if (list != NULL && node != NULL) {
        (void)removeNode(list, node);
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
    
    // Crear el Node una sola vez y encolarlo en READY
    int prio = (p->priority < SCHED_READY_LEVELS) ? p->priority : 0;
    Node *node = appendElement(scheduler->ready[prio], (void *)p);
    if (node == NULL) {
        return -1;
    }
    // Guardar el Node* para moverlo entre listas en O(1)
    scheduler->index[p->pid] = node;
    p->state = READY;
    
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
                // Sacar de BLOCKED
                remove_node(scheduler->blocked, node);
                // Política: boost al desbloquear para responsividad
                p->priority = MAX_PRIORITY;
                // Encolar al frente de la cola de mayor prioridad
                prependNode(scheduler->ready[p->priority], node);
                p->state = READY;
                // Forzar replanificación para correr cuanto antes
                scheduler->remainingTicks = 0;
            }
            break;
            
        case BLOCKED:
            if (p->state == READY) {
                // Sacar de READY y encolar en BLOCKED
                remove_node(scheduler->ready[p->priority], node);
                push_back(scheduler->blocked, node);
                p->state = BLOCKED;
            } else if (p->state == RUNNING) {
                // Bloquear proceso en ejecución (syscalls bloqueantes)
                p->state = BLOCKED;
                push_back(scheduler->blocked, node);
                // Forzar cambio de contexto
                sched_yield();
            }
            break;
            
        default:
            return -1;
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

// ------------------------- MAIN MAIN  REVISAR ACA 
void *schedule(void *prev_sp) {
    if (scheduler == NULL) {
        return prev_sp;
    }
    
    // Guardar SP del proceso saliente si estaba RUNNING
    if (scheduler->currentPid != 0) {
        Node *currentNode = scheduler->index[scheduler->currentPid];
        if (currentNode != NULL) {
            Process *currentProc = (Process*)currentNode->data;
            if (currentProc != NULL) {
                // Siempre guardamos el SP actual del proceso saliente
                currentProc->stackPos = prev_sp;
                if (currentProc->state == RUNNING) {
                    // Preemption: pasa a READY y se reencola (baja una prioridad)
                    currentProc->state = READY;
                    uint8_t newPrio = (currentProc->priority > 0) ? (currentProc->priority - 1) : currentProc->priority;
                    currentProc->priority = newPrio;
                    // El nodo no pertenece a ninguna lista en RUNNING, solo lo encolamos
                    push_back(scheduler->ready[currentProc->priority], currentNode);
                }
                // Si estaba BLOCKED, ya fue encolado en blocked por quien lo bloqueó
                // y solo necesitábamos preservar su stackPos aquí.
            }
        }
    }
    
    // Verificar ctrl C (kill foreground)
    if (kill_foreground_flag) {
        kill_foreground_flag = 0;
        Node *curNode = scheduler->index[scheduler->currentPid];
        if (curNode != NULL) {
            Process *cur = (Process*)curNode->data;
            if (cur != NULL && cur->fileDescriptors[STDIN] == STDIN) {
                sched_kill_current(-1);
                sched_yield();
            }
        }
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
        // No hay procesos READY en las colas. Intentar encontrar alguno en el índice (ej: idle)
        for (int pid = 0; pid < SCHED_MAX_PROCS; pid++) {
            Node *n = scheduler->index[pid];
            if (n != NULL) {
                Process *p = (Process*)n->data;
                if (p != NULL && p->state == READY) {
                    // Sacar de su cola READY actual y seleccionarlo
                    remove_node(scheduler->ready[p->priority], n);
                    nextNode = n;
                    break;
                }
            }
        }
        if (nextNode == NULL) {
            // Mantener el contexto actual si no hay opción válida
            return prev_sp;
        }
    }
    
    Process *nextProc = (Process*)nextNode->data;
    if (nextProc == NULL) {
        scheduler->currentPid = 0;
        return prev_sp;
    }
    
    // Configurar proceso entrante
    scheduler->currentPid = nextProc->pid;
    nextProc->state = RUNNING;
    // Set quantum length based on priority like the reference (higher priority -> longer quantum)
    // MAX_PRIORITY is defined in processes.h
    scheduler->remainingTicks = (MAX_PRIORITY > nextProc->priority) ? (MAX_PRIORITY - nextProc->priority) : 1;
    
    // Si no hay stackPos válido, conservar contexto actual
    return nextProc->stackPos != NULL ? nextProc->stackPos : prev_sp;
}

// Obtener PID actual
uint16_t sched_getpid(void) {
    if (scheduler == NULL) {
        return 0;
    }
    return scheduler->currentPid;
}

// Obtener proceso actual
Process *sched_get_current_process(void) {
    if (scheduler == NULL || scheduler->currentPid == 0) {
        return NULL;
    }
    
    Node *node = scheduler->index[scheduler->currentPid];
    if (node == NULL) {
        return NULL;
    }
    
    return (Process*)node->data;
}

// Obtener proceso por PID
Process *sched_get_process_by_pid(uint16_t pid) {
    if (scheduler == NULL || pid >= SCHED_MAX_PROCS || scheduler->index[pid] == NULL) {
        return NULL;
    }
    
    Node *node = scheduler->index[pid];
    return (Process*)node->data;
}

// Forzar fin de quantum
void sched_yield(void) {
    if (scheduler == NULL) {
        return;
    }
    scheduler->remainingTicks = 0;
}

static void destroyZombie(Scheduler *scheduler, Process *zombie) {
	Node *zombieNode = scheduler->index[zombie->pid];
	scheduler->qtyProcesses--;
	scheduler->index[zombie->pid] = NULL;
	// Liberar recursos del proceso y el contenedor Node asociado al índice
	freeProcess(zombie);
	if (zombieNode != NULL) {
		mm_free(zombieNode);
	}
}

int32_t waitpid(uint16_t pid) {
	Scheduler *scheduler = (Scheduler *)sched_get();
	Node *zombieNode = scheduler->index[pid];
	if (zombieNode == NULL)
		return -1;
	Process *zombieProcess = (Process *) zombieNode->data;
	if (zombieProcess->parentPid != scheduler->currentPid)
		return -1;

	Process *parent = (Process *) scheduler->index[scheduler->currentPid]->data;
	parent->waitingForPid = pid;
	if (zombieProcess->state != ZOMBIE) {
		sched_set_status(parent->pid, BLOCKED);
		sched_yield();
	}
	// Remover el hijo zombie de la lista de zombies del padre (por data)
	LinkedListADT zlist = (LinkedListADT)parent->zombieChildren;
	if (zlist != NULL) {
		Node *iter = getFirst(zlist);
		while (iter != NULL) {
			if (iter->data == (void *)zombieProcess) {
				removeNode(zlist, iter);
				break;
			}
			iter = iter->next;
		}
	}
	destroyZombie(scheduler, zombieProcess);
	return zombieProcess->retValue;
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
    if (p->state == ZOMBIE || p->unkillable) {
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
    
    // Agregar a la lista de zombies del padre
    if (p->parentPid != 0 && p->parentPid < SCHED_MAX_PROCS) {
        Node *parentNode = scheduler->index[p->parentPid];
        if (parentNode != NULL) {
            Process *parent = (Process*)parentNode->data;
            if (parent != NULL && parent->zombieChildren != NULL) {
                LinkedListADT zombieList = (LinkedListADT)parent->zombieChildren;
                (void)appendElement(zombieList, (void*)p); // la lista guarda Process*
            }
        }
    }
    
    // Si era el proceso actual, forzar cambio
    if (pid == scheduler->currentPid) {
        sched_yield();
        // Disparar inmediatamente el tick para no volver al contexto destruido
        forceTimerTick();
    }
    
    // Despertar al padre si estaba esperando
    if (p->parentPid != 0 && p->parentPid < SCHED_MAX_PROCS) {
        Node *parentNode = scheduler->index[p->parentPid];
        if (parentNode != NULL) {
            Process *parent = (Process*)parentNode->data;
            if (parent != NULL && parent->waitingForPid == pid) {
                // Aseguramos que el padre pase a READY independientemente del estado actual
                // Si estaba bloqueado, remover de la cola de bloqueados
                if (parent->state == BLOCKED) {
                    remove_node(scheduler->blocked, parentNode);
                }
                parent->state = READY;
                parent->waitingForPid = 0;
                // Boost de prioridad y encolado al frente
                parent->priority = MAX_PRIORITY;
                prependNode(scheduler->ready[parent->priority], parentNode);
                // Forzar replanificación inmediata
                scheduler->remainingTicks = 0;
            }
        }
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
    kill_foreground_flag = 1;  // esta flag podria estar en el adt del scheduler, por ahora la tenemos global oooo
}

// Obtener snapshots para ps()
ProcessSnapshotList *sched_get_snapshot(void) {
    if (scheduler == NULL) {
        return NULL;
    }
    
    ProcessSnapshotList *snapshotList = mm_malloc(sizeof(ProcessSnapshotList));
    if (snapshotList == NULL) {
        return NULL;
    }
    snapshotList->snapshots = mm_malloc(sizeof(ProcessSnapshot) * SCHED_MAX_PROCS);
    if (snapshotList->snapshots == NULL) {
        return NULL;
    }
    snapshotList->count = 0;
    snapshotList->capacity = SCHED_MAX_PROCS;
    
    for (int i = 0; i < SCHED_MAX_PROCS; i++) {
        if (scheduler->index[i] != NULL) {
            Process *p = (Process*)scheduler->index[i]->data;
            if (p != NULL) {
                snapshotList->snapshots[i] = *loadSnapshot(&snapshotList->snapshots[i], p);
                snapshotList->count++;
            }
        }
    }   
    return snapshotList;
}

// ISR del timer
void *sched_tick_isr(void *current_sp) {
    if (scheduler == NULL) {
        return current_sp;
    }
    
    if (scheduler->remainingTicks > 0) {
        scheduler->remainingTicks--;
        return current_sp;
    }
    
    // Fin de quantum: delegar en schedule y retornar el nuevo SP
    return schedule(current_sp);
}
