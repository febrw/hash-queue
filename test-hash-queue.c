#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "hash-queue.h"

static const int test_count = 67;
static int tests_passed = 0;

static ThreadQueue *threadqueue;
static HashQueue *hashqueue;
static thread* threads[256];
static thread* overlapping_threads[10];

static void initialiseOverlappingThreads(void) {
    for (u16 i = 0; i < 10; ++i) {
        overlapping_threads[i] = malloc(sizeof(thread));
    }

    overlapping_threads[0] -> id = 0;       // hashes to 0, placed at 0
    overlapping_threads[1] -> id = 128;     // hashes to 0, should be placed at 1
    overlapping_threads[2] -> id = 256;     // hashes to 0, should be placed at 2
    overlapping_threads[3] -> id = 3;       // in place
    overlapping_threads[4] -> id = 1;       // hashes to 1, should be placed at 4
    overlapping_threads[5] -> id = 129;     // hashes to 1, should be placed at 5
}

static void initialiseBasicThreads(void) {
    for (u16 i = 0; i < 256; ++i) {
        threads[i] = malloc(sizeof(thread));
        threads[i] -> id = i;
    }
}

static void initOK(void) {
    for (int i = 0; i < 256; ++i) {
        printf("Thread %d: ID: %u.\n", i, threads[i] -> id);
    }
}

static void setup() {
    threadqueue = (ThreadQueue*) new_HashQueue();
    hashqueue = (HashQueue*) threadqueue;
}

static void teardown() {
    threadqueue -> freeQueue(threadqueue);
}

static void runTest(void (*testFunction) (void)) {
    setup();
    testFunction();
    teardown();
}

/*
    Construction Test
    - private fields initialised correctly
*/

static void constructionTest(void) {
    assert(hashqueue -> _size == 0);
    assert(hashqueue -> capacity == INITIAL_CAPACITY);
    assert(hashqueue -> load_factor == 0.0);
    assert(hashqueue -> table != NULL);
    assert(hashqueue -> head == NULL);
    assert(hashqueue -> tail == NULL);

    ++tests_passed;
}


/*
    Enqueue tests
*/
static void sizeModified(void) {
    QueueResultPair result = threadqueue -> enqueue(threads[0], threadqueue);
    threadqueue = result.queue;

    assert(threadqueue -> size(threadqueue) == 1);                  // size is 1
    assert(threadqueue -> isEmpty(threadqueue) == 0);               // queue not empty

    assert(hashqueue -> load_factor != 0);                          // load factor has changed
    assert(hashqueue -> load_factor == (double) 1 / hashqueue -> capacity);

    ++tests_passed;
}

static void queuePointersUpdatedFirstInsertion(void) {
    QueueResultPair result = threadqueue -> enqueue(threads[0], threadqueue);
    threadqueue = result.queue;
    hashqueue = (HashQueue*) threadqueue;

    Entry *head = hashqueue -> head;
    Entry *tail = hashqueue -> tail;
    assert(head == tail);           // head and tail are same for first insertion
    assert(head -> next == NULL);   // next and prev are NULL
    assert(head -> prev == NULL);

    ++tests_passed;
}

static void tableUpdatedFirstInsertion(void) {
    QueueResultPair result = threadqueue -> enqueue(threads[0], threadqueue);
    threadqueue = result.queue;
    hashqueue = (HashQueue*) threadqueue;
    
    Entry *head = hashqueue -> head;                            // get the head
    u16 table_index = head -> table_index;                      // find its table index
    assert(hashqueue -> table[table_index] == head);            // verify entry's table index corresponds to table's entry

    ++tests_passed;
}

static void addressUnmodified(void) {
    ThreadQueue *original_address = threadqueue;
    QueueResultPair result = threadqueue -> enqueue(threads[0], threadqueue);
    threadqueue = result.queue;
    assert(threadqueue == original_address);

    ++tests_passed;
}

static void enqueueSuccessfulReturnValue(void) {
    QueueResultPair result = threadqueue -> enqueue(threads[0], threadqueue);
    threadqueue = result.queue;
    assert(result.result == 1);

    ++tests_passed;
}

static void queuePointersUpdatedSecondInsertion(void) {
    QueueResultPair result;
    
    for (int i = 0; i < 2; ++i) {
        result = threadqueue -> enqueue(threads[i], threadqueue);
        threadqueue = result.queue;
    }

    hashqueue = (HashQueue*) threadqueue;
    
    assert(hashqueue -> head != hashqueue -> tail);             // tail has been updated
    assert(hashqueue -> head -> next == hashqueue -> tail);     // the head's next pointer points to the tail
    assert(hashqueue -> tail -> prev == hashqueue -> head);     // the tail's prev pointer points to the head

    ++tests_passed;
}

static void insert3(void) {
    QueueResultPair result;

    for (int i = 0; i < 3; ++i) {
        result = threadqueue -> enqueue(threads[i], threadqueue);
        threadqueue = result.queue;
    }

    hashqueue = (HashQueue*) threadqueue;

    assert(hashqueue -> _size == 3);
    assert(hashqueue -> load_factor == (double) 3 / INITIAL_CAPACITY);

    assert(hashqueue -> head -> next -> t == threads[1]);   // head next OK
    assert(hashqueue -> tail -> prev -> t == threads[1]);   // tail prev OK

    // once tail -> prev verified
    Entry *middle = hashqueue -> tail -> prev;              
    assert(middle -> prev == hashqueue -> head);            // middle elem pointers OK
    assert(middle -> next == hashqueue -> tail);

    assert(hashqueue -> head -> table_index == 0);
    assert(middle -> table_index == 1);                     // middle's table index field is 1
    assert(hashqueue -> table[1] == middle);                // and the table at index 1 holds middle
    assert(hashqueue -> tail -> table_index == 2);
    

    ++tests_passed;
}

static void linearProbing(void) {
    QueueResultPair result;
    for (int i = 0; i < 6; ++i) {
        result = threadqueue -> enqueue(overlapping_threads[i], threadqueue);
        threadqueue = result.queue;
    }

    hashqueue = (HashQueue*) threadqueue;

    assert(hashqueue -> table[0] -> t -> id == 0);
    assert(hashqueue -> table[1] -> t -> id == 128);
    assert(hashqueue -> table[2] -> t -> id == 256);
    assert(hashqueue -> table[3] -> t -> id == 3);
    assert(hashqueue -> table[4] -> t -> id == 1);
    assert(hashqueue -> table[5] -> t -> id == 129);

    ++tests_passed;
}

/*
    Size tests
*/

static void sizeEmpty(void) {
    assert(threadqueue -> size(threadqueue) == 0);
    ++tests_passed;
}

static void sizeNonEmpty(void) {
    QueueResultPair result = threadqueue -> enqueue(threads[0], threadqueue);
    threadqueue = result.queue;
    
    assert(threadqueue -> size(threadqueue) == 1);

    ++tests_passed;
}

static void sizeMatchesPrivateField(void) {
    QueueResultPair result;
    for (int i = 0; i < 10; ++i) {
        result = threadqueue -> enqueue(threads[i], threadqueue);
        threadqueue = result.queue;
    }
    hashqueue = (HashQueue*) threadqueue;

    assert(threadqueue -> size(threadqueue) == hashqueue -> _size);
    ++tests_passed;
}


/*
    Dequeue Tests
*/


static void dequeueSizesChanged(void) {

    QueueResultPair result;
    for (int i = 0; i < 3; ++i) {
        result = threadqueue -> enqueue(threads[i], threadqueue);
        threadqueue = result.queue;
    }

    double old_load_factor = hashqueue -> load_factor;
    int old_size = hashqueue -> _size;
    thread *dequeued = threadqueue -> dequeue(threadqueue);
    hashqueue = (HashQueue*) threadqueue;

    assert(hashqueue -> load_factor != old_load_factor);
    assert(hashqueue -> _size == old_size - 1);

    ++tests_passed;
}

static void dequeueEmptyFails(void) {
    thread *dequeued = threadqueue -> dequeue(threadqueue);
    assert(dequeued == NULL);

    ++tests_passed;
}

static void dequeueCorrectElement(void) {
    QueueResultPair result;
    for (int i = 0; i < 3; ++i) {
        result = threadqueue -> enqueue(threads[i], threadqueue);
        threadqueue = result.queue;
    }

    thread *dequeued = threadqueue -> dequeue(threadqueue);

    assert(dequeued -> id == 0);
    ++tests_passed;
}

static void dequeuePointersMended(void) {
    QueueResultPair result;
    for (int i = 0; i < 3; ++i) {
        result = threadqueue -> enqueue(threads[i], threadqueue);
        threadqueue = result.queue;
    }

    thread *dequeued = threadqueue -> dequeue(threadqueue);
    hashqueue = (HashQueue*) threadqueue;

    assert(hashqueue -> head -> t -> id == 1);                  // new head is thread 1
    assert(hashqueue -> head -> prev == NULL);                  // new head has no prev element
    assert(hashqueue -> head -> next == hashqueue -> tail);     // new head's next is the tail

    ++tests_passed;
}

static void dequeueTableMended(void) {
    QueueResultPair result;
    for (int i = 0; i < 3; ++i) {
        result = threadqueue -> enqueue(threads[i], threadqueue);
        threadqueue = result.queue;
    }

    thread *dequeued = threadqueue -> dequeue(threadqueue);
    hashqueue = (HashQueue*) threadqueue;

    // Table OK
    assert(hashqueue -> table[0] == NULL);
    assert(hashqueue -> table[1] -> t -> id == 1);
    assert(hashqueue -> table[2] -> t -> id == 2);

    ++tests_passed;
}

/*
    Table repair procedure test

    Before:
    Slot    thread_id
    0       0
    1       128
    2       256
    3       3
    4       1
    5       129

    After dequeueing thread with id 0, we expect 4 deletion restorations to take place:
    -   thread id 128 (slot 1) should move to slot 0 (just vacated by id 0)
    -   thread id 256 (slot 2) should move to slot 1 (just vacated by id 128)
    -   thread id 1 (slot 4) should move to slot 2 (just vacated by id 256)
    -   thread id 129 (slot 5) should move to slot 4 (just vacated by id 1)
    -   slot 5 should be empty
*/
static void dequeueTableRepairTest(void) {

    QueueResultPair result;
    for (int i = 0; i < 6; ++i) {
        result = threadqueue -> enqueue(overlapping_threads[i], threadqueue);
        threadqueue = result.queue;
    }
    hashqueue = (HashQueue*) threadqueue;

    assert(hashqueue -> table[0] -> t -> id == 0);          // slot 0 occupied by thread 0

    thread *dequeued = threadqueue -> dequeue(threadqueue);

    assert(hashqueue -> table[0] -> t -> id == 128);        // id 128 moved to slot 0
    assert(hashqueue -> table[0] -> table_index == 0);

    assert(hashqueue -> table[1] -> t -> id == 256);        // id 256 moved to slot 1
    assert(hashqueue -> table[1] -> table_index == 1);

    assert(hashqueue -> table[2] -> t -> id == 1);          // id 129 moved to slot 4
    assert(hashqueue -> table[2] -> table_index == 2);

    assert(hashqueue -> table[3] -> t -> id == 3);          // id 3 did not move
    assert(hashqueue -> table[3] -> table_index == 3);

    assert(hashqueue -> table[4] -> t -> id == 129);        // id 128 moved to slot 0
    assert(hashqueue -> table[4] -> table_index == 4);

    assert(hashqueue -> table[5] == NULL);                  // slot 5 is empty

    ++tests_passed;
}

static void dequeueLoneElementQPointersAmended(void) {
    QueueResultPair result = threadqueue -> enqueue(threads[0], threadqueue);
    threadqueue = result.queue;

    hashqueue = (HashQueue*) threadqueue;

    assert(hashqueue -> head == hashqueue -> tail);

    threadqueue -> dequeue(threadqueue);

    assert(hashqueue -> head == NULL);
    assert(hashqueue -> tail == NULL);

    ++tests_passed;
}

static void dequeueTableIndicesUpdated(void) {
    thread* test_threads[8];

    for (int i = 0; i < 8; ++i) {
        test_threads[i] = malloc(sizeof(thread));
        if (test_threads[i] == NULL) {
            printf("mem alloc failed\n");
            assert(1==0);
        }
    }

    // 128, 0, 1, 2, 3, 4, 5, 6
    test_threads[0] -> id = 128;
    for (int i = 1; i < 8; ++i) {
        test_threads[i] -> id = i-1;
    }

    QueueResultPair result;
    for (int i = 0; i < 8; ++i) {
        result = threadqueue -> enqueue(test_threads[i], threadqueue);
        threadqueue = result.queue;
    }
    hashqueue = (HashQueue*) threadqueue;

    thread* dequeued = threadqueue -> dequeue(threadqueue);
    assert(dequeued -> id == 128);

    // All threads shifted down
    for (int i = 0; i < 7; ++i) {
        assert(hashqueue -> table[i] -> table_index == i);
        assert(hashqueue -> table[i] -> t -> id == i);
    }

    assert(hashqueue -> table[7] == NULL);

    ++tests_passed;
}

/*
    RemoveByID tests
*/

static void removeByIDCorrectElement(void) {
    QueueResultPair result;
    for (int i = 0; i < 10; ++i) {
        result = threadqueue -> enqueue(threads[i], threadqueue);
        threadqueue = result.queue;
    }

    thread *removed = threadqueue -> removeByID(5, threadqueue);
    assert(removed -> id == 5);

    ++tests_passed;
}

static void removeByIDSizeChanged(void) {
    QueueResultPair result;
    for (int i = 0; i < 10; ++i) {
        result = threadqueue -> enqueue(threads[i], threadqueue);
        threadqueue = result.queue;
    }

    assert(threadqueue -> size(threadqueue) == 10);
    threadqueue -> removeByID(0, threadqueue);
    assert(hashqueue -> size(threadqueue) == 9);

    ++tests_passed;
}

static void removeByIDLoadFactorChanged(void) {
    QueueResultPair result;
    for (int i = 0; i < 10; ++i) {
        result = threadqueue -> enqueue(threads[i], threadqueue);
        threadqueue = result.queue;
    }

    hashqueue = (HashQueue*) threadqueue;

    assert(hashqueue -> load_factor == (double) 10 / INITIAL_CAPACITY);
    threadqueue -> removeByID(0, threadqueue);
    assert(hashqueue -> load_factor == (double) 9 / INITIAL_CAPACITY);

    ++tests_passed;
}

static void removeByIDNotFound(void) {
    QueueResultPair result;
    for (int i = 0; i < 10; ++i) {
        result = threadqueue -> enqueue(threads[i], threadqueue);
        threadqueue = result.queue;
    }

    thread *found = threadqueue -> removeByID(42, threadqueue);
    assert(found == NULL);
    assert(threadqueue -> size(threadqueue) == 10);

    ++tests_passed;
}

static void removeByIDIntermediatePointersMended(void) {
    QueueResultPair result;
    for (int i = 0; i < 10; ++i) {
        result = threadqueue -> enqueue(threads[i], threadqueue);
        threadqueue = result.queue;
    }

    hashqueue = (HashQueue*) threadqueue;

    threadqueue -> removeByID(5, threadqueue);
    Entry *entry4 = hashqueue -> table[4];
    Entry *entry6 = hashqueue -> table[6];

    assert(entry4 -> next == entry6);
    assert(entry6 -> prev == entry4);

    ++tests_passed;
}

static void removeByIDIntermediateTableAmended(void) {
    QueueResultPair result;
    for (int i = 0; i < 10; ++i) {
        result = threadqueue -> enqueue(threads[i], threadqueue);
        threadqueue = result.queue;
    }

    hashqueue = (HashQueue*) threadqueue;

    threadqueue -> removeByID(5, threadqueue);

    assert(hashqueue -> table[5] == NULL);

    ++tests_passed;
}

static void removeByIDHeadPointersAmended(void) {
    QueueResultPair result;
    for (int i = 0; i < 10; ++i) {
        result = threadqueue -> enqueue(threads[i], threadqueue);
        threadqueue = result.queue;
    }

    hashqueue = (HashQueue*) threadqueue;
    
    assert(hashqueue -> head == hashqueue -> table[0]);             // original head at 0
    threadqueue -> removeByID(0, threadqueue);                      // remove head by its ID
    assert(hashqueue -> head == hashqueue -> table[1]);             // new head is at table index 1
    assert(hashqueue -> table[2] -> prev == hashqueue -> head);
    assert(hashqueue -> head -> prev == NULL);

    ++tests_passed;
}

static void removeByIDHeadTableAmended(void) {
    QueueResultPair result;
    for (int i = 0; i < 10; ++i) {
        result = threadqueue -> enqueue(threads[i], threadqueue);
        threadqueue = result.queue;
    }

    hashqueue = (HashQueue*) threadqueue;

    assert(hashqueue -> table[0] -> t -> id == 0);
    threadqueue -> removeByID(0, threadqueue);

    assert(hashqueue -> table[0] == NULL);

    ++tests_passed;
}

static void removeByIDTailPointersAmended(void) {
    QueueResultPair result;
    for (int i = 0; i < 10; ++i) {
        result = threadqueue -> enqueue(threads[i], threadqueue);
        threadqueue = result.queue;
    }

    hashqueue = (HashQueue*) threadqueue;
    
    assert(hashqueue -> tail == hashqueue -> table[9]);
    threadqueue -> removeByID(9, threadqueue);
    assert(hashqueue -> tail == hashqueue -> table[8]);
    assert(hashqueue -> tail -> next == NULL);
    assert(hashqueue -> table[7] -> next == hashqueue -> tail);

    ++tests_passed;
}

static void removeByIDTailTableAmended(void) {
    QueueResultPair result;
    for (int i = 0; i < 10; ++i) {
        result = threadqueue -> enqueue(threads[i], threadqueue);
        threadqueue = result.queue;
    }

    hashqueue = (HashQueue*) threadqueue;

    assert(hashqueue -> table[9] -> t -> id == 9);
    threadqueue -> removeByID(9, threadqueue);

    assert(hashqueue -> table[9] == NULL);

    ++tests_passed;
}

/*
    Before:
    Slot    thread_id
    0       0
    1       128
    2       256
    3       3
    4       1
    5       129

    If we remove id 128, we should expect:

    -   thread id 256 (slot 2) should move to slot 1 (just vacated by id 128)
    -   thread id 1 (slot 4) should move to slot 2 (just vacated by id 256)
    -   thread id 129 (slot 5) should move to slot 4 (just vacated by id 1)
    -   slot 5 should be empty

*/
static void removeByIDTableRepairTest(void) {
    QueueResultPair result;
    for (int i = 0; i < 6; ++i) {
        result = threadqueue -> enqueue(overlapping_threads[i], threadqueue);
        threadqueue = result.queue;
    }

    hashqueue = (HashQueue*) threadqueue;

    threadqueue -> removeByID(128, threadqueue);

    assert(hashqueue -> table[1] -> t -> id == 256);        // id 256 moved to slot 1
    assert(hashqueue -> table[1] -> table_index == 1);

    assert(hashqueue -> table[2] -> t -> id == 1);          // id 129 moved to slot 4
    assert(hashqueue -> table[2] -> table_index == 2);

    assert(hashqueue -> table[3] -> t -> id == 3);          // id 3 did not move
    assert(hashqueue -> table[3] -> table_index == 3);

    assert(hashqueue -> table[4] -> t -> id == 129);        // id 128 moved to slot 0
    assert(hashqueue -> table[4] -> table_index == 4);

    assert(hashqueue -> table[5] == NULL);                  // slot 5 is empty

    ++tests_passed;
}

/*
    Enqueueing 124, 252, 380, 508, 0, 636 should give:

        Table slot  | Thread ID
            0           0
            1           636

            125         124
            126         252
            127         380
            128         508

    - removeByID(380) should force wraparound to give

    Table slot  | Thread ID
            0           0
            1           -

            125         124
            126         252
            127         508
            128         636

*/

static void removeByIDTableRepairWrapAround(void) {
    // Setup
    thread* test_threads[6];

    test_threads[0] = malloc(sizeof(thread));
    if (test_threads[0] != NULL) {
        test_threads[0] -> id = 0;
    }

    for (int i = 1; i < 6; ++i) {
        test_threads[i] = malloc(sizeof(thread));
        if (test_threads[i] != NULL) {
            test_threads[i] -> id = 128 * (i-1) + 124;
        }
    }

    // Enqueuing 124, 252, 380, 508
    QueueResultPair result;
    for (int i = 1; i < 5; ++i) {
        result = threadqueue -> enqueue(test_threads[i], threadqueue);
        threadqueue = result.queue;
    }

    // Then enqueue 0, 636
    // Q: (head) -> 124, 252, 380, 508, 0, 636

    result = threadqueue -> enqueue(test_threads[0], threadqueue);
    threadqueue = result.queue;
    result = threadqueue -> enqueue(test_threads[5], threadqueue);
    threadqueue = result.queue;


    assert(hashqueue -> table[0] -> t -> id == 0);
    assert(hashqueue -> table[1] -> t -> id == 636);
    assert(hashqueue -> table[124] -> t -> id == 124);
    assert(hashqueue -> table[125] -> t -> id == 252);
    assert(hashqueue -> table[126] -> t -> id == 380);
    assert(hashqueue -> table[127] -> t -> id == 508);
    
    threadqueue -> removeByID(380, threadqueue);

    /*
        508 moves into slot 126, vacated by removed thread 380
        636 moves into slot 127, (wrapped back around), vacated by 508
    */
    assert(hashqueue -> table[0] -> t -> id == 0);
    assert(hashqueue -> table[1] == NULL);
    assert(hashqueue -> table[124] -> t -> id == 124);
    assert(hashqueue -> table[125] -> t -> id == 252);

    assert(hashqueue -> table[126] -> t -> id == 508);
    assert(hashqueue -> table[126] -> table_index == 126);

    assert(hashqueue -> table[127] -> t -> id == 636);
    assert(hashqueue -> table[127] -> table_index == 127);

    ++tests_passed;
}


static void removeByIDLoneElement(void) {
    threadqueue -> enqueue(threads[0], threadqueue);
    hashqueue = (HashQueue*) threadqueue;

    threadqueue -> removeByID(0, threadqueue);
    assert(threadqueue -> isEmpty(threadqueue) == 1);
    assert(threadqueue -> size(threadqueue) == 0);
    assert(hashqueue -> head == NULL);
    assert(hashqueue -> tail == NULL);

    ++tests_passed;
}

static void removeByIDDuplicateIDs(void) {
    // IDs 0, 1, 2, 0
    thread* test_threads[4];

    for (int i = 0; i < 4; ++i) {
        test_threads[i] = malloc(sizeof(thread));
        if (test_threads[i] == NULL) {
            printf("mem alloc failed\n");
            assert(1==0);
        } else {
            test_threads[i] -> id = i % 3;
        }
    }
    QueueResultPair result;
    for (int i = 0; i < 4; ++i) {
        result = threadqueue -> enqueue(test_threads[i], threadqueue);
        threadqueue = result.queue;
    }
    hashqueue = (HashQueue*) threadqueue;

    assert(hashqueue -> table[0] -> t -> id == 0);
    assert(hashqueue -> table[1] -> t -> id == 1);
    assert(hashqueue -> table[2] -> t -> id == 2);
    assert(hashqueue -> table[3] -> t -> id == 0);

    threadqueue -> removeByID(0, threadqueue);

    assert(hashqueue -> table[0] -> t -> id == 0);
    assert(hashqueue -> table[1] -> t -> id == 1);
    assert(hashqueue -> table[2] -> t -> id == 2);
    assert(hashqueue -> table[3] == NULL);

    ++tests_passed;
}

static void removeByIDTableIndicesUpdated1(void) {
    thread* test_threads[8];

    for (int i = 0; i < 8; ++i) {
        test_threads[i] = malloc(sizeof(thread));
        if (test_threads[i] == NULL) {
            printf("Mem alloc failed.\n");
            assert(1==0);
        }
    }
    // 0, 128, 1, 129, 2, 130, 3, 131
    for (int i = 0; i < 4; ++i) {
        test_threads[i*2] -> id = i;
        test_threads[2*i+1] -> id = i + 128;
    }

    QueueResultPair result;
    for (int i = 0; i < 8; ++i) {
        result = threadqueue -> enqueue(test_threads[i], threadqueue);
        threadqueue = result.queue;
    }
    hashqueue = (HashQueue*) threadqueue;
    
    // Before
    assert(hashqueue -> table[0] -> t -> id == 0);
    assert(hashqueue -> table[1] -> t -> id == 128);
    assert(hashqueue -> table[2] -> t -> id == 1);
    assert(hashqueue -> table[3] -> t -> id == 129);
    assert(hashqueue -> table[4] -> t -> id == 2);
    assert(hashqueue -> table[5] -> t -> id == 130);
    assert(hashqueue -> table[6] -> t -> id == 3);
    assert(hashqueue -> table[7] -> t -> id == 131);
    
    
    threadqueue -> removeByID(0, threadqueue);
    // After RemoveByID(0)
    assert(hashqueue -> table[0] -> t -> id == 128);
    assert(hashqueue -> table[0] -> table_index == 0);

    assert(hashqueue -> table[1] -> t -> id == 1);
    assert(hashqueue -> table[1] -> table_index == 1);

    assert(hashqueue -> table[2] -> t -> id == 129);
    assert(hashqueue -> table[2] -> table_index == 2);

    assert(hashqueue -> table[3] -> t -> id == 2);
    assert(hashqueue -> table[3] -> table_index == 3);

    assert(hashqueue -> table[4] -> t -> id == 130);
    assert(hashqueue -> table[4] -> table_index == 4);

    assert(hashqueue -> table[5] -> t -> id == 3);
    assert(hashqueue -> table[5] -> table_index == 5);

    assert(hashqueue -> table[6] -> t -> id == 131);
    assert(hashqueue -> table[6] -> table_index == 6);

    assert(hashqueue -> table[7] == NULL);

    ++tests_passed; 
}

static void removeByIDTableIndicesUpdated2(void) {
    thread* test_threads[6];

    for (int i = 0; i < 6; ++i) {
        test_threads[i] = malloc(sizeof(thread));
        if (test_threads[i] == NULL) {
            printf("Mem alloc failed.\n");
            assert(1==0);
        }
    }

    test_threads[0] -> id = 0;
    test_threads[1] -> id = 1;
    test_threads[2] -> id = 2;
    test_threads[3] -> id = 128;        // ideal slot: 0
    test_threads[4] -> id = 129;        // ideal slot: 1
    test_threads[5] -> id = 133;        // in place

    QueueResultPair result;
    for (int i = 0; i < 6; ++i) {
        result = threadqueue -> enqueue(test_threads[i], threadqueue);
        threadqueue = result.queue;
    }
    hashqueue = (HashQueue*) threadqueue;

    threadqueue -> removeByID(0, threadqueue);
    
    assert(hashqueue -> table[0] -> t -> id == 128);        // moved
    assert(hashqueue -> table[0] -> table_index == 0);      // and index updated

    assert(hashqueue -> table[1] -> t -> id == 1);
    assert(hashqueue -> table[1] -> table_index == 1);

    assert(hashqueue -> table[2] -> t -> id == 2);
    assert(hashqueue -> table[2] -> table_index == 2);

    assert(hashqueue -> table[3] -> t -> id == 129);        // moved
    assert(hashqueue -> table[3] -> table_index == 3);      // and index updated

    assert(hashqueue -> table[4] == NULL);

    assert(hashqueue -> table[5] -> t -> id == 133);
    assert(hashqueue -> table[5] -> table_index == 5);

    ++tests_passed;
}

/*
    GetByID tests
*/

static void getByIDFalseReturnsNull(void) {
    thread* retrieved = threadqueue -> getByID(0, threadqueue);
    assert(retrieved == NULL);

    ++tests_passed;
}

static void getByIDReturnsCorrect(void) {
    QueueResultPair result = threadqueue -> enqueue(threads[10], threadqueue);
    threadqueue = result.queue;

    thread* retrieved = threadqueue -> getByID(10, threadqueue);
    assert(retrieved != NULL);
    assert(retrieved -> id == 10);

    ++tests_passed;
}

static void getByIDNoModifications(void) {
    QueueResultPair result = threadqueue -> enqueue(threads[10], threadqueue);
    threadqueue = result.queue;

    thread* retrieved = threadqueue -> getByID(10, threadqueue);
    assert(threadqueue -> size(threadqueue) == 1);
    assert(threadqueue -> isEmpty(threadqueue) == 0);

    ++tests_passed;
}

static void getByIDTwiceSameElementFound(void) {
    QueueResultPair result = threadqueue -> enqueue(threads[10], threadqueue);
    threadqueue = result.queue;

    thread* retrieved1 = threadqueue -> getByID(10, threadqueue);
    thread* retrieved2 = threadqueue -> getByID(10, threadqueue);
    assert(retrieved1 == retrieved2);
    
    ++tests_passed;
}

static void getByIDDuplicateElemsSameFound(void) {
    QueueResultPair result;
    for (int i = 0; i < 3; ++i) {
        result = threadqueue -> enqueue(threads[0], threadqueue);
        threadqueue = result.queue;
    }
    hashqueue = (HashQueue*) threadqueue;
    
    for (int i = 0; i < 3; ++i) {
        assert(hashqueue -> table[i] -> t -> id == 0);
    }

    thread* retrieved1 = threadqueue -> getByID(0, threadqueue);
    thread* retrieved2 = threadqueue -> getByID(0, threadqueue);
    assert(retrieved1 == retrieved2);
    
    ++tests_passed;
}

/*
    Contains Tests
*/

static void containsTrue(void) {
    threadqueue -> enqueue(threads[3], threadqueue);
    assert(threadqueue -> contains(3, threadqueue) == 1);

    ++tests_passed;
}

static void containsFalse(void) {
    assert(threadqueue -> contains(3, threadqueue) == 0);
    ++tests_passed;
}

static void containsFalseAfterDequeue(void) {
    QueueResultPair result = threadqueue -> enqueue(threads[3], threadqueue);
    threadqueue = result.queue;
    
    threadqueue -> dequeue(threadqueue);
    assert(threadqueue -> contains(3, threadqueue) == 0);

    ++tests_passed;
}

static void containsFalseAfterRemoveByID(void) {
    QueueResultPair result = threadqueue -> enqueue(threads[3], threadqueue);
    threadqueue = result.queue;
    threadqueue -> removeByID(3, threadqueue);
    assert(threadqueue -> contains(3, threadqueue) == 0);

    ++tests_passed;
}

static void containsContiguousBlockTest(void) {
    // Setup
    thread* test_threads[4];
    for (int i = 0; i < 4; ++i) {
        test_threads[i] = malloc(sizeof(thread));
        if (test_threads[i] == NULL) {
            printf("Mem alloc failed.\n");
            assert(1==0);
        }
    }

    test_threads[0] -> id = 125;
    test_threads[1] -> id = 126;
    test_threads[2] -> id = 127;
    test_threads[3] -> id = 253;        // hashes to 125

    QueueResultPair result;
    for (int i = 0; i < 4; ++i) {
        result = threadqueue -> enqueue(test_threads[i], threadqueue);
    }
    hashqueue = (HashQueue*) threadqueue;

    assert(hashqueue -> table[0] -> t -> id == 253);

    threadqueue -> removeByID(126, threadqueue);

    assert(threadqueue -> contains(126, threadqueue) == 0);
    assert(threadqueue -> contains(125, threadqueue) == 1);
    assert(threadqueue -> contains(127, threadqueue) == 1);
    assert(threadqueue -> contains(253, threadqueue) == 1);

    assert(hashqueue -> table[126] -> t -> id == 253);

    ++tests_passed;
}


/*
    Rehash Tests
*/

static void noRehashBeforeThreshold(void) {
    // do not surpass 50%
    ThreadQueue* original_queue_address = threadqueue;

    QueueResultPair result;
    for (int i = 0; i < 64; ++i) {
        result = threadqueue -> enqueue(threads[i], threadqueue);
        threadqueue = result.queue;
    }

    assert(original_queue_address == threadqueue);

    ++tests_passed;
}

static void addressModifiedAfterRehash(void) {
    ThreadQueue* original_queue_address = threadqueue;

    QueueResultPair result;
    for (int i = 0; i < 64; ++i) {
        result = threadqueue -> enqueue(threads[i], threadqueue);
        threadqueue = result.queue;
    }

    result = threadqueue -> enqueue(threads[64], threadqueue);
    threadqueue = result.queue;

    assert(original_queue_address != threadqueue);

    ++tests_passed;
}



static void newTableQPointersCorrect(void) {
    ThreadQueue* original_queue_address = threadqueue;

    QueueResultPair result;
    for (int i = 0; i < 64; ++i) {
        result = threadqueue -> enqueue(threads[i], threadqueue);
        threadqueue = result.queue;
    }
    hashqueue = (HashQueue*) threadqueue;

    Entry *original_head = hashqueue -> head;
    Entry *next_up = hashqueue -> head -> next;
    Entry *original_tail = hashqueue -> tail; 

    result = threadqueue -> enqueue(threads[64], threadqueue);
    threadqueue = result.queue;

    hashqueue = (HashQueue*) threadqueue;
    assert(original_head == hashqueue -> head);
    assert(next_up == hashqueue -> head -> next);
    assert(original_tail == hashqueue -> tail -> prev);

    ++tests_passed;
}

static void correctRehashLocations(void) {
    // IDs [64, 191]
    thread* test_threads[128];
    for (int i = 0; i < 128; ++i) {
        test_threads[i] = malloc(sizeof(thread));
        if (test_threads[i] == NULL) {
            printf("mem alloc failed\n");
            assert(1==0);
        } else {
            test_threads[i] -> id = i + 64;
        }
    }

    QueueResultPair result;
    for (int i = 0; i < 64; ++i) {
        result = threadqueue -> enqueue(test_threads[i], threadqueue);
        threadqueue = result.queue;
    }
    hashqueue = (HashQueue*) threadqueue;

    // first 64 slots empty
    for (int i = 0; i < 64; ++i) {
        assert(hashqueue -> table[i] == NULL);
    }

    // latter 64 filled
    for (int i = 64; i < 128; ++i) {
        assert(hashqueue -> table[i] -> t -> id == i);
    }

    // Add further 64 elements, forcing rehashing on first enqueue
    for (int i = 64; i < 128; ++i) {
        result = threadqueue -> enqueue(test_threads[i], threadqueue);
        threadqueue = result.queue;
    }
    hashqueue = (HashQueue*) threadqueue;

    // New table
    // first 64 should be empty
    for (int i = 0; i < 64; ++i) {
        assert(hashqueue -> table[i] == NULL);
    }

    // middle 128 should be full
    for (int i = 64; i < 192; ++i) {
        assert(hashqueue -> table[i] -> t -> id == i);
    }

    // final 64 should be empty
    for (int i = 192; i < 256; ++i) {
        assert(hashqueue -> table[i] == NULL);
    }

    ++tests_passed;
}

static void rehashTableFieldsUpdated(void) {
    // IDs [64, 191]
    thread* test_threads[128];
    for (int i = 0; i < 128; ++i) {
        test_threads[i] = malloc(sizeof(thread));
        if (test_threads[i] == NULL) {
            printf("mem alloc failed\n");
            assert(1==0);
        } else {
            test_threads[i] -> id = i + 64;
        }
    }

    QueueResultPair result;
    for (int i = 0; i < 64; ++i) {
        result = threadqueue -> enqueue(test_threads[i], threadqueue);
        threadqueue = result.queue;
    }

    hashqueue = (HashQueue*) threadqueue;

    int old_size = hashqueue -> _size;
    int old_capacity = hashqueue -> capacity;
    double old_load_factor = hashqueue -> load_factor;

    // Add 1 more element, forcing rehashing
    result = threadqueue -> enqueue(test_threads[64], threadqueue);
    threadqueue = result.queue;

    hashqueue = (HashQueue*) threadqueue;

    int new_size = hashqueue -> _size;
    int new_capacity = hashqueue -> capacity;
    double new_load_factor = hashqueue -> load_factor;
    

    assert(new_size == old_size + 1);
    assert(new_capacity == old_capacity * 2);
    assert(new_load_factor == ((old_load_factor) / 2) + ((double) 1 / 256)); // halved, plus 1 / 256

    ++tests_passed;
}

static void rehashFullChainMaintained(void) {
    // IDs [64, 191]
    thread* test_threads[128];
    for (int i = 0; i < 128; ++i) {
        test_threads[i] = malloc(sizeof(thread));
        if (test_threads[i] == NULL) {
            printf("mem alloc failed\n");
            assert(1==0);
        } else {
            test_threads[i] -> id = i + 64;
        }
    }

    QueueResultPair result;
    // IDs [64, 127]
    for (int i = 0; i < 64; ++i) {
        result = threadqueue -> enqueue(test_threads[i], threadqueue);
        threadqueue = result.queue;
    }
    hashqueue = (HashQueue*) threadqueue;

    u16 original_ids[64];
    Entry *curr = hashqueue -> head;
    
    // store old ids by following next pointers
    for (int i = 0; curr != NULL; ++i, curr = curr -> next) {
        original_ids[i] = curr -> t -> id;
    }

    // Add 1 more element, forcing rehashing
    result = threadqueue -> enqueue(test_threads[64], threadqueue);
    threadqueue = result.queue;
    hashqueue = (HashQueue*) threadqueue;

    // store ids in new table by following next pointers
    u16 test_ids[65];
    curr = hashqueue -> head;
    for (int i = 0; curr != NULL; ++i, curr = curr -> next) {
        test_ids[i] = curr -> t -> id;
    }

    // equal for first 64
    for (int i = 0; i < 64; ++i) {
        assert(original_ids[i] == test_ids[i]);
    }
    assert(test_ids[64] == 128);

    ++tests_passed;
}

/*
    isEmpty tests
*/
static void isEmptyTrue(void) {
    assert(threadqueue -> isEmpty(threadqueue) == 1);
    ++tests_passed;
}


static void isEmptyFalse(void) {
    QueueResultPair result;
    for (int i = 0; i < 4; ++i) {
        result = threadqueue -> enqueue(threads[i], threadqueue);
        threadqueue = result.queue;
    }

    assert(threadqueue -> isEmpty(threadqueue) == 0);
    ++tests_passed;
}

static void isEmptyFollowingDequeues(void) {
    QueueResultPair result;
    for (int i = 0; i < 4; ++i) {
        result = threadqueue -> enqueue(threads[i], threadqueue);
        threadqueue = result.queue;
    }

    for (int i = 0; i < 4; ++i) {
        threadqueue -> dequeue(threadqueue);
    }

    assert(threadqueue -> isEmpty(threadqueue) == 1);
    ++tests_passed;
}

static void isEmptyFollowingRemoveByIDs(void) {
    QueueResultPair result;
    for (int i = 0; i < 4; ++i) {
        result = threadqueue -> enqueue(threads[i], threadqueue);
        threadqueue = result.queue;
    }

    for (int i = 0; i < 4; ++i) {
        threadqueue -> removeByID(i, threadqueue);
    }

    assert(threadqueue -> isEmpty(threadqueue) == 1);
    ++tests_passed;
}

static void isEmptyPointersAgreePositive(void) {
    QueueResultPair result;
    for (int i = 0; i < 4; ++i) {
        result = threadqueue -> enqueue(threads[i], threadqueue);
        threadqueue = result.queue;
    }
    hashqueue = (HashQueue*) threadqueue;

    assert(threadqueue -> isEmpty(threadqueue) == 
        (hashqueue -> head == NULL && hashqueue -> tail == NULL));

    ++tests_passed;
}

static void isEmptyPointersAgreeNegative(void) {
    QueueResultPair result;
    for (int i = 0; i < 4; ++i) {
        result = threadqueue -> enqueue(threads[i], threadqueue);
        threadqueue = result.queue;
    }
    hashqueue = (HashQueue*) threadqueue;

    assert(threadqueue -> isEmpty(threadqueue) == 
        (hashqueue -> head == NULL && hashqueue -> tail == NULL));

    for (int i = 0; i < 4; ++i) {
        threadqueue -> removeByID(i, threadqueue);
    }

    assert(threadqueue -> isEmpty(threadqueue) == 
        (hashqueue -> head == NULL && hashqueue -> tail == NULL));

    ++tests_passed;
}





static void tableRepairNoMoveTest1(void) {
    thread* test_threads[3];

    for (int i = 0; i < 3; ++i) {
        test_threads[i] = malloc(sizeof(thread));
        if (test_threads[i] == NULL) {
            printf("mem alloc failed\n");
            assert(1==0);
        }
    }

    test_threads[0] -> id = 0;
    test_threads[1] -> id = 1;
    test_threads[2] -> id = 129;

    QueueResultPair result;
    for (int i = 0; i < 3; ++i) {
        result = threadqueue -> enqueue(test_threads[i], threadqueue);
    }
    hashqueue = (HashQueue*) threadqueue;

    // Before removeByID(0)
    assert(hashqueue -> table[0] -> t -> id == 0);
    assert(hashqueue -> table[1] -> t -> id == 1);
    assert(hashqueue -> table[2] -> t -> id == 129);

    threadqueue -> removeByID(0, threadqueue);

    // After removeByID(126)
    assert(hashqueue -> table[0] == NULL);
    assert(hashqueue -> table[1] -> t -> id == 1);
    assert(hashqueue -> table[2] -> t -> id ==  129);
    
    ++tests_passed;
}


static void tableRepairNoMoveTest2(void) {
    thread* test_threads[3];

    for (int i = 0; i < 3; ++i) {
        test_threads[i] = malloc(sizeof(thread));
        if (test_threads[i] == NULL) {
            printf("mem alloc failed\n");
            assert(1==0);
        }
    }

    test_threads[0] -> id = 0;
    test_threads[1] -> id = 127;
    test_threads[2] -> id = 128;

    QueueResultPair result;
    for (int i = 0; i < 3; ++i) {
        result = threadqueue -> enqueue(test_threads[i], threadqueue);
    }
    hashqueue = (HashQueue*) threadqueue;

    // Before removeByID(0)
    assert(hashqueue -> table[0] -> t -> id == 0);
    assert(hashqueue -> table[0] -> table_index == 0);

    assert(hashqueue -> table[1] -> t -> id == 128);
    assert(hashqueue -> table[1] -> table_index == 1);

    assert(hashqueue -> table[127] -> t -> id == 127);
    assert(hashqueue -> table[127] -> table_index == 127);

    threadqueue -> removeByID(127, threadqueue);

    // After removeByID(0)
    assert(hashqueue -> table[0] -> t -> id == 0);
    assert(hashqueue -> table[0] -> table_index == 0);

    assert(hashqueue -> table[1] -> t -> id == 128);
    assert(hashqueue -> table[1] -> table_index == 1);

    assert(hashqueue -> table[127] == NULL);

    ++tests_passed;
}

static void tableRepairNoMoveTest3(void) {
    thread* test_threads[3];

    for (int i = 0; i < 3; ++i) {
        test_threads[i] = malloc(sizeof(thread));
        if (test_threads[i] == NULL) {
            printf("mem alloc failed\n");
            assert(1==0);
        }
    }

    test_threads[0] -> id = 126;
    test_threads[1] -> id = 127;
    test_threads[2] -> id = 255;

    QueueResultPair result;
    for (int i = 0; i < 3; ++i) {
        result = threadqueue -> enqueue(test_threads[i], threadqueue);
    }

    // Before removeByID(126)
    assert(hashqueue -> table[0] -> t -> id == 255);
    assert(hashqueue -> table[126] -> t -> id == 126);
    assert(hashqueue -> table[127] -> t -> id == 127);

    threadqueue -> removeByID(126, threadqueue);

    // After removeByID(126)
    assert(hashqueue -> table[0] -> t -> id == 255);
    assert(hashqueue -> table[126] == NULL);
    assert(hashqueue -> table[127] -> t -> id == 127);

    ++tests_passed;
}

static void constructIteratorTest(void) {
    Iterator *iterator = threadqueue -> iterator(threadqueue);
    assert(iterator != NULL);

    free(iterator);
    ++tests_passed;
}

static void iteratorHasNextTrue(void) {
    QueueResultPair result = threadqueue -> enqueue(threads[0], threadqueue);
    threadqueue = result.queue;
    Iterator *iterator = threadqueue -> iterator(threadqueue);

    assert(iterator -> hasNext(iterator) != 0);

    free(iterator);
    ++tests_passed;
}

static void iteratorHasNextEmptyFalse(void) {
    Iterator *iterator = threadqueue -> iterator(threadqueue);;
    assert(iterator -> hasNext(iterator) == 0);
    assert(iterator -> hasNext(iterator) == 0);

    free(iterator);
    ++tests_passed;
}

static void iteratorHasNextDoesNotModify(void) {
    QueueResultPair result = threadqueue -> enqueue(threads[0], threadqueue);
    Iterator *iterator = threadqueue -> iterator(threadqueue);

    assert(iterator -> hasNext(iterator) != 0);
    assert(iterator -> hasNext(iterator) != 0);
    assert(iterator -> hasNext(iterator) != 0);

    free(iterator);
    ++tests_passed;
}

static void iteratorCorrectNext(void) {
    QueueResultPair result = threadqueue -> enqueue(threads[0], threadqueue);
    Iterator *iterator = threadqueue -> iterator(threadqueue);;

    assert(iterator -> hasNext(iterator) != 0);         // next exists
    assert(iterator -> next(iterator) == threads[0]);   // correct thread reference returned
    assert(iterator -> hasNext(iterator) == 0);         // no further next element

    free(iterator);
    ++tests_passed;
}


static void iteratorExampleUsage(void) {
    QueueResultPair result;
    for (int i = 0; i < 8; ++i) {
        result = threadqueue -> enqueue(threads[i], threadqueue);
        threadqueue = result.queue;
    }

    Iterator *it = threadqueue -> iterator(threadqueue);
    thread *current_thread;

    int idx = 0;
    while (it -> hasNext(it)) {
        current_thread = (it -> next(it));
        assert(current_thread == threads[idx]);
        ++idx;
    }

    free(it);
    ++tests_passed;
}



int main(void) {
    // Setup global test variables
    initialiseBasicThreads();
    initialiseOverlappingThreads();

    // Construction Test
    runTest(constructionTest);

    // Enqueue tests
    runTest(sizeModified);
    runTest(queuePointersUpdatedFirstInsertion);
    runTest(tableUpdatedFirstInsertion);
    runTest(addressUnmodified);
    runTest(enqueueSuccessfulReturnValue);
    runTest(queuePointersUpdatedSecondInsertion);
    runTest(insert3);
    runTest(linearProbing);

    
    // Size tests
    runTest(sizeEmpty);
    runTest(sizeNonEmpty);
    runTest(sizeMatchesPrivateField);

    // Dequeue tests
    runTest(dequeueSizesChanged);
    runTest(dequeueEmptyFails);
    runTest(dequeueCorrectElement);
    runTest(dequeuePointersMended);
    runTest(dequeueTableMended);
    runTest(dequeueTableRepairTest);
    runTest(dequeueLoneElementQPointersAmended);
    runTest(dequeueTableIndicesUpdated);

    // removeByID tests
    runTest(removeByIDCorrectElement);
    runTest(removeByIDSizeChanged);
    runTest(removeByIDLoadFactorChanged);
    runTest(removeByIDNotFound);
    runTest(removeByIDIntermediatePointersMended);
    runTest(removeByIDIntermediateTableAmended);
    runTest(removeByIDHeadPointersAmended);
    runTest(removeByIDHeadTableAmended);
    runTest(removeByIDTailPointersAmended);
    runTest(removeByIDTailTableAmended);
    runTest(removeByIDTableRepairTest);
    runTest(removeByIDTableRepairWrapAround);
    runTest(removeByIDLoneElement);
    runTest(removeByIDDuplicateIDs);
    runTest(removeByIDTableIndicesUpdated1);
    runTest(removeByIDTableIndicesUpdated2);

    // GetByID tests
    runTest(getByIDFalseReturnsNull);
    runTest(getByIDReturnsCorrect);
    runTest(getByIDNoModifications);
    runTest(getByIDTwiceSameElementFound);
    runTest(getByIDDuplicateElemsSameFound);

    // Contains tests
    runTest(containsTrue);
    runTest(containsFalse);
    runTest(containsFalseAfterDequeue);
    runTest(containsFalseAfterRemoveByID);
    runTest(containsContiguousBlockTest);

    // Rehashing tests
    runTest(noRehashBeforeThreshold);
    runTest(addressModifiedAfterRehash);
    runTest(newTableQPointersCorrect);
    runTest(correctRehashLocations);
    runTest(rehashTableFieldsUpdated);
    runTest(rehashFullChainMaintained);

    // isEmpty tests
    runTest(isEmptyTrue);
    runTest(isEmptyFalse);
    runTest(isEmptyFollowingDequeues);
    runTest(isEmptyFollowingRemoveByIDs);
    runTest(isEmptyPointersAgreePositive);
    runTest(isEmptyPointersAgreeNegative);
    
    // Table repair tests
    runTest(tableRepairNoMoveTest1);
    runTest(tableRepairNoMoveTest2);
    runTest(tableRepairNoMoveTest3);

    // Iterator tests
    runTest(constructIteratorTest);
    runTest(iteratorHasNextTrue);
    runTest(iteratorHasNextEmptyFalse);
    runTest(iteratorHasNextDoesNotModify);
    runTest(iteratorCorrectNext);
    runTest(iteratorExampleUsage);
    
    
    printf("Passed %u/%u tests.\n", tests_passed, test_count);
    return 0; 
}