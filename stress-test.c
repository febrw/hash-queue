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

static void containsAll(void) {
    int contains;
    for (int i = 0; i < 65535; ++i) {
        contains = threadqueue -> contains(0, threadqueue);
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

        

        if (entry_belief != actual_loc) {
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
    const int test_size = 128;
    int indices[test_size];     // for each thread, stores its table index

    // Do enqueues
    QueueResultPair result;
    for (int i = 0; i < test_size; ++i) {
        result = threadqueue -> enqueue(threads[i], threadqueue);
        threadqueue = result.queue;
        hashqueue = (HashQueue*) threadqueue;
    }

    // store table indices
    for (int i = 0; i < test_size; ++i) {
        indices[i] = hashqueue -> getTableIndexByID(i, threadqueue);
        //printf("t_id: %d, idx: %d\n", i, indices[i]);
    }

    

    Thread *dqd_threads[test_size];
    for (int i = 0; i < test_size; ++i) {
        dqd_threads[i] = threadqueue -> dequeue(threadqueue);
        
    }

    //printf("\n\n\n\n\n\n\n");
    



  
    /*
    for (int i = 0; i < test_size; ++i) {
        u16 idx = indices[i];
        if (hashqueue -> table[idx] != NULL) {
            printf("Unfreed index: %u\n", idx);
        }
    }
    */



    //printf("here\n");

    /*
    int c = 0;
    for (int i = 0; i < hashqueue -> capacity; ++i) {
        if (hashqueue -> table[i] != NULL) {
            printf("bad idx: %d\n", i);
            ++c;
        }
    }
    printf("\n\nbad counts: %d\n", c);
    
    */

}

static void t3(void) {
    enqueueAll();

    int c = 0;
    for (int i = 0; i < hashqueue -> capacity; ++i) {
        if (hashqueue -> table[i] != NULL) {
            ++c;
        }
    }
    
    printf("Occupied counts:  %d\n", c);

    for (int i = 0; i < c; ++i) {
        threadqueue -> dequeue(threadqueue);
    }

    int d = 0;
    for (int i = 0; i < hashqueue -> capacity; ++i) {
        if (hashqueue -> table[i] != NULL) {
            ++d;
        }
    }

    printf("Occupied counts after deq all:  %d\n", d);
}

// correct threads being dequeued
static void t4(void) {
    enqueueAll();

    Thread *dqd;
    for (int i = 0; i < 65535; ++i) {
        dqd = threadqueue -> dequeue(threadqueue);
        if (dqd -> id != i) {
            printf("t_id: %d\n", i);
            return;
        }
    }
}

static void t5(void) {
    enqueueAll();

    Thread *dqd;
    int table_index;
    int bad_counts = 0;
    for (int i = 0; i < 65535; ++i) {
        table_index = hashqueue -> getTableIndexByID(i, threadqueue);
        dqd = threadqueue -> dequeue(threadqueue);
        assert(i == dqd -> id);
        if (hashqueue -> table[table_index] != NULL) {
            printf("Index not set to null: %d, t_id: %u\n", table_index, dqd -> id);
            ++bad_counts;
            return;
        }
    }
    printf("\n\n\n\n bad counts: %d\n", bad_counts);
}

static void t6(void) {
    enqueueAll();

    int occupied_counts1 = 0;
    for (int i = 0; i < hashqueue -> capacity; ++i) {
        if (hashqueue -> table[i] != NULL) {
            ++occupied_counts1;
        }
    }
    printf("Occupied counts after enq all: %d\n", occupied_counts1);


    int table_idx = hashqueue -> getTableIndexByID(0, threadqueue);
    //assert(table_idx == 34491);

    Thread *dqd = threadqueue -> dequeue(threadqueue);

    int occupied_counts2 = 0;
    for (int i = 0; i < hashqueue -> capacity; ++i) {
        if (hashqueue -> table[i] != NULL) {
            ++occupied_counts2;
        }
    }
    printf("Occupied counts after single dq: %d\n", occupied_counts2);

    if (hashqueue -> table[table_idx] != NULL) {
        printf("Dq did not set 34491 to null\n");
    }
    // So what did it set to null?
}

static void t7(void) {
    enqueueAll();

    int mismatch_count = 0;

    printf("ideal loc for t_id 0: %u\n", FNV1AHash(0) & (hashqueue -> capacity -1));

    for (int i = 0; i < 65535; ++i) {
        u16 table_index = (u16) hashqueue -> getTableIndexByID(i, threadqueue);
        u16 believed_index = hashqueue -> getEntryByID(i, threadqueue) -> table_index;
        //assert(believed_index ==  table_index);
        if (table_index != believed_index) {
            printf("ID: %d, Believed: %u, Table: %u\n", i, believed_index, table_index);
            ++mismatch_count;
            return;
        }
    }

    printf("mismatch count: %d\n", mismatch_count);
}



/*
    Problem:
    - after dequeueing all, freeing memory that was not allocated
    - table entries not set to null are erroneously being freed?
*/

int main() {

    setup();
    //t1();
    //t2();
    //t3();
    //t4();
    //t5();
    //t6();
    //t7();
    //enqueueAll();

    /*
    const double enqueue_time = timeFunction(enqueueAll);
    printf("Enqueue all time elapsed (ms): %f\n", enqueue_time);

    printf("size: %d\n", threadqueue -> size(threadqueue));
    printf("capacity: %d\n", hashqueue -> capacity);
    assert(hashqueue -> capacity == 131072);

    for (int i = 0; i < 65535; ++i) {
        assert(threadqueue -> contains(i, threadqueue) == 1);
    }

    */

    const double enqueue_time = timeFunction(enqueueAll);
    printf("Enqueue all time elapsed (ms): %f\n", enqueue_time);
    
    const double dequeue_time = timeFunction(dequeueAll);
    printf("Dequeue all time elapsed (ms): %f\n", dequeue_time);

    enqueueAll();
    const double remove_time = timeFunction(removeByIDAll);
    printf("remove all time elapsed (ms): %f\n", remove_time);

    assert(threadqueue -> size(threadqueue) == 0);

    enqueueAll();
    const double contains_time = timeFunction(containsAll);
    printf("contains all time elapsed (ms): %f\n", contains_time);



    

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