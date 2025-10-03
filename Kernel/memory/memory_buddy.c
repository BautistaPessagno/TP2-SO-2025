#include "../include/memory_manager.h"
#include <stddef.h>
#include <stdint.h>

static Header base;                 // ancla de la free-list (lista circular)
static MemoryManagerADT mm = 0;     // manager en dirección fija


static uintptr_t align_up_uintptr(uintptr_t p, size_t a) {
    const uintptr_t mask = (uintptr_t)(a - 1);
    return (p + mask) & ~mask;
}

int power(int base, int power){ // por ahi tengo q cambiar el tipo q retorna
    if (!power) {
        return 1;
    }

    for (int i = 0; power > 1; power--) {
        base = base * base;
    }
    return base;
};

size_t memory_required(size_t nbytes) {
    if (nbytes == 0) {
        return 1;
    }
    
    size_t result = 1;
    while (result < nbytes) {
        result <<= 1;
    }
    
    return result;
}

Header *findBlock(Header *header, size_t memory_required) {
    if (header == NULL) {
        return NULL;
    }
    
    Header *current = header;
    
    do {
        if (current->s.units == memory_required) {
            return current;
        }
        current = current->s.next;
    } while (current != NULL && current != header);
    
    return NULL;
}

void splitHeader(Header *header) {
    if (header == NULL || header->s.units < 2) {
        return; // Cannot split a block smaller than 2 units
    }
    
    size_t half_size = header->s.units / 2;
    
    // Create the second header at the midpoint
    Header *second_header = (Header *)((uint8_t *)header + half_size);
    
    // Set up the second header
    second_header->s.units = half_size;
    second_header->s.next = header->s.next;
    
    // Update the first header
    header->s.units = half_size;
    header->s.next = second_header;
}

MemoryManagerADT create_memory_manager(/* void* const restrict managed_memory, */ uint64_t memory_amount /* reservado */) {
    (void)memory_amount;

    // Construir el objeto en la dirección fija
    mm = (MemoryManagerADT)MEMORY_MANAGER_FIRST_ADDRESS;

    // Calcular límites del pool administrado
    uintptr_t manager_begin      = (uintptr_t)MEMORY_MANAGER_FIRST_ADDRESS;
    uintptr_t manager_end        = manager_begin + (uintptr_t)sizeof(*mm);
    uintptr_t aligned_pool_start = align_up_uintptr(manager_end, sizeof(Header));
    uintptr_t pool_end_uint      = (uintptr_t)MEMORY_MANAGER_LAST_ADDRESS; // exclusivo

    mm->pool_start    = (uint8_t *)aligned_pool_start;
    mm->pool_end      = (uint8_t *)pool_end_uint;
    mm->memory_amount = (uint64_t)(pool_end_uint - aligned_pool_start); //Cantidad de bytes de la memoria.

    // en este punto tengo 3*2ˆ20 bytes de memoria. como quiero potencia de 2, uso 2ˆ21
    int dif =  mm->memory_amount - power(2, 21);
    mm->pool_end = mm->pool_end - dif; 
    mm->memory_amount = power(2, 21); // nueva cantidad de bytes


    // Crear el primer gran bloque libre que cubre todo el pool
    if (mm->memory_amount >= sizeof(Header)) {
        Header *first = (Header *)mm->pool_start;
        first->s.units = mm->memory_amount;
        first->s.next = NULL;

        mm->free = first;
    }

    return mm;
}


void *mm_malloc(size_t nbytes) {
    if (mm == 0 || nbytes == 0) {
        return 0;
    }

    size_t bytes_required = memory_required(nbytes);

    Header *prevp = mm->free;    // prevp es el nodo anterior al que se va a asignar
    if (prevp == 0) {
        return 0;
    }

    Header *p = findBlock(prevp, bytes_required);

    // si p tiene valor se le asigno un bloque que cumplia con la memoria requerida. 
    // si no tiene valor, sigo dividiendo el primer bloque (free)
    while (!p) {
        // Check if we can split the first block
        if (prevp->s.units < bytes_required || prevp->s.units < 2) {
            return NULL; // Cannot allocate: block too small to split
        }
        
        splitHeader(prevp);
        p = findBlock(prevp, bytes_required); // itera toda la lista cuando ya sabe que solo pueden ser los primeros dos :(
    }

    // Return pointer to the payload (after the header)
    return (void *)(p + 1);
}

