#include <stdint.h>
#include <string.h>
#include <memory_manager.h>
#include <processes.h>
#include <scheduler.h>
#include <linkedListADT.h>

int64_t my_getpid() {
  return (int64_t)sched_getpid();
}

// Función auxiliar para copiar argumentos desde userland
static char **copy_argv_from_userland(char **user_argv, uint64_t argc) {
    if (user_argv == NULL) {
        return NULL;
    }
    
    // Si argc == 0, retornar array con solo NULL
    if (argc == 0) {
        char **result = (char**)mm_malloc(sizeof(char*));
        if (result == NULL) {
            return NULL;
        }
        result[0] = NULL;
        return result;
    }
    
    // Calcular tamaño total necesario
    size_t argvTableSize = (argc + 1) * sizeof(char*);
    size_t totalStringsSize = 0;
    
    for (uint64_t i = 0; i < argc; i++) {
        if (user_argv[i] == NULL) {
            return NULL;  // Argumento inválido
        }
        totalStringsSize += strlen(user_argv[i]) + 1;
    }
    
    size_t totalSize = argvTableSize + totalStringsSize;
    char *contiguousBlock = mm_malloc(totalSize);
    if (contiguousBlock == NULL) {
        return NULL;
    }
    
    char **kernel_argv = (char**)contiguousBlock;
    char *stringArea = contiguousBlock + argvTableSize;
    
    // Copiar strings
    for (uint64_t i = 0; i < argc; i++) {
        size_t strLen = strlen(user_argv[i]) + 1;
        memcpy(stringArea, user_argv[i], strLen);
        kernel_argv[i] = stringArea;
        stringArea += strLen;
    }
    kernel_argv[argc] = NULL;
    
    return kernel_argv;
}

int64_t my_create_process(char *name, uint64_t argc, char *argv[]) {
    // Validar parámetros
    if (name == NULL || strlen(name) == 0) {
        return -1;
    }
    
    if (argc > 100) {  // Límite razonable
        return -1;
    }
    
    // Buscar módulo por nombre
    MainFunction code = findModuleByName(name);
    if (code == NULL) {
        return -1;  // Módulo no encontrado
    }
    
    // Copiar argumentos desde userland
    char **kernel_argv = copy_argv_from_userland(argv, argc);
    if (kernel_argv == NULL) {
        return -1;  // Error copiando argumentos
    }
    
    // Crear proceso
    Process *p = (Process*)mm_malloc(sizeof(Process));
    if (p == NULL) {
        mm_free(kernel_argv);
        return -1;
    }
    
    uint16_t parent = sched_getpid();
    uint16_t pid = next_pid++;
    
    // File descriptors por defecto (sin pipes aún)
    int16_t fds[3] = {STDIN, STDOUT, STDERR};
    
    // Inicializar proceso (esto copia argv internamente)
    initProcess(p, pid, parent, code, kernel_argv, name, 
                2, fds, 0);  // priority=2 (default), killable
    
    // Liberar kernel_argv ya que initProcess hizo su propia copia
    mm_free(kernel_argv);
    
    // Verificar que initProcess fue exitoso (verificando que stackBase fue asignado)
    if (p->stackBase == NULL) {
        // initProcess falló, liberar lo que se pudo asignar
        if (p->name != NULL) {
            mm_free(p->name);
        }
        mm_free(p);
        return -1;
    }
    
    // Registrar en scheduler
    if (sched_register_process(p) != 0) {
        freeProcess(p);
        return -1;
    }
    
    return (int64_t)pid;
}

int64_t my_nice(uint64_t pid, uint64_t newPrio) {
  return 0;
}

int64_t my_kill(uint64_t pid) {
  return 0;
}

int64_t my_block(uint64_t pid) {
  return 0;
}

int64_t my_unblock(uint64_t pid) {
  return 0;
}

int64_t my_sem_open(char *sem_id, uint64_t initialValue) {
  return 0;
}

int64_t my_sem_wait(char *sem_id) {
  return 0;
}

int64_t my_sem_post(char *sem_id) {
  return 0;
}

int64_t my_sem_close(char *sem_id) {
  return 0;
}

int64_t my_yield() {
  return 0;
}

int64_t my_wait(int64_t pid) {
    uint16_t currentPid = sched_getpid();
    uint16_t childPid = (uint16_t)pid;
    
    // Validar que el proceso hijo existe y es realmente hijo
    Process *child = sched_get_process_by_pid(childPid);
    if (child == NULL || child->parentPid != currentPid) {
        return -1;  // No existe o no es hijo de este proceso
    }
    
    // Si el hijo ya es zombie, retornar inmediatamente
    if (child->state == ZOMBIE) {
        // Buscar en la lista de zombies del padre
        Process *current = sched_get_current_process();
        if (current == NULL) {
            return -1;
        }
        
        LinkedListADT zombieList = (LinkedListADT)current->zombieChildren;
        if (zombieList != NULL) {
            Node *node = getFirst(zombieList);
            while (node != NULL) {
                Process *zombie = (Process*)node->data;
                if (zombie != NULL && zombie->pid == childPid && zombie->state == ZOMBIE) {
                    int32_t retValue = zombie->retValue;
                    removeNode(zombieList, node);
                    freeProcess(zombie);
                    return (int64_t)retValue;
                }
                node = node->next;
            }
        }
        // Si es zombie pero no está en la lista, algo está mal, pero retornamos el valor
        return (int64_t)child->retValue;
    }
    
    // Obtener proceso actual
    Process *current = sched_get_current_process();
    if (current == NULL) {
        return -1;
    }
    
    // No es zombie aún, bloquear hasta que termine
    current->waitingForPid = childPid;
    sched_set_status(currentPid, BLOCKED);
    sched_yield();  // Cambiar de contexto
    
    // Cuando despertemos, obtener proceso actual de nuevo (puede haber cambiado)
    current = sched_get_current_process();
    if (current == NULL) {
        return -1;
    }
    
    // Buscar el zombie en la lista
    LinkedListADT zombieList = (LinkedListADT)current->zombieChildren;
    if (zombieList != NULL) {
        Node *node = getFirst(zombieList);
        while (node != NULL) {
            Process *zombie = (Process*)node->data;
            if (zombie != NULL && zombie->pid == childPid && zombie->state == ZOMBIE) {
                int32_t retValue = zombie->retValue;
                Node *next = node->next;  // Guardar siguiente antes de remover
                removeNode(zombieList, node);
                freeProcess(zombie);
                current->waitingForPid = 0;
                return (int64_t)retValue;
            }
            node = node->next;
        }
    }
    
    current->waitingForPid = 0;
    return -1;  // Error: debería haber encontrado el zombie
}
