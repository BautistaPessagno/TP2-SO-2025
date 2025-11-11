// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

// Doubly linked list implementation for scheduler-ready intrusive mode
#include <stddef.h>
#include <stdint.h>

#include "../include/memory_manager.h"
#include "../include/linkedListADT.h"

typedef struct LinkedListCDT {
    Node *first;
    Node *last;
    Node *current;
    int len;
} LinkedListCDT;

static void link_at_back(LinkedListADT list, Node *node) {
    node->prev = list->last;
    node->next = NULL;
    if (list->last != NULL) {
        list->last->next = node;
    }
    list->last = node;
    if (list->first == NULL) {
        list->first = node;
    }
    list->len++;
}

static void link_at_front(LinkedListADT list, Node *node) {
    node->prev = NULL;
    node->next = list->first;
    if (list->first != NULL) {
        list->first->prev = node;
    }
    list->first = node;
    if (list->last == NULL) {
        list->last = node;
    }
    list->len++;
}

LinkedListADT createLinkedListADT(void) {
    LinkedListADT list = (LinkedListADT)mm_malloc(sizeof(LinkedListCDT));
    if (list == NULL) {
        return NULL;
    }
    list->first = NULL;
    list->last = NULL;
    list->current = NULL;
    list->len = 0;
    return list;
}

void freeLinkedListADT(LinkedListADT list) {
    if (list == NULL) {
        return;
    }
    mm_free(list);
}

void freeLinkedListADTDeep(LinkedListADT list) {
    if (list == NULL) {
        return;
    }
    // Contrato: en el scheduler, solo quedarán nodos "propios" de la lista
    Node *it = list->first;
    while (it != NULL) {
        Node *next = it->next;
        // Liberamos el Node, nunca el data
        mm_free(it);
        it = next;
    }
    list->first = NULL;
    list->last = NULL;
    list->current = NULL;
    list->len = 0;
    mm_free(list);
}

Node *appendElement(LinkedListADT list, void *data) {
    if (list == NULL) {
        return NULL;
    }
    Node *node = (Node *)mm_malloc(sizeof(Node));
    if (node == NULL) {
        return NULL;
    }
    node->data = data;
    node->prev = NULL;
    node->next = NULL;
    link_at_back(list, node);
    return node;
}

Node *appendNode(LinkedListADT list, Node *node) {
    if (list == NULL || node == NULL) {
        return NULL;
    }
    // No modificar data; solo encadenar
    link_at_back(list, node);
    return node;
}

Node *prependNode(LinkedListADT list, Node *node) {
    if (list == NULL || node == NULL) {
        return NULL;
    }
    // No modificar data  solo encadenar
    link_at_front(list, node);
    return node;
}

Node *getFirst(LinkedListADT list) {
    if (list == NULL) {
        return NULL;
    }
    return list->first;
}

Node *popFront(LinkedListADT list) {
    if (list == NULL) {
        return NULL;
    }
    Node *first = list->first;
    if (first == NULL) {
        return NULL;
    }
    // Desenlazar en O(1)
    if (first->next != NULL) {
        first->next->prev = NULL;
    } else {
        // era único
        list->last = NULL;
    }
    list->first = first->next;
    first->prev = NULL;
    first->next = NULL;
    list->len--;
    return first;
}

void *removeNode(LinkedListADT list, Node *node) {
    if (list == NULL || node == NULL) {
        return NULL;
    }

    // Ajustar vecinos
    if (node->prev == NULL && node->next == NULL) {
        // único o ya desenlazado
        if (list->first == node && list->last == node) {
            list->first = NULL;
            list->last = NULL;
            list->len--;
        }
    } else if (node->prev == NULL) {
        // primero (y hay siguiente)
        list->first = node->next;
        if (node->next != NULL) {
            node->next->prev = NULL;
        }
        list->len--;
        if (list->last == node) {
            list->last = node->next; // por seguridad
        }
	    } else if (node->next == NULL) {
	        // último (y hay anterior)
	        list->last = node->prev;
	        node->prev->next = NULL;
	        list->len--;
	        if (list->first == node) {
	            list->first = node->prev; // por seguridad
        }
    } else {
        // intermedio
        node->prev->next = node->next;
        node->next->prev = node->prev;
        list->len--;
    }

    // Si apuntaban a current, avanzar el cursor
    if (list->current == node) {
        list->current = node->next;
    }

    void *data = node->data;
    node->prev = NULL;
    node->next = NULL;
    return data;
}

int isEmpty(LinkedListADT list) {
    if (list == NULL) {
        return -1;
    }
    return (list->len == 0) ? 1 : 0;
}

int getLength(LinkedListADT list) {
    if (list == NULL) {
        return -1;
    }
    return list->len;
}

void begin(LinkedListADT list) {
    if (list == NULL) {
        return;
    }
    list->current = list->first;
}

int hasNext(LinkedListADT list) {
    if (list == NULL) {
        return -1;
    }
    return (list->current != NULL) ? 1 : 0;
}

void *next(LinkedListADT list) {
    if (hasNext(list) != 1) {
        return NULL;
    }
    void *ret = list->current->data;
    list->current = list->current->next;
    return ret;
}
