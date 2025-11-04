#include "../include/memory_manager.h"
#include <stddef.h>
#include <stdint.h>
#include <sys/resource.h>

static Header base;                 // ancla de la free-list (lista circular)
static MemoryManagerADT mm = 0;     // manager en dirección fija

extern uint8_t endOfKernel;

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
    Header *prev = NULL;
    
    if (current->s.units == memory_required) { // el primer bloque tiene la memoria requerida
        header = header->s.next; // saco el primer bloque de la lista.
        return current;
    }

    prev = current;
    current = current->s.next;

    while (current != NULL){
        if (current->s.units == memory_required) {
            prev->s.next = current->s.next;
            return current;
        }
        prev = current;
        current = current->s.next;
    }
    
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

    const uintptr_t page_size = 0x1000;
    const uintptr_t stack_size = page_size * 8; // 32KB stack
    uintptr_t kernel_end = (uintptr_t)&endOfKernel;
    // Place manager after stack region (endOfKernel + 32KB + 4KB padding)


    // Construir el objeto en la dirección fija


    // Calcular límites del pool administrado
    uintptr_t manager_begin = align_up_uintptr(kernel_end + stack_size + page_size, page_size);
    uintptr_t manager_end        = manager_begin + (uintptr_t)sizeof(*mm);
    uintptr_t aligned_pool_start = align_up_uintptr(manager_end, sizeof(Header));
    uintptr_t pool_end_uint      = (uintptr_t)MEMORY_MANAGER_LAST_ADDRESS; // exclusivo

    mm = (MemoryManagerADT)manager_begin;

    mm->pool_start    = (uint8_t *)aligned_pool_start;
    mm->pool_end      = (uint8_t *)pool_end_uint;
    mm->memory_amount = (uint64_t)(pool_end_uint - aligned_pool_start); //Cantidad de bytes de la memoria.
    mm->allocated_bytes = 0;

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

// Chequeo si end position de P1 (p1 + units) es igual a la starting position de P2]
// no se si estan bien los casteos
int is_contiguous(Header *p1, Header *p2){
    return (uint8_t *)p1 + (uint8_t)p1->s.units == (uint8_t *)p2 - 1 ;
}


// Si no se puede llegar a la starting position de la memoria desde la starting position del nuevo bloque sumando bloques de ese tamano entonces los dos bloques que se estan analizando no fueron dividos al mismo tiempo (No son buddies)
// Suena raro pero esta chequeadisimo.
int is_buddy(Header *p1){
    uint8_t step = p1->s.units * 2;
    uint8_t *aux = (uint8_t *) p1;
    while (aux > mm->pool_start) {
        if (aux == mm->pool_start) {
            return 1;
        }
        aux -= step;
    }
    return 0;
}

// Retorna 1 si coalesco, para indicar que se puede seguir coalescando.
int coalesce(){
    Header *p = mm->free;

    while (p->s.next != NULL) {
        if((p->s.units == p->s.next->s.units) && is_contiguous(p, p->s.next) && is_buddy(p)){
            p->s.units = p->s.units * 2;
            p->s.next = p->s.next->s.next;
            return 1;
        }
    }
    return 0;
}

void insert(Header *bp){
    Header *p = mm->free;

    if (bp < p) {
        bp->s.next = p;
        mm->free = bp;
        return;
    }

    // Buscar posición por dirección (lista ordenanda ascendentemente)
    while (p->s.next != NULL) {
        if (bp < p->s.next) {
            bp->s.next = p->s.next;
            p->s.next = bp;
            return;
        }

        p = p->s.next; 
    }

    p->s.next = bp;
    return;

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
        p = findBlock(prevp, bytes_required); // itera toda la lista cuando ya  se sabe que solo pueden ser los primeros dos :(
    }

    // Contabilizar bytes del bloque asignado (bloque buddy completo)
    mm->allocated_bytes += (uint64_t)p->s.units;

    // Return pointer to the payload (after the header)
    return (void *)(p + 1);
}

void mm_free(void *ptr) {
    if (mm == 0 || ptr == 0) {
        return;
    }

    Header *bp = (Header *)ptr - 1; 

    if ((uint8_t *)bp < mm->pool_start || (uint8_t *)bp >= mm->pool_end) { // castea a (uint8_t *) para comparar direcciones. 
        return; // ignorar puntero inválido
    }

    // Descontar bytes del bloque liberado
    uint64_t bytes = (uint64_t)bp->s.units;
    if (mm->allocated_bytes >= bytes) {
        mm->allocated_bytes -= bytes;
    } else {
        mm->allocated_bytes = 0; // clamp defensivo
    }

    insert(bp);
    while (coalesce());
}

MMState mm_state(void) {
    MMState st = (MMState){0, 0, 0};
    if (mm == 0) {
        return st;
    }
    st.total = mm->memory_amount;
    st.allocated = mm->allocated_bytes;
    st.available = (mm->memory_amount >= mm->allocated_bytes)
        ? (mm->memory_amount - mm->allocated_bytes)
        : 0;
    return st;
}



