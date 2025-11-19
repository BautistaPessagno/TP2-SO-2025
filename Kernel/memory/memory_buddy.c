// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "../include/memory_manager.h"
#include <stddef.h>
#include <stdint.h>

static MemoryManagerADT mm = 0;     // manager en dirección fija

extern uint8_t endOfKernel;

// ---- helpers ----
static uintptr_t align_up_uintptr(uintptr_t p, size_t a) {
    const uintptr_t mask = (uintptr_t)(a - 1);
    return (p + mask) & ~mask;
}

static size_t to_units(size_t nbytes) {
    return (nbytes + sizeof(Header) - 1) / sizeof(Header) + 1;
}

static size_t next_power_of_two_units(size_t nunits) {
    if (nunits <= 1) return 1;
    // potencia de 2
    nunits--;
    nunits |= nunits >> 1;
    nunits |= nunits >> 2;
    nunits |= nunits >> 4;
    nunits |= nunits >> 8;
    nunits |= nunits >> 16;
#if SIZE_MAX > 0xFFFFFFFFu
    nunits |= nunits >> 32;
#endif
    nunits++;
    return nunits;
}

static size_t largest_power_of_two_not_exceeding(size_t nunits) {
    if (nunits == 0) return 0;
    size_t p = 1;
    while ((p << 1) > p && (p << 1) <= nunits) {
        p <<= 1;
    }
    return p;
}

static Header *find_and_detach_ge(size_t req_units) {
    Header *prev = NULL;
    Header *cur = mm->free;
    while (cur != NULL) {
        if (cur->s.units >= req_units) {
            if (prev == NULL) {
                mm->free = cur->s.next;
            } else {
                prev->s.next = cur->s.next;
            }
            cur->s.next = NULL;
            return cur;
        }
        prev = cur;
        cur = cur->s.next;
    }
    return NULL;
}

MemoryManagerADT create_memory_manager( uint64_t memory_amount ) {
    (void)memory_amount;

    const uintptr_t page_size = 0x1000;
    const uintptr_t stack_size = page_size * 8; // 32KB stack
    uintptr_t kernel_end = (uintptr_t)&endOfKernel;
    
    uintptr_t manager_begin = align_up_uintptr(kernel_end + stack_size + page_size, page_size);
    uintptr_t manager_end        = manager_begin + (uintptr_t)sizeof(*mm);
    uintptr_t aligned_pool_start = align_up_uintptr(manager_end, sizeof(Header));
    uintptr_t pool_end_uint      = (uintptr_t)MEMORY_MANAGER_LAST_ADDRESS; // exclusivo

    mm = (MemoryManagerADT)manager_begin;

    mm->pool_start    = (uint8_t *)aligned_pool_start;
    mm->pool_end      = (uint8_t *)pool_end_uint; // match memory_manager.c (exclusive end)
    mm->memory_amount = (uint64_t)(pool_end_uint - aligned_pool_start); // bytes in pool
    mm->allocated_bytes = 0;

    // Seed the free list by decomposing the whole pool into power-of-two blocks (Header units)
    mm->free = NULL;
    if (mm->memory_amount >= sizeof(Header)) {
        size_t total_units = (size_t)(mm->memory_amount / sizeof(Header));
        Header *cur = (Header *)mm->pool_start;
        while (total_units > 0) {
            size_t block_units = largest_power_of_two_not_exceeding(total_units);
            Header *blk = cur;
            blk->s.units = block_units;
            // insert sorted by address (at tail)
            blk->s.next = NULL;
            if (mm->free == NULL) {
                mm->free = blk;
            } else {
                Header *p = mm->free;
                while (p->s.next != NULL) p = p->s.next;
                p->s.next = blk;
            }
            cur += block_units;
            total_units -= block_units;
        }
    } else {
        // no usable memory -> lista vacia
        mm->free = NULL;
    }

    return mm;
}

static int are_contiguous(Header *a, Header *b) {
    return (a + a->s.units) == b;
}

static int are_buddies(Header *a, Header *b) {
    if (a->s.units != b->s.units) return 0;
    
    if (!are_contiguous(a, b) && !are_contiguous(b, a)) return 0;
    
    Header *baseH = (Header *)mm->pool_start;
    Header *left  = (a < b) ? a : b;
    size_t offset_units = (size_t)(left - baseH);
    
    return (offset_units % (left->s.units * 2)) == 0;
}

static void insert(Header *bp){
    if (mm->free == NULL || bp < mm->free) {
        bp->s.next = mm->free;
        mm->free = bp;
        return;
    }
    Header *p = mm->free;
    while (p->s.next != NULL && !(bp < p->s.next)) {
        p = p->s.next;
    }
    bp->s.next = p->s.next;
    p->s.next = bp;
}

static void coalesce_all(void){
    int merged;
    do {
        merged = 0;
        Header *p = mm->free;
        while (p != NULL && p->s.next != NULL) {
            Header *q = p->s.next;
            if (are_buddies(p, q)) {
                p->s.units *= 2;
                p->s.next = q->s.next;
                merged = 1;
            } else {   
                p = q;
            }
        }
    } while (merged);
}

void *mm_malloc(size_t nbytes) {
    if (mm == 0 || nbytes == 0) {
        return 0;
    }

    size_t nunits = to_units(nbytes);
    size_t req_units = next_power_of_two_units(nunits);
    if (mm->free == NULL) return 0;

    Header *blk = find_and_detach_ge(req_units);
    if (blk == NULL) return 0;

    while (blk->s.units > req_units) {
        size_t half = blk->s.units / 2;
        Header *buddy = blk + half;
        buddy->s.units = half;
        blk->s.units = half;
        buddy->s.next = NULL;
        insert(buddy);
    }

    mm->allocated_bytes += (uint64_t)(blk->s.units * sizeof(Header));

    return (void *)(blk + 1);
}

void mm_free(void *ptr) {
    if (mm == 0 || ptr == 0) {
        return;
    }

    Header *bp = (Header *)ptr - 1; 

    if ((uint8_t *)bp < mm->pool_start || (uint8_t *)bp >= mm->pool_end) { // castea a (uint8_t *) para comparar direcciones.
        return;
    }

    uint64_t bytes = (uint64_t)(bp->s.units * sizeof(Header));
    if (mm->allocated_bytes >= bytes) {
        mm->allocated_bytes -= bytes;
    } else {
        mm->allocated_bytes = 0;
    }

    insert(bp);
    coalesce_all();
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