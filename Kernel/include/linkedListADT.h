#ifndef LINKED_LIST_ADT_H
#define LINKED_LIST_ADT_H

#include <stddef.h>
#include <stdint.h>

// Nodo público (para modo intrusivo)
typedef struct Node {
    struct Node *prev;
    struct Node *next;
    void *data;
} Node;

// Lista opaca (puntero a struct interna)
typedef struct LinkedListCDT *LinkedListADT;

// Creación / destrucción
LinkedListADT createLinkedListADT(void);
void freeLinkedListADT(LinkedListADT list);
void freeLinkedListADTDeep(LinkedListADT list);

// Inserciones
Node *appendElement(LinkedListADT list, void *data);
Node *appendNode(LinkedListADT list, Node *node);
Node *prependNode(LinkedListADT list, Node *node);

// Acceso / remoción
Node *getFirst(LinkedListADT list);
Node *popFront(LinkedListADT list);
void *removeNode(LinkedListADT list, Node *node);

// Utilidades
int   isEmpty(LinkedListADT list);
int   getLength(LinkedListADT list);

// Iteración simple (no modificar la lista durante la iteración)
void  begin(LinkedListADT list);
int   hasNext(LinkedListADT list);
void *next(LinkedListADT list);

#endif


