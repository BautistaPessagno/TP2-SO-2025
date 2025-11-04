#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H
#include <stdint.h>
#include <stddef.h>

// ULL es para que el compilador sepa que es un unsigned long long
//va desde el espacio libre hasta el espacio del SO
#define MEMORY_MANAGER_FIRST_ADDRESS 0x0000000000100000ULL
#define MEMORY_MANAGER_LAST_ADDRESS  0x00000000003FFFFFULL 



typedef long Align;

typedef union Header {
    struct {
        union Header *next;   // solo válido cuando está en la free-list
        size_t units;         // tamaño EN UNIDADES de Header (incluye header)
    } s;
    Align _;                  // fuerza alineación del Header/payload
} Header;

typedef struct MemoryManagerCDT {
    Header *free;             // puntero a algún nodo de la free-list (circular), es el Next
    uint8_t *pool_start;      // inicio del pool administrado
    uint8_t *pool_end;        // fin EXCLUSIVO del pool
    uint64_t memory_amount;   // bytes del pool
    uint64_t allocated_bytes; // bytes actualmente asignados (incluye overhead de Header)
} *MemoryManagerADT;

typedef struct {
    uint64_t total;       // bytes totales administrados por el manager
    uint64_t allocated;   // bytes asignados (incluye headers)
    uint64_t available;   // bytes disponibles = total - asignados
} MMState;

//en nuestrop SO, em memory amount va a ser MEMORY_MANAGER_LAST_ADDRESS - MEMORY_MANAGER_FIRST_ADDRESS
MemoryManagerADT create_memory_manager(/* void* const restrict managed_memory, */ uint64_t memory_amount /* reservado para futuro */);
void *mm_malloc(size_t nbytes);
void  mm_free(void *ptr);
MMState mm_state(void);

#endif