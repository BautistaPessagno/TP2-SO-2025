// K&R-style free-list memory manager (freestanding, no libc)
#include <memoryManager.h>
#include <stdint.h>

// Estado interno (singleton)
static Header base;                 // ancla de la free-list (lista circular)
static MemoryManagerADT mm = 0;     // manager en dirección fija

// Convierte bytes a unidades de Header, incluyendo el header del bloque
static size_t to_units(size_t nbytes) {
    return (nbytes + sizeof(Header) - 1) / sizeof(Header) + 1;
}

// Alinea p hacia arriba al múltiplo a
// a = sizeof(Header) en nuestro caso
// basicamente “suma” hasta el siguiente borde de tamaño a, y luego “corta” los bits bajos para caer justo en ese borde.
static uintptr_t align_up_uintptr(uintptr_t p, size_t a) {
    const uintptr_t mask = (uintptr_t)(a - 1);
    return (p + mask) & ~mask;
}

// TODO --------------------- HACER UNA LIB PARA MANEJAR LISTAS CIRCULARES ---------------------
// Inserta un bloque libre bp en la free-list circular manteniendo orden por dirección.
// Realiza coalescing con vecinos (merge con anterior y siguiente) si corresponde.
static void insert_and_coalesce(Header *bp) {
    Header *p = mm->free;
    if (p == 0) {
        // Lista aún no inicializada; crear lista con base como centinela
        base.s.next = &base;
        base.s.units = 0;
        mm->free = &base;
        p = &base;
    }

    // Buscar posición por dirección (lista circular ordenada)
    for (; !(bp > p && bp < p->s.next); p = p->s.next) {
        // Caso de wrap: p >= p->s.next significa que cruzamos el final del espacio
        if (p >= p->s.next && (bp > p || bp < p->s.next)) {
            break;
        }
    }

    // Intentar fusionar con el siguiente (upper neighbor)
    if (bp + bp->s.units == p->s.next) {
        bp->s.units += p->s.next->s.units;
        bp->s.next = p->s.next->s.next;
    } else {
        bp->s.next = p->s.next;
    }

    // Intentar fusionar con el anterior (lower neighbor)
    if (p + p->s.units == bp) {
        p->s.units += bp->s.units;
        p->s.next = bp->s.next;
    } else {
        p->s.next = bp;
    }

    // Mantener el puntero libre en algún nodo estable
    mm->free = p;
}

// Crea/Inicializa el memory manager en la dirección fija, con un pool fijo
// inmediatamente después del manager hasta MEMORY_MANAGER_LAST_ADDRESS (exclusivo).
MemoryManagerADT create_memory_manager(/* void* const restrict managed_memory, */ uint64_t memory_amount /* reservado */) {
    (void)memory_amount;

    // Construir el objeto en la dirección fija
    mm = (MemoryManagerADT)MEMORY_MANAGER_FIRST_ADDRESS;

    // Calcular límites del pool administrado
    uintptr_t manager_begin = (uintptr_t)MEMORY_MANAGER_FIRST_ADDRESS;
    uintptr_t manager_end   = manager_begin + (uintptr_t)sizeof(*mm);
    uintptr_t aligned_pool_start = align_up_uintptr(manager_end, sizeof(Header));
    uintptr_t pool_end_uint      = (uintptr_t)MEMORY_MANAGER_LAST_ADDRESS; // exclusivo

    mm->pool_start    = (uint8_t *)aligned_pool_start;
    mm->pool_end      = (uint8_t *)pool_end_uint;
    mm->memory_amount = (uint64_t)(pool_end_uint - aligned_pool_start);

    // Inicializar free-list (circular, ancla base)
    base.s.next = &base;
    base.s.units = 0;
    mm->free = &base;

    // Crear el primer gran bloque libre que cubre todo el pool
    if (mm->memory_amount >= sizeof(Header)) {
        Header *first = (Header *)mm->pool_start;
        first->s.units = (mm->memory_amount) / sizeof(Header);
        // Insertarlo en la free-list y coalescar por si coincidiera con bordes
        insert_and_coalesce(first);
    }

    return mm;
}

// Asigna nbytes del pool fijo usando first-fit. Split si el bloque es mayor.
void *mm_malloc(size_t nbytes) {
    if (mm == 0 || nbytes == 0) {
        return 0;
    }

    size_t nunits = to_units(nbytes);

    Header *prevp = mm->free;    // prevp es el nodo anterior al que se va a asignar
    if (prevp == 0) {
        return 0;
    }

    // p y prevp avanzan juntos 
    for (Header *p = prevp->s.next;; prevp = p, p = p->s.next) { // p es el nodo actual
        if (p->s.units >= nunits) {
            if (p->s.units == nunits) {
                // Quitar bloque completo
                prevp->s.next = p->s.next;
            } else {
                // Tomar la cola (split)
                p->s.units -= nunits;
                p += p->s.units;
                p->s.units = nunits;
            }
            mm->free = prevp;
            return (void *)(p + 1);
        }
        if (p == mm->free) {
            // Recorrió toda la lista y no hay espacio
            return 0;
        }
    }
}

// Libera un bloque y realiza coalescing con vecinos si corresponde.
void mm_free(void *ptr) {
    if (mm == 0 || ptr == 0) {
        return;
    }


    // CLAVE ESTO DEL K & R, sacado del libro. Une bloques libres adyacentes, y para 
    // hacer un free, le resta uno al puntero para obtener bloques de memoria usada.
    // basicamente son dos listas distintas digamos
    // Freeing also causes a search of the free list, to find the proper place to insert the block being freed. If the block being freed is adjacent to a free block on either side, it
// is coalesced with it into a single bigger block, so storage does not become too fragmented. Determining the adjacency is easy because the free list is maintained in
// order of decreasing address. 

    Header *bp = (Header *)ptr - 1; 

    // Validar que el header cae dentro del pool
    if ((uint8_t *)bp < mm->pool_start || (uint8_t *)bp >= mm->pool_end) {
        return; // ignorar puntero inválido
    }

    insert_and_coalesce(bp);
}

