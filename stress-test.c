#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>


#include "hash-queue.h"

static ThreadQueue *threadqueue;
static HashQueue *hashqueue;
static Thread *threads[65535];

static void setup() {
    threadqueue = (ThreadQueue*) new_HashQueue();
    hashqueue = (HashQueue*) threadqueue;
    //hashqueue -> getHash = IDHash;

    for (int i = 0; i < 65535; ++i) {
        threads[i] = malloc(sizeof(Thread));
        if (threads[i] == NULL) {
            printf("malloc failed");
            exit(0);
        } else {
            threads[i] -> id = i;
        }
    }
}

static void wrapUp() {
    threadqueue -> freeQueue(threadqueue);
    for (int i = 0; i < 65535; ++i) {
        free(threads[i]);
    }
}

static void enqueueAll(void) {
    QueueResultPair result;
    for (int i = 0; i < 65535; ++i) {
        result = threadqueue -> enqueue(threads[i], threadqueue);
        threadqueue = result.queue;
    }
    hashqueue = (HashQueue*) threadqueue;
}

static void removeByIDAll(void) {
    Thread *removed;
    for (int i = 0; i < 65535; ++i) {
        removed = threadqueue -> removeByID(i, threadqueue);
    }
}

static void dequeueAll(void) {
    Thread *dequeued;
    for (int i = 0; i < 65535; ++i) {
        dequeued = threadqueue -> dequeue(threadqueue);
    }
}

static double timeFunction(void (*testFunction) (void)) {
    clock_t begin = clock();
    testFunction();
    clock_t end = clock();
    return (double) (end - begin) * 1000 / CLOCKS_PER_SEC;
}


// Correction Tests


/*
    t1:
    -   entry beliefs match table indices
    -   probing seems fine
*/
static void t1(void) {
    QueueResultPair result;
    for (int i = 0; i < 65535; ++i) {
        result = threadqueue -> enqueue(threads[i], threadqueue);
        threadqueue = result.queue;
        hashqueue = (HashQueue*) threadqueue;

        u16 ideal_loc = FNV1AHash(i) & (hashqueue -> capacity - 1);
        u16 actual_loc = hashqueue -> getTableIndexByID(i, threadqueue);
        u16 entry_belief = hashqueue -> getEntryByID(i, threadqueue) -> table_index;

        if (ideal_loc != actual_loc) {
            printf("i: %d, ideal: %u, actual: %u, entry_belief: %u\n\n", i, ideal_loc, actual_loc, entry_belief);
            
            for (int j = 0; j <= i; ++j) {
                printf("idx: %d, table_idx: %u, entry belief: %u\n",
                    j, hashqueue -> getTableIndexByID(j, threadqueue), hashqueue -> getEntryByID(j, threadqueue) -> table_index);
            }
            return;
        }
    }
}


/*
    t2:
    -   enqueue some stuff
    -   stores the table indices of each entry
    -   dequeue all the stuff
    -   check table entries

*/
static void t2(void) {
    u16 indices[256];
    QueueResultPair result;
    for (int i = 0; i < 256; ++i) {
        result = threadqueue -> enqueue(threads[i], threadqueue);
        threadqueue = result.queue;
        hashqueue = (HashQueue*) threadqueue;

        indices[i] = hashqueue -> getTableIndexByID(i, threadqueue);
        if(indices[i] == 372) {
            printf("thread causing issue: %u\n\n", i);
        }
    }

    Thread *dqd_threads[256];
    for (int i = 0; i < 256; ++i) {
        dqd_threads[i] = threadqueue -> dequeue(threadqueue);
    }

    for (int i = 0; i < 256; ++i) {
        u16 idx = indices[i];
        if (hashqueue -> table[idx] != NULL) {
            printf("Unfreed index: %u\n", idx);
        }
    }



    
}

/*
    Problem:
    - after dequeueing all, freeing memory that was not allocated
    - implies table entries not set to null are erroneously being freed
*/

int main() {

    setup();
    //t1();
    t2();

    /*
    const double enqueue_time = timeFunction(enqueueAll);
    printf("Enqueue all time elapsed (ms): %f\n", enqueue_time);

    printf("size: %d\n", threadqueue -> size(threadqueue));
    printf("capacity: %d\n", hashqueue -> capacity);
    assert(hashqueue -> capacity == 131072);

    for (int i = 0; i < 65535; ++i) {
        assert(threadqueue -> contains(i, threadqueue) == 1);
    }

    
    //const double dequeue_time = timeFunction(dequeueAll);
    //printf("Dequeue all time elapsed (ms): %f\n", dequeue_time);

    */

    /*
    Iterator *iterator = threadqueue -> iterator(threadqueue);
    int c = 0;
    while (iterator -> hasNext(iterator)) {
        Entry *curr = iterator -> currentEntry;
        u16 thread_id = curr -> t -> id;
        u16 inner_ref = curr -> table_index;    // where entry believes it is
        u16 table_ref = hashqueue -> getTableIndex(thread_id, threadqueue);
        if (inner_ref != table_ref) {
            //printf("Table believes entry is at %u\n"
                   // "Entry believes it is at %u\n\n", table_ref, inner_ref);
            ++c;
        }

        
        iterator -> next(iterator);
    }
    printf("bad counts: %d\n", c);

    */
    wrapUp();

    return 0;    
}