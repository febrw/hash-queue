#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "hash-queue.h"


static const int tests_count = 0;
static int tests_passed = 0;

static HashQueue * hashqueue;
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

static void init_OK(void) {
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
}


/*
    Enqueue tests
*/
static void sizeModified(void) {
    QueueResultPair result = hashqueue -> enqueue((void*) threads[0], (void*) hashqueue);
    assert(hashqueue -> size == 1);                 // size is 1
    assert(hashqueue -> isEmpty(hashqueue) == 0);   // queue not empty
    assert(hashqueue -> load_factor != 0);          // load factor has changed
    assert(hashqueue -> load_factor == (double) 1 / hashqueue -> capacity);
}

static void queuePointersUpdatedFirstInsertion(void) {
    QueueResultPair result = hashqueue -> enqueue((void*) threads[0], (void*) hashqueue);
    Entry *head = hashqueue -> head;
    Entry *tail = hashqueue -> tail;
    assert(head == tail);           // head and tail are same for first insertion
    assert(head -> next == NULL);   // next and prev are NULL
    assert(head -> prev == NULL);
}

static void tableUpdatedFirstInsertion(void) {
    QueueResultPair result = hashqueue -> enqueue((void*) threads[0], (void*) hashqueue);
    Entry *head = hashqueue -> head;
    u16 table_index = head -> table_index;
    assert(hashqueue -> table[table_index] == head);            // table contains the entry
    assert(hashqueue -> occupied_index[table_index] == 1);      // occupied index reflects insertion
}

static void addressUnmodified(void) {
    HashQueue *original_address = hashqueue;
    QueueResultPair result = hashqueue -> enqueue((void*) threads[0], (void*) hashqueue);
    assert(result.queue == (original_address));
}

static void enqueueSuccessfulReturnValue(void) {
    QueueResultPair result = hashqueue -> enqueue((void*) threads[0], (void*) hashqueue);
    assert(result.result == 1);
}

static void queuePointersUpdatedSecondInsertion(void) {
    hashqueue -> enqueue((void*) threads[0], (void*) hashqueue);
    hashqueue -> enqueue((void*) threads[1], (void*) hashqueue);
    assert(hashqueue -> head != hashqueue -> tail);             // tail has been updated
    assert(hashqueue -> head -> next == hashqueue -> tail);     // the head's next pointer points to the tail
    assert(hashqueue -> tail -> prev == hashqueue -> head);     // the tail's prev pointer points to the head
}

static void insert3(void) {
    hashqueue -> enqueue((void*) threads[0], (void*) hashqueue);
    hashqueue -> enqueue((void*) threads[1], (void*) hashqueue);
    hashqueue -> enqueue((void*) threads[2], (void*) hashqueue);
    
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

}

static void linearProbing(void) {
    for (int i = 0; i <= 5; ++i) {
        hashqueue -> enqueue((void*) overlapping_threads[i],(void*) hashqueue);
    }

    assert(hashqueue -> table[0] -> t -> id == 0);
    assert(hashqueue -> table[1] -> t -> id == 128);
    assert(hashqueue -> table[2] -> t -> id == 256);
    assert(hashqueue -> table[3] -> t -> id == 3);
    assert(hashqueue -> table[4] -> t -> id == 1);
    assert(hashqueue -> table[5] -> t -> id == 129);
}

/*
    Dequeue Tests
*/


static void dequeueSizesChanged(void) {
    hashqueue -> enqueue((void*) threads[0], (void*) hashqueue);
    hashqueue -> enqueue((void*) threads[1], (void*) hashqueue);
    hashqueue -> enqueue((void*) threads[2], (void*) hashqueue);

    double old_load_factor = hashqueue -> load_factor;
    int old_size = hashqueue -> size;
    thread *dequeued = (thread*) hashqueue -> dequeue((void*) hashqueue);

    assert(hashqueue -> load_factor != old_load_factor);
    assert(hashqueue -> size == old_size - 1);
}

static void dequeueEmptyFails(void) {
    thread *dequeued = hashqueue -> dequeue((void*) hashqueue);
    assert(dequeued == NULL);
}

static void dequeueCorrectElement(void) {
    hashqueue -> enqueue((void*) threads[0], (void*) hashqueue);
    hashqueue -> enqueue((void*) threads[1], (void*) hashqueue);
    hashqueue -> enqueue((void*) threads[2], (void*) hashqueue);
    thread *dequeued = (thread*) hashqueue -> dequeue((void*) hashqueue);

    assert(dequeued -> id == 0);
}

static void dequeuePointersMended(void) {
    hashqueue -> enqueue((void*) threads[0], (void*) hashqueue);
    hashqueue -> enqueue((void*) threads[1], (void*) hashqueue);
    hashqueue -> enqueue((void*) threads[2], (void*) hashqueue);
    thread *dequeued = (thread*) hashqueue -> dequeue((void*) hashqueue);

    assert(hashqueue -> head -> t -> id == 1);                  // new head is thread 1
    assert(hashqueue -> head -> prev == NULL);                  // new head has no prev element
    assert(hashqueue -> head -> next == hashqueue -> tail);     // new head's next is the tail
}

static void dequeueTableMended(void) {
    hashqueue -> enqueue((void*) threads[0], (void*) hashqueue);
    hashqueue -> enqueue((void*) threads[1], (void*) hashqueue);
    hashqueue -> enqueue((void*) threads[2], (void*) hashqueue);
    thread *dequeued = (thread*) hashqueue -> dequeue((void*) hashqueue);

    // Occupied index ok
    assert(hashqueue -> occupied_index[0] == 0);
    assert(hashqueue -> occupied_index[1] == 1);
    assert(hashqueue -> occupied_index[2] == 1);

    // Table OK
    assert(hashqueue -> table[0] == NULL);
    assert(hashqueue -> table[1] -> t -> id == 1);
    assert(hashqueue -> table[2] -> t -> id == 2);
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
static void dequeueTableRepairTest1(void) {
    for (int i = 0; i <= 5; ++i) {
        hashqueue -> enqueue((void*) overlapping_threads[i],(void*) hashqueue);
    }

    assert(hashqueue -> table[0] -> t -> id == 0);          // slot 0 occupied by thread 0

    thread *dequeued = (thread*) hashqueue -> dequeue((void*) hashqueue);

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
}


/*
    RemoveByID tests
*/

static void removeByIDCorrectElement(void) {
    for (int i = 0; i < 10; ++i) {
        hashqueue -> enqueue((void*) threads[i], (void*) hashqueue);
    }

    thread *removed = (thread*) hashqueue -> removeByID(5, (void*) hashqueue);
    assert(removed -> id == 5);
}

static void removeByIDSizeChanged(void) {
    for (int i = 0; i < 10; ++i) {
        hashqueue -> enqueue((void*) threads[i], (void*) hashqueue);
    }

    assert(hashqueue -> size == 10);
    hashqueue -> removeByID(0, (void*) hashqueue);
    assert(hashqueue -> size == 9);
}

static void removeByIDLoadFactorChanged(void) {
    for (int i = 0; i < 10; ++i) {
        hashqueue -> enqueue((void*) threads[i], (void*) hashqueue);
    }

    assert(hashqueue -> load_factor == (double) 10 / 128);
    hashqueue -> removeByID(0, (void*) hashqueue);
    assert(hashqueue -> load_factor == (double) 9 / 128);

}

static void removeByIDNotFound(void) {
    for (int i = 0; i < 10; ++i) {
        hashqueue -> enqueue((void*) threads[i], (void*) hashqueue);
    }

    thread *found = (thread*) hashqueue -> removeByID(42, (void*) hashqueue);
    assert(found == NULL);
    assert(hashqueue -> size == 10);
}

static void removeByIDIntermediatePointersMended(void) {
    // Enqueue 10 threads, ids [0..9]
    for (int i = 0; i < 10; ++i) {
        hashqueue -> enqueue((void*) threads[i], (void*) hashqueue);
    }
    hashqueue -> removeByID(5, (void*) hashqueue);
    Entry *entry4 = hashqueue -> table[4];
    Entry *entry6 = hashqueue -> table[6];

    assert(entry4 -> next == entry6);
    assert(entry6 -> prev == entry4);
}

static void removeByIDIntermediateTableAmended(void) {
    // Enqueue 10 threads, ids [0..9]
    for (int i = 0; i < 10; ++i) {
        hashqueue -> enqueue((void*) threads[i], (void*) hashqueue);
    }

    hashqueue -> removeByID(5, (void*) hashqueue);

    assert(hashqueue -> table[5] == NULL);
    assert(hashqueue -> occupied_index[5] == 0);
}

static void removeByIDHeadPointersAmended(void) {
    // Enqueue 10 threads, ids [0..9]
    for (int i = 0; i < 10; ++i) {
        hashqueue -> enqueue((void*) threads[i], (void*) hashqueue);
    }
    
    assert(hashqueue -> head == hashqueue -> table[0]);
    hashqueue -> removeByID(0, (void*) hashqueue);
    assert(hashqueue -> head == hashqueue -> table[1]);
    assert(hashqueue -> table[2] -> prev == hashqueue -> head);
}

static void removeByIDTailPointersAmended(void) {
    // Enqueue 10 threads, ids [0..9]
    for (int i = 0; i < 10; ++i) {
        hashqueue -> enqueue((void*) threads[i], (void*) hashqueue);
    }
    
    assert(hashqueue -> tail == hashqueue -> table[9]);
    hashqueue -> removeByID(9, (void*) hashqueue);
    assert(hashqueue -> tail == hashqueue -> table[8]);
    assert(hashqueue -> table[7] -> next == hashqueue -> tail);
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
static void removeByIDTableRepairTest1(void) {
    for (int i = 0; i <= 5; ++i) {
        hashqueue -> enqueue((void*) overlapping_threads[i],(void*) hashqueue);
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

}

/*
    Contains Tests
*/

static void containsTrue(void) {
    hashqueue -> enqueue((void*) threads[3], (void*) hashqueue);
    assert(hashqueue -> contains(3, (void*) hashqueue) == 1);
}

static void containsFalse(void) {
    assert(hashqueue -> contains(3, (void*) hashqueue) == 0);
}

static void containsFalseAfterDequeue(void) {
    hashqueue -> enqueue((void*) threads[3], (void*) hashqueue);
    hashqueue -> dequeue((void*) hashqueue);
    assert(hashqueue -> contains(3, (void*) hashqueue) == 0);
}

static void containsFalseAfterRemoveByID(void) {
    hashqueue -> enqueue((void*) threads[3], (void*) hashqueue);
    hashqueue -> removeByID(3, (void*) hashqueue);
    assert(hashqueue -> contains(3, (void*) hashqueue) == 0);
}

static void noRehashBeforeThreshold(void) {
    // do not surpass 50%
    void* original_queue_address = (void*) hashqueue;

    for (int i = 0; i < 63; ++i) {
        hashqueue -> enqueue((void*) threads[i], (void*) hashqueue);
    }

    QueueResultPair final_enqueue = hashqueue -> enqueue((void*) threads[63], (void*) hashqueue);
    assert(original_queue_address == final_enqueue.queue);
}

static void addressModifiedAfterRehash(void) {
    void* original_queue_address = (void*) hashqueue;

    for (int i = 0; i < 64; ++i) {
        hashqueue -> enqueue((void*) threads[i], (void*) hashqueue);
    }

    QueueResultPair final_enqueue = hashqueue -> enqueue((void*) threads[64], (void*) hashqueue);
    //assert(original_queue_address != final_enqueue.queue);
}

int main() {
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
    runTest(linearProbing);
    runTest(queuePointersUpdatedSecondInsertion);
    runTest(insert3);

    // Dequeue tests
    runTest(dequeueSizesChanged);
    runTest(dequeueEmptyFails);
    runTest(dequeueCorrectElement);
    runTest(dequeuePointersMended);
    runTest(dequeueTableMended);
    runTest(dequeueTableRepairTest1);

    // removeByID tests
    runTest(removeByIDCorrectElement);
    runTest(removeByIDSizeChanged);
    runTest(removeByIDLoadFactorChanged);
    runTest(removeByIDNotFound);
    runTest(removeByIDIntermediatePointersMended);
    runTest(removeByIDIntermediateTableAmended);
    runTest(removeByIDHeadPointersAmended);
    runTest(removeByIDTailPointersAmended);
    runTest(removeByIDTableRepairTest1);

    // Contains tests
    runTest(containsTrue);
    runTest(containsFalse);
    runTest(containsFalseAfterDequeue);
    runTest(containsFalseAfterRemoveByID);

    // Rehashing tests
    runTest(noRehashBeforeThreshold);
    //runTest(addressModifiedAfterRehash);

    printf("All tests passsed.\n");
    return 0; 
}