#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H
#include <stdint.h>
#include <stddef.h>

// ULL es para que el compilador sepa que es un unsigned long long
#define MEMORY_MANAGER_FIRST_ADDRESS 0x0000000000100000ULL
#define MEMORY_MANAGER_LAST_ADDRESS  0x00000000FFFFFFFFULL 

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
} *MemoryManagerADT;

MemoryManagerADT create_memory_manager(/* void* const restrict managed_memory, */ uint64_t memory_amount /* reservado para futuro */);
void *mm_malloc(size_t nbytes);
void  mm_free(void *ptr);

#endif