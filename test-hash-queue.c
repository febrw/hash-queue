#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "hash-queue.h"

static const int tests_count = 42;
static int tests_passed = 0;

static HashQueue *hashqueue;
static thread* threads[128];
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
    for (u16 i = 0; i < 128; ++i) {
        threads[i] = malloc(sizeof(thread));
        threads[i] -> id = i;
    }
}

static void initOK(void) {
    for (int i = 0; i < 128; ++i) {
        printf("Thread %d: ID: %u.\n", i, threads[i] -> id);
    }
}

static void setup() {
    hashqueue = HashQueue_new();
}

static void teardown() {
    hashqueue -> freeQueue(hashqueue);
}

static void runTest(void (*testFunction) (void)) {
    setup();
    testFunction();
    teardown();
}

/*
    Construction Test
*/

static void constructionTest(void) {
    assert(hashqueue -> size == 0);
    assert(hashqueue -> load_factor == 0.0);
    assert(hashqueue -> rehash_threshold == 0.5);
    assert(hashqueue -> table != NULL);
    assert(hashqueue -> occupied_index != NULL);
    assert(hashqueue -> head == NULL);
    assert(hashqueue -> tail == NULL);

    ++tests_passed;
}


/*
    Enqueue tests
*/
static void sizeModified(void) {
    QueueResultPair result = hashqueue -> enqueue(threads[0], hashqueue);
    hashqueue = result.queue;

    assert(hashqueue -> size == 1);                 // size is 1
    assert(hashqueue -> isEmpty(hashqueue) == 0);   // queue not empty
    assert(hashqueue -> load_factor != 0);          // load factor has changed
    assert(hashqueue -> load_factor == (double) 1 / hashqueue -> capacity);

    ++tests_passed;
}

static void queuePointersUpdatedFirstInsertion(void) {
    QueueResultPair result = hashqueue -> enqueue(threads[0], hashqueue);
    hashqueue = result.queue;

    Entry *head = hashqueue -> head;
    Entry *tail = hashqueue -> tail;
    assert(head == tail);           // head and tail are same for first insertion
    assert(head -> next == NULL);   // next and prev are NULL
    assert(head -> prev == NULL);

    ++tests_passed;
}

static void tableUpdatedFirstInsertion(void) {
    QueueResultPair result = hashqueue -> enqueue(threads[0], hashqueue);
    hashqueue = result.queue;
    
    Entry *head = hashqueue -> head;
    u16 table_index = head -> table_index;
    assert(hashqueue -> table[table_index] == head);            // table contains the entry
    assert(hashqueue -> occupied_index[table_index] == 1);      // occupied index reflects insertion

    ++tests_passed;
}

static void addressUnmodified(void) {
    HashQueue *original_address = hashqueue;
    QueueResultPair result = hashqueue -> enqueue(threads[0], hashqueue);
    hashqueue = result.queue;
    assert(hashqueue == original_address);

    ++tests_passed;
}

static void enqueueSuccessfulReturnValue(void) {
    QueueResultPair result = hashqueue -> enqueue(threads[0], hashqueue);
    hashqueue = result.queue;
    assert(result.result == 1);

    ++tests_passed;
}

static void queuePointersUpdatedSecondInsertion(void) {
    QueueResultPair result = hashqueue -> enqueue(threads[0], hashqueue);
    hashqueue = result.queue;

    result = hashqueue -> enqueue(threads[1], hashqueue);
    hashqueue = result.queue;
    
    assert(hashqueue -> head != hashqueue -> tail);             // tail has been updated
    assert(hashqueue -> head -> next == hashqueue -> tail);     // the head's next pointer points to the tail
    assert(hashqueue -> tail -> prev == hashqueue -> head);     // the tail's prev pointer points to the head

    ++tests_passed;
}

static void insert3(void) {
    QueueResultPair result = hashqueue -> enqueue(threads[0], hashqueue);
    hashqueue = result.queue;

    result = hashqueue -> enqueue(threads[1], hashqueue);
    hashqueue = result.queue;

    result = hashqueue -> enqueue(threads[2], hashqueue);
    hashqueue = result.queue;

    assert(hashqueue -> size == 3);
    assert(hashqueue -> load_factor == (double) 3 / 128);

    assert(hashqueue -> head -> next -> t == threads[1]);   // head next OK
    assert(hashqueue -> tail -> prev -> t == threads[1]);   // tail prev OK

    // once tail -> prev verified
    Entry *middle = hashqueue -> tail -> prev;              
    assert(middle -> prev == hashqueue -> head);            // middle elem pointers OK
    assert(middle -> next == hashqueue -> tail);

    // only valid for ID_hash
    assert(hashqueue -> head -> table_index == 0);
    assert(middle -> table_index == 1);
    assert(hashqueue -> tail -> table_index == 2);

    ++tests_passed;
}

static void linearProbing(void) {
    for (int i = 0; i <= 5; ++i) {
        hashqueue -> enqueue(overlapping_threads[i], hashqueue);
    }

    assert(hashqueue -> table[0] -> t -> id == 0);
    assert(hashqueue -> table[1] -> t -> id == 128);
    assert(hashqueue -> table[2] -> t -> id == 256);
    assert(hashqueue -> table[3] -> t -> id == 3);
    assert(hashqueue -> table[4] -> t -> id == 1);
    assert(hashqueue -> table[5] -> t -> id == 129);

    ++tests_passed;
}

/*
    Dequeue Tests
*/


static void dequeueSizesChanged(void) {
    hashqueue -> enqueue(threads[0], hashqueue);
    hashqueue -> enqueue(threads[1], hashqueue);
    hashqueue -> enqueue(threads[2], hashqueue);

    double old_load_factor = hashqueue -> load_factor;
    int old_size = hashqueue -> size;
    thread *dequeued = hashqueue -> dequeue(hashqueue);

    assert(hashqueue -> load_factor != old_load_factor);
    assert(hashqueue -> size == old_size - 1);

    ++tests_passed;
}

static void dequeueEmptyFails(void) {
    thread *dequeued = hashqueue -> dequeue(hashqueue);
    assert(dequeued == NULL);

    ++tests_passed;
}

static void dequeueCorrectElement(void) {
    hashqueue -> enqueue(threads[0], hashqueue);
    hashqueue -> enqueue(threads[1], hashqueue);
    hashqueue -> enqueue(threads[2], hashqueue);
    thread *dequeued = hashqueue -> dequeue(hashqueue);

    assert(dequeued -> id == 0);
    ++tests_passed;
}

static void dequeuePointersMended(void) {
    hashqueue -> enqueue(threads[0], hashqueue);
    hashqueue -> enqueue(threads[1], hashqueue);
    hashqueue -> enqueue(threads[2], hashqueue);
    thread *dequeued = (thread*) hashqueue -> dequeue(hashqueue);

    assert(hashqueue -> head -> t -> id == 1);                  // new head is thread 1
    assert(hashqueue -> head -> prev == NULL);                  // new head has no prev element
    assert(hashqueue -> head -> next == hashqueue -> tail);     // new head's next is the tail

    ++tests_passed;
}

static void dequeueTableMended(void) {
    hashqueue -> enqueue(threads[0], hashqueue);
    hashqueue -> enqueue(threads[1], hashqueue);
    hashqueue -> enqueue(threads[2], hashqueue);
    thread *dequeued = hashqueue -> dequeue(hashqueue);

    // Occupied index ok
    assert(hashqueue -> occupied_index[0] == 0);
    assert(hashqueue -> occupied_index[1] == 1);
    assert(hashqueue -> occupied_index[2] == 1);

    // Table OK
    assert(hashqueue -> table[0] == NULL);
    assert(hashqueue -> table[1] -> t -> id == 1);
    assert(hashqueue -> table[2] -> t -> id == 2);

    ++tests_passed;
}

/*
    Table repair procedure tests
*/


/*
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
    for (int i = 0; i <= 5; ++i) {
        hashqueue -> enqueue(overlapping_threads[i], hashqueue);
    }

    assert(hashqueue -> table[0] -> t -> id == 0);          // slot 0 occupied by thread 0

    thread *dequeued = hashqueue -> dequeue(hashqueue);

    assert(hashqueue -> table[0] -> t -> id == 128);        // id 128 moved to slot 0
    assert(hashqueue -> table[0] -> table_index == 0);
    assert(hashqueue -> occupied_index[0] == 1);

    assert(hashqueue -> table[1] -> t -> id == 256);        // id 256 moved to slot 1
    assert(hashqueue -> table[1] -> table_index == 1);
    assert(hashqueue -> occupied_index[1] == 1);

    assert(hashqueue -> table[2] -> t -> id == 1);          // id 129 moved to slot 4
    assert(hashqueue -> table[2] -> table_index == 2);
    assert(hashqueue -> occupied_index[2] == 1);

    assert(hashqueue -> table[3] -> t -> id == 3);          // id 3 did not move
    assert(hashqueue -> table[3] -> table_index == 3);
    assert(hashqueue -> occupied_index[3] == 1);

    assert(hashqueue -> table[4] -> t -> id == 129);        // id 128 moved to slot 0
    assert(hashqueue -> table[4] -> table_index == 4);
    assert(hashqueue -> occupied_index[4] == 1);

    assert(hashqueue -> table[5] == NULL);                  // slot 5 is empty
    assert(hashqueue -> occupied_index[5] == 0);

    ++tests_passed;
}

static void dequeueLoneElementQPointersAmended(void) {
    hashqueue -> enqueue(threads[0], hashqueue);
    assert(hashqueue -> head == hashqueue -> tail);

    hashqueue -> dequeue(hashqueue);

    assert(hashqueue -> head == NULL);
    assert(hashqueue -> tail == NULL);

    ++tests_passed;
}

/*
    RemoveByID tests
*/

static void removeByIDCorrectElement(void) {
    for (int i = 0; i < 10; ++i) {
        hashqueue -> enqueue(threads[i], hashqueue);
    }

    thread *removed = (thread*) hashqueue -> removeByID(5, hashqueue);
    assert(removed -> id == 5);

    ++tests_passed;
}

static void removeByIDSizeChanged(void) {
    for (int i = 0; i < 10; ++i) {
        hashqueue -> enqueue(threads[i], hashqueue);
    }

    assert(hashqueue -> size == 10);
    hashqueue -> removeByID(0, (void*) hashqueue);
    assert(hashqueue -> size == 9);

    ++tests_passed;
}

static void removeByIDLoadFactorChanged(void) {
    for (int i = 0; i < 10; ++i) {
        hashqueue -> enqueue(threads[i], hashqueue);
    }

    assert(hashqueue -> load_factor == (double) 10 / 128);
    hashqueue -> removeByID(0, (void*) hashqueue);
    assert(hashqueue -> load_factor == (double) 9 / 128);

    ++tests_passed;
}

static void removeByIDNotFound(void) {
    for (int i = 0; i < 10; ++i) {
        hashqueue -> enqueue(threads[i], hashqueue);
    }

    thread *found = hashqueue -> removeByID(42, hashqueue);
    assert(found == NULL);
    assert(hashqueue -> size == 10);

    ++tests_passed;
}

static void removeByIDIntermediatePointersMended(void) {
    // Enqueue 10 threads, ids [0..9]
    for (int i = 0; i < 10; ++i) {
        hashqueue -> enqueue(threads[i], hashqueue);
    }
    hashqueue -> removeByID(5, (void*) hashqueue);
    Entry *entry4 = hashqueue -> table[4];
    Entry *entry6 = hashqueue -> table[6];

    assert(entry4 -> next == entry6);
    assert(entry6 -> prev == entry4);

    ++tests_passed;
}

static void removeByIDIntermediateTableAmended(void) {
    // Enqueue 10 threads, ids [0..9]
    for (int i = 0; i < 10; ++i) {
        hashqueue -> enqueue(threads[i], hashqueue);
    }

    hashqueue -> removeByID(5, hashqueue);

    assert(hashqueue -> table[5] == NULL);
    assert(hashqueue -> occupied_index[5] == 0);

    ++tests_passed;
}

static void removeByIDHeadPointersAmended(void) {
    // Enqueue 10 threads, ids [0..9]
    for (int i = 0; i < 10; ++i) {
        hashqueue -> enqueue(threads[i], hashqueue);
    }
    
    assert(hashqueue -> head == hashqueue -> table[0]);
    hashqueue -> removeByID(0, hashqueue);
    assert(hashqueue -> head == hashqueue -> table[1]);
    assert(hashqueue -> table[2] -> prev == hashqueue -> head);

    ++tests_passed;
}

static void removeByIDHeadTableAmended(void) {
     // Enqueue 10 threads, ids [0..9]
    for (int i = 0; i < 10; ++i) {
        hashqueue -> enqueue(threads[i], hashqueue);
    }

    assert(hashqueue -> table[0] -> t -> id == 0);
    assert(hashqueue -> occupied_index[0] == 1);
    hashqueue -> removeByID(0, hashqueue);

    assert(hashqueue -> table[0] == NULL);
    assert(hashqueue -> occupied_index[0] == 0);

    ++tests_passed;
}

static void removeByIDTailPointersAmended(void) {
    // Enqueue 10 threads, ids [0..9]
    for (int i = 0; i < 10; ++i) {
        hashqueue -> enqueue(threads[i], hashqueue);
    }
    
    assert(hashqueue -> tail == hashqueue -> table[9]);
    hashqueue -> removeByID(9, hashqueue);
    assert(hashqueue -> tail == hashqueue -> table[8]);
    assert(hashqueue -> table[7] -> next == hashqueue -> tail);

    ++tests_passed;
}

static void removeByIDTailTableAmended(void) {
     // Enqueue 10 threads, ids [0..9]
    for (int i = 0; i < 10; ++i) {
        hashqueue -> enqueue(threads[i], hashqueue);
    }

    assert(hashqueue -> table[9] -> t -> id == 9);
    assert(hashqueue -> occupied_index[9] == 1);
    hashqueue -> removeByID(9, hashqueue);

    assert(hashqueue -> table[9] == NULL);
    assert(hashqueue -> occupied_index[9] == 0);

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
    for (int i = 0; i <= 5; ++i) {
        hashqueue -> enqueue(overlapping_threads[i], hashqueue);
    }

    hashqueue -> removeByID(128, (void*) hashqueue);

    assert(hashqueue -> table[1] -> t -> id == 256);        // id 256 moved to slot 1
    assert(hashqueue -> table[1] -> table_index == 1);
    assert(hashqueue -> occupied_index[1] == 1);

    assert(hashqueue -> table[2] -> t -> id == 1);          // id 129 moved to slot 4
    assert(hashqueue -> table[2] -> table_index == 2);
    assert(hashqueue -> occupied_index[2] == 1);

    assert(hashqueue -> table[3] -> t -> id == 3);          // id 3 did not move
    assert(hashqueue -> table[3] -> table_index == 3);
    assert(hashqueue -> occupied_index[3] == 1);

    assert(hashqueue -> table[4] -> t -> id == 129);        // id 128 moved to slot 0
    assert(hashqueue -> table[4] -> table_index == 4);
    assert(hashqueue -> occupied_index[4] == 1);

    assert(hashqueue -> table[5] == NULL);                  // slot 5 is empty
    assert(hashqueue -> occupied_index[5] == 0);

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
    for (int i = 1; i < 5; ++i) {
        hashqueue -> enqueue(test_threads[i], hashqueue);
    }

    hashqueue -> enqueue(test_threads[0], hashqueue);
    hashqueue -> enqueue(test_threads[5], hashqueue);

    assert(hashqueue -> table[0] -> t -> id == 0);
    assert(hashqueue -> table[1] -> t -> id == 636);
    assert(hashqueue -> table[124] -> t -> id == 124);
    assert(hashqueue -> table[125] -> t -> id == 252);
    assert(hashqueue -> table[126] -> t -> id == 380);
    assert(hashqueue -> table[127] -> t -> id == 508);
    
    hashqueue -> removeByID(380, hashqueue);

    /*
        508 moves into slot 126, vacated by removed thread 380
        636 moves into slot 127, (wrapped back around), vacated by 508
    */
    assert(hashqueue -> table[0] -> t -> id == 0);
    assert(hashqueue -> table[1] == NULL);
    assert(hashqueue -> table[124] -> t -> id == 124);
    assert(hashqueue -> table[125] -> t -> id == 252);
    assert(hashqueue -> table[126] -> t -> id == 508);
    assert(hashqueue -> table[127] -> t -> id == 636);

    ++tests_passed;
}


static void removeByIDLoneElement(void) {
    hashqueue -> enqueue(threads[0], hashqueue);

    hashqueue -> removeByID(0, hashqueue);
    assert(hashqueue -> head == NULL);
    assert(hashqueue -> tail == NULL);

    ++tests_passed;
}

/*
    Contains Tests
*/

static void containsTrue(void) {
    hashqueue -> enqueue(threads[3], hashqueue);
    assert(hashqueue -> contains(3, hashqueue) == 1);

    ++tests_passed;
}

static void containsFalse(void) {
    assert(hashqueue -> contains(3, hashqueue) == 0);

    ++tests_passed;
}

static void containsFalseAfterDequeue(void) {
    hashqueue -> enqueue( threads[3], hashqueue);
    hashqueue -> dequeue(hashqueue);
    assert(hashqueue -> contains(3, hashqueue) == 0);

    ++tests_passed;
}

static void containsFalseAfterRemoveByID(void) {
    hashqueue -> enqueue(threads[3], hashqueue);
    hashqueue -> removeByID(3, hashqueue);
    assert(hashqueue -> contains(3, hashqueue) == 0);

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

    for (int i = 0; i < 4; ++i) {
        hashqueue -> enqueue(test_threads[i], hashqueue);
    }

    assert(hashqueue -> table[0] -> t -> id == 253);

    hashqueue -> removeByID(126, hashqueue);

    assert(hashqueue -> contains(126, hashqueue) == 0);
    assert(hashqueue -> contains(253, hashqueue) == 1);

    assert(hashqueue -> table[126] -> t -> id == 253);

    ++tests_passed;
}


/*
    Rehash Tests
*/

static void noRehashBeforeThreshold(void) {
    // do not surpass 50%
    ThreadQueue* original_queue_address = hashqueue;

    for (int i = 0; i < 63; ++i) {
        hashqueue -> enqueue(threads[i], hashqueue);
    }

    QueueResultPair final_enqueue = hashqueue -> enqueue(threads[63], hashqueue);
    assert(original_queue_address == final_enqueue.queue);

    ++tests_passed;
}

static void addressModifiedAfterRehash(void) {
    ThreadQueue* original_queue_address = (ThreadQueue*) hashqueue;

    for (int i = 0; i < 64; ++i) {
        hashqueue -> enqueue(threads[i], hashqueue);
    }

    QueueResultPair final_enqueue = hashqueue -> enqueue(threads[64], hashqueue);
    hashqueue = final_enqueue.queue;
    assert(original_queue_address != hashqueue);

    ++tests_passed;
}

static void newTableQPointersCorrect(void) {
    ThreadQueue* original_queue_address = (ThreadQueue*) hashqueue;

    for (int i = 0; i < 64; ++i) {
        hashqueue -> enqueue(threads[i], hashqueue);
    }
    Entry *original_head = hashqueue -> head;
    Entry *next_up = hashqueue -> head -> next;
    Entry *original_tail = hashqueue -> tail; 

    QueueResultPair final_enqueue = hashqueue -> enqueue(threads[64], hashqueue);
    hashqueue = final_enqueue.queue;
    assert(original_head == hashqueue -> head);
    assert(next_up == hashqueue -> head -> next);
    assert(original_tail == hashqueue -> tail -> prev);

    ++tests_passed;
}

/*
    isEmpty tests
*/

static void isEmptyFalse(void) {
    for (int i = 0; i < 4; ++i) {
        hashqueue -> enqueue(threads[i], hashqueue);
    }

    assert(hashqueue -> isEmpty(hashqueue) == 0);
    ++tests_passed;
}

static void isEmptyFollowingDequeues(void) {
    for (int i = 0; i < 4; ++i) {
        hashqueue -> enqueue(threads[i], hashqueue);
    }

    for (int i = 0; i < 4; ++i) {
        hashqueue -> dequeue(hashqueue);
    }

    assert(hashqueue -> isEmpty(hashqueue) == 1);
    ++tests_passed;
}

static void isEmptyFollowingRemoveByIDs(void) {
    for (int i = 0; i < 4; ++i) {
        hashqueue -> enqueue(threads[i], hashqueue);
    }

    for (int i = 0; i < 4; ++i) {
        hashqueue -> removeByID(i, hashqueue);
    }

    assert(hashqueue -> isEmpty(hashqueue) == 1);
    ++tests_passed;
}

static void isEmptyPointersAgreePositive(void) {
    for (int i = 0; i < 4; ++i) {
        hashqueue -> enqueue(threads[i], hashqueue);
    }

    assert(hashqueue -> isEmpty(hashqueue) == 
        (hashqueue -> head == NULL && hashqueue -> tail == NULL));

    ++tests_passed;
}

static void isEmptyPointersAgreeNegative(void) {
    for (int i = 0; i < 4; ++i) {
        hashqueue -> enqueue(threads[i], hashqueue);
    }

    assert(hashqueue -> isEmpty(hashqueue) == 
        (hashqueue -> head == NULL && hashqueue -> tail == NULL));

    for (int i = 0; i < 4; ++i) {
        hashqueue -> removeByID(i, hashqueue);
    }

    assert(hashqueue -> isEmpty(hashqueue) == 
        (hashqueue -> head == NULL && hashqueue -> tail == NULL));

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

    // Dequeue tests
    runTest(dequeueSizesChanged);
    runTest(dequeueEmptyFails);
    runTest(dequeueCorrectElement);
    runTest(dequeuePointersMended);
    runTest(dequeueTableMended);
    runTest(dequeueTableRepairTest);
    runTest(dequeueLoneElementQPointersAmended);

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

    // isEmpty tests
    runTest(isEmptyFalse);
    runTest(isEmptyFollowingDequeues);
    runTest(isEmptyFollowingRemoveByIDs);
    runTest(isEmptyPointersAgreePositive);
    runTest(isEmptyPointersAgreeNegative);

    printf("Passed %u/%u tests.\n", tests_passed, tests_count);
    return 0; 
}