#include <stdlib.h>

#include "hash-queue.h"

static thread* Iterator_next(Iterator *iterator) {
    Entry *current = iterator -> currentEntry;
    iterator -> currentEntry = iterator -> currentEntry -> next;
    return current -> t;
}

static int Iterator_hasNext(Iterator *iterator) {
    return iterator -> currentEntry != NULL;  
}

Iterator* new_Iterator(ThreadQueue* queue) {
    HashQueue *hashqueue = (HashQueue*) queue;
    Iterator *iterator = malloc(sizeof(Iterator));
    if (iterator == NULL) {
        return NULL;
    }

    iterator -> hasNext = Iterator_hasNext;
    iterator -> next = Iterator_next;
    iterator -> currentEntry = hashqueue -> head;

    return iterator;
}

/*
    - malloc up space for iterator struct
    - set iterator's currentEntry to be hashqueue -> head
    - set the pointers to the functions inside the iterator stuct 
    - return pointer to the iterator

*/