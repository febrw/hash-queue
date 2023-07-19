#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "hash-queue.h"

static const int tests_count = 54;
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
    hashqueue = new_HashQueue();
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
    assert(hashqueue -> table != NULL);
    assert(hashqueue -> head == NULL);
    assert(hashqueue -> tail == NULL);

    ++tests_passed;
}


/*
    Enqueue tests
*/
static void sizeModified(void) {
    QueueResultPair result = hashqueue -> enqueue(threads[0], (ThreadQueue*) hashqueue);
    hashqueue = (HashQueue*) result.queue;

    assert(hashqueue -> size == 1);                 // size is 1
    assert(hashqueue -> isEmpty((ThreadQueue*) hashqueue) == 0);   // queue not empty
    assert(hashqueue -> load_factor != 0);          // load factor has changed
    assert(hashqueue -> load_factor == (double) 1 / hashqueue -> capacity);

    ++tests_passed;
}

static void queuePointersUpdatedFirstInsertion(void) {
    QueueResultPair result = hashqueue -> enqueue(threads[0],(ThreadQueue*) hashqueue);
    hashqueue = (HashQueue*) result.queue;

    Entry *head = hashqueue -> head;
    Entry *tail = hashqueue -> tail;
    assert(head == tail);           // head and tail are same for first insertion
    assert(head -> next == NULL);   // next and prev are NULL
    assert(head -> prev == NULL);

    ++tests_passed;
}

static void tableUpdatedFirstInsertion(void) {
    QueueResultPair result = hashqueue -> enqueue(threads[0], (ThreadQueue*) hashqueue);
    hashqueue = (HashQueue*) result.queue;
    
    Entry *head = hashqueue -> head;
    u16 table_index = head -> table_index;
    assert(hashqueue -> table[table_index] == head);            // table contains the entry

    ++tests_passed;
}

static void addressUnmodified(void) {
    HashQueue *original_address = hashqueue;
    QueueResultPair result = hashqueue -> enqueue(threads[0], (ThreadQueue*) hashqueue);
    hashqueue = (HashQueue*) result.queue;
    assert(hashqueue == original_address);

    ++tests_passed;
}

static void enqueueSuccessfulReturnValue(void) {
    QueueResultPair result = hashqueue -> enqueue(threads[0], (ThreadQueue*) hashqueue);
    hashqueue = (HashQueue*) result.queue;
    assert(result.result == 1);

    ++tests_passed;
}

static void queuePointersUpdatedSecondInsertion(void) {
    QueueResultPair result = hashqueue -> enqueue(threads[0], (ThreadQueue*) hashqueue);
    hashqueue = (HashQueue*) result.queue;

    result = hashqueue -> enqueue(threads[1], (ThreadQueue*) hashqueue);
    hashqueue = (HashQueue*) result.queue;
    
    assert(hashqueue -> head != hashqueue -> tail);             // tail has been updated
    assert(hashqueue -> head -> next == hashqueue -> tail);     // the head's next pointer points to the tail
    assert(hashqueue -> tail -> prev == hashqueue -> head);     // the tail's prev pointer points to the head

    ++tests_passed;
}

static void insert3(void) {
    QueueResultPair result = hashqueue -> enqueue(threads[0], (ThreadQueue*) hashqueue);
    hashqueue = (HashQueue*) result.queue;

    result = hashqueue -> enqueue(threads[1], (ThreadQueue*) hashqueue);
    hashqueue = (HashQueue*) result.queue;

    result = hashqueue -> enqueue(threads[2], (ThreadQueue*) hashqueue);
    hashqueue = (HashQueue*) result.queue;

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
        hashqueue -> enqueue(overlapping_threads[i], (ThreadQueue*) hashqueue);
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
    hashqueue -> enqueue(threads[0], (ThreadQueue*) hashqueue);
    hashqueue -> enqueue(threads[1], (ThreadQueue*) hashqueue);
    hashqueue -> enqueue(threads[2], (ThreadQueue*) hashqueue);

    double old_load_factor = hashqueue -> load_factor;
    int old_size = hashqueue -> size;
    thread *dequeued = hashqueue -> dequeue((ThreadQueue*) hashqueue);

    assert(hashqueue -> load_factor != old_load_factor);
    assert(hashqueue -> size == old_size - 1);

    ++tests_passed;
}

static void dequeueEmptyFails(void) {
    thread *dequeued = hashqueue -> dequeue((ThreadQueue*) hashqueue);
    assert(dequeued == NULL);

    ++tests_passed;
}

static void dequeueCorrectElement(void) {
    hashqueue -> enqueue(threads[0], (ThreadQueue*) hashqueue);
    hashqueue -> enqueue(threads[1], (ThreadQueue*) hashqueue);
    hashqueue -> enqueue(threads[2], (ThreadQueue*) hashqueue);
    thread *dequeued = hashqueue -> dequeue((ThreadQueue*) hashqueue);

    assert(dequeued -> id == 0);
    ++tests_passed;
}

static void dequeuePointersMended(void) {
    hashqueue -> enqueue(threads[0], (ThreadQueue*) hashqueue);
    hashqueue -> enqueue(threads[1], (ThreadQueue*) hashqueue);
    hashqueue -> enqueue(threads[2], (ThreadQueue*) hashqueue);
    thread *dequeued = (thread*) hashqueue -> dequeue((ThreadQueue*) hashqueue);

    assert(hashqueue -> head -> t -> id == 1);                  // new head is thread 1
    assert(hashqueue -> head -> prev == NULL);                  // new head has no prev element
    assert(hashqueue -> head -> next == hashqueue -> tail);     // new head's next is the tail

    ++tests_passed;
}

static void dequeueTableMended(void) {
    hashqueue -> enqueue(threads[0], (ThreadQueue*) hashqueue);
    hashqueue -> enqueue(threads[1], (ThreadQueue*) hashqueue);
    hashqueue -> enqueue(threads[2], (ThreadQueue*) hashqueue);
    thread *dequeued = hashqueue -> dequeue((ThreadQueue*) hashqueue);

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
    for (int i = 0; i <= 5; ++i) {
        hashqueue -> enqueue(overlapping_threads[i], (ThreadQueue*) hashqueue);
    }

    assert(hashqueue -> table[0] -> t -> id == 0);          // slot 0 occupied by thread 0

    thread *dequeued = hashqueue -> dequeue((ThreadQueue*) hashqueue);

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
    hashqueue -> enqueue(threads[0], (ThreadQueue*) hashqueue);
    assert(hashqueue -> head == hashqueue -> tail);

    hashqueue -> dequeue((ThreadQueue*) hashqueue);

    assert(hashqueue -> head == NULL);
    assert(hashqueue -> tail == NULL);

    ++tests_passed;
}

/*
    RemoveByID tests
*/

static void removeByIDCorrectElement(void) {
    for (int i = 0; i < 10; ++i) {
        hashqueue -> enqueue(threads[i], (ThreadQueue*) hashqueue);
    }

    thread *removed = (thread*) hashqueue -> removeByID(5, (ThreadQueue*) hashqueue);
    assert(removed -> id == 5);

    ++tests_passed;
}

static void removeByIDSizeChanged(void) {
    for (int i = 0; i < 10; ++i) {
        hashqueue -> enqueue(threads[i], (ThreadQueue*) hashqueue);
    }

    assert(hashqueue -> size == 10);
    hashqueue -> removeByID(0, (void*) (ThreadQueue*) hashqueue);
    assert(hashqueue -> size == 9);

    ++tests_passed;
}

static void removeByIDLoadFactorChanged(void) {
    for (int i = 0; i < 10; ++i) {
        hashqueue -> enqueue(threads[i], (ThreadQueue*) hashqueue);
    }

    assert(hashqueue -> load_factor == (double) 10 / 128);
    hashqueue -> removeByID(0, (ThreadQueue*) hashqueue);
    assert(hashqueue -> load_factor == (double) 9 / 128);

    ++tests_passed;
}

static void removeByIDNotFound(void) {
    for (int i = 0; i < 10; ++i) {
        hashqueue -> enqueue(threads[i], (ThreadQueue*) hashqueue);
    }

    thread *found = hashqueue -> removeByID(42, (ThreadQueue*) hashqueue);
    assert(found == NULL);
    assert(hashqueue -> size == 10);

    ++tests_passed;
}

static void removeByIDIntermediatePointersMended(void) {
    // Enqueue 10 threads, ids [0..9]
    for (int i = 0; i < 10; ++i) {
        hashqueue -> enqueue(threads[i], (ThreadQueue*) hashqueue);
    }
    hashqueue -> removeByID(5, (ThreadQueue*) hashqueue);
    Entry *entry4 = hashqueue -> table[4];
    Entry *entry6 = hashqueue -> table[6];

    assert(entry4 -> next == entry6);
    assert(entry6 -> prev == entry4);

    ++tests_passed;
}

static void removeByIDIntermediateTableAmended(void) {
    // Enqueue 10 threads, ids [0..9]
    for (int i = 0; i < 10; ++i) {
        hashqueue -> enqueue(threads[i], (ThreadQueue*) hashqueue);
    }

    hashqueue -> removeByID(5, (ThreadQueue*) hashqueue);

    assert(hashqueue -> table[5] == NULL);

    ++tests_passed;
}

static void removeByIDHeadPointersAmended(void) {
    // Enqueue 10 threads, ids [0..9]
    for (int i = 0; i < 10; ++i) {
        hashqueue -> enqueue(threads[i], (ThreadQueue*) hashqueue);
    }
    
    assert(hashqueue -> head == hashqueue -> table[0]);
    hashqueue -> removeByID(0, (ThreadQueue*) hashqueue);
    assert(hashqueue -> head == hashqueue -> table[1]);
    assert(hashqueue -> table[2] -> prev == hashqueue -> head);

    ++tests_passed;
}

static void removeByIDHeadTableAmended(void) {
     // Enqueue 10 threads, ids [0..9]
    for (int i = 0; i < 10; ++i) {
        hashqueue -> enqueue(threads[i], (ThreadQueue*) hashqueue);
    }

    assert(hashqueue -> table[0] -> t -> id == 0);
    hashqueue -> removeByID(0, (ThreadQueue*) hashqueue);

    assert(hashqueue -> table[0] == NULL);

    ++tests_passed;
}

static void removeByIDTailPointersAmended(void) {
    // Enqueue 10 threads, ids [0..9]
    for (int i = 0; i < 10; ++i) {
        hashqueue -> enqueue(threads[i], (ThreadQueue*) hashqueue);
    }
    
    assert(hashqueue -> tail == hashqueue -> table[9]);
    hashqueue -> removeByID(9, (ThreadQueue*) hashqueue);
    assert(hashqueue -> tail == hashqueue -> table[8]);
    assert(hashqueue -> table[7] -> next == hashqueue -> tail);

    ++tests_passed;
}

static void removeByIDTailTableAmended(void) {
     // Enqueue 10 threads, ids [0..9]
    for (int i = 0; i < 10; ++i) {
        hashqueue -> enqueue(threads[i], (ThreadQueue*) hashqueue);
    }

    assert(hashqueue -> table[9] -> t -> id == 9);
    hashqueue -> removeByID(9, (ThreadQueue*) hashqueue);

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
    for (int i = 0; i <= 5; ++i) {
        hashqueue -> enqueue(overlapping_threads[i], (ThreadQueue*) hashqueue);
    }

    hashqueue -> removeByID(128, (ThreadQueue*) hashqueue);

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
    for (int i = 1; i < 5; ++i) {
        hashqueue -> enqueue(test_threads[i], (ThreadQueue*) hashqueue);
    }

    hashqueue -> enqueue(test_threads[0], (ThreadQueue*) hashqueue);
    hashqueue -> enqueue(test_threads[5], (ThreadQueue*) hashqueue);

    assert(hashqueue -> table[0] -> t -> id == 0);
    assert(hashqueue -> table[1] -> t -> id == 636);
    assert(hashqueue -> table[124] -> t -> id == 124);
    assert(hashqueue -> table[125] -> t -> id == 252);
    assert(hashqueue -> table[126] -> t -> id == 380);
    assert(hashqueue -> table[127] -> t -> id == 508);
    
    hashqueue -> removeByID(380, (ThreadQueue*) hashqueue);

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
    hashqueue -> enqueue(threads[0], (ThreadQueue*) hashqueue);

    hashqueue -> removeByID(0, (ThreadQueue*) hashqueue);
    assert(hashqueue -> head == NULL);
    assert(hashqueue -> tail == NULL);

    ++tests_passed;
}

static void removeByIDDuplicateIDs(void) {
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

    for (int i = 0; i < 4; ++i) {
        hashqueue -> enqueue(test_threads[i], (ThreadQueue*) hashqueue);
    }

    assert(hashqueue -> table[0] -> t -> id == 0);
    assert(hashqueue -> table[1] -> t -> id == 1);
    assert(hashqueue -> table[2] -> t -> id == 2);
    assert(hashqueue -> table[3] -> t -> id == 0);

    hashqueue -> removeByID(0, (ThreadQueue*) hashqueue);

    assert(hashqueue -> table[0] -> t -> id == 0);
    assert(hashqueue -> table[1] -> t -> id == 1);
    assert(hashqueue -> table[2] -> t -> id == 2);
    assert(hashqueue -> table[3] == NULL);

    ++tests_passed;
}

/*
    Contains Tests
*/

static void containsTrue(void) {
    hashqueue -> enqueue(threads[3], (ThreadQueue*) hashqueue);
    assert(hashqueue -> contains(3, (ThreadQueue*) hashqueue) == 1);

    ++tests_passed;
}

static void containsFalse(void) {
    assert(hashqueue -> contains(3, (ThreadQueue*) hashqueue) == 0);

    ++tests_passed;
}

static void containsFalseAfterDequeue(void) {
    hashqueue -> enqueue(threads[3], (ThreadQueue*) hashqueue);
    hashqueue -> dequeue((ThreadQueue*) hashqueue);
    assert(hashqueue -> contains(3, (ThreadQueue*) hashqueue) == 0);

    ++tests_passed;
}

static void containsFalseAfterRemoveByID(void) {
    hashqueue -> enqueue(threads[3], (ThreadQueue*) hashqueue);
    hashqueue -> removeByID(3, (ThreadQueue*) hashqueue);
    assert(hashqueue -> contains(3, (ThreadQueue*) hashqueue) == 0);

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
        hashqueue -> enqueue(test_threads[i], (ThreadQueue*) hashqueue);
    }

    assert(hashqueue -> table[0] -> t -> id == 253);

    hashqueue -> removeByID(126, (ThreadQueue*) hashqueue);

    assert(hashqueue -> contains(126, (ThreadQueue*) hashqueue) == 0);
    assert(hashqueue -> contains(253, (ThreadQueue*) hashqueue) == 1);

    assert(hashqueue -> table[126] -> t -> id == 253);

    ++tests_passed;
}


/*
    Rehash Tests
*/

static void noRehashBeforeThreshold(void) {
    // do not surpass 50%
    ThreadQueue* original_queue_address = (ThreadQueue*) hashqueue;

    for (int i = 0; i < 63; ++i) {
        hashqueue -> enqueue(threads[i], (ThreadQueue*) hashqueue);
    }

    QueueResultPair final_enqueue = hashqueue -> enqueue(threads[63], (ThreadQueue*) hashqueue);
    assert(original_queue_address == final_enqueue.queue);

    ++tests_passed;
}

static void addressModifiedAfterRehash(void) {
    HashQueue* original_queue_address = hashqueue;

    for (int i = 0; i < 64; ++i) {
        hashqueue -> enqueue(threads[i], (ThreadQueue*) hashqueue);
    }

    QueueResultPair final_enqueue = hashqueue -> enqueue(threads[64], (ThreadQueue*) hashqueue);
    hashqueue = (HashQueue*) final_enqueue.queue;
    assert(original_queue_address != hashqueue);

    ++tests_passed;
}

static void newTableQPointersCorrect(void) {
    ThreadQueue* original_queue_address = (ThreadQueue*) hashqueue;

    for (int i = 0; i < 64; ++i) {
        hashqueue -> enqueue(threads[i], (ThreadQueue*) hashqueue);
    }
    Entry *original_head = hashqueue -> head;
    Entry *next_up = hashqueue -> head -> next;
    Entry *original_tail = hashqueue -> tail; 

    QueueResultPair final_enqueue = hashqueue -> enqueue(threads[64], (ThreadQueue*) hashqueue);
    hashqueue = (HashQueue*) final_enqueue.queue;
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
        result = hashqueue -> enqueue(test_threads[i], (ThreadQueue*) hashqueue);
        hashqueue = (HashQueue*) result.queue;
    }

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
        result = hashqueue -> enqueue(test_threads[i], (ThreadQueue*) hashqueue);
        hashqueue = (HashQueue*) result.queue;
    }

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
        result = hashqueue -> enqueue(test_threads[i], (ThreadQueue*) hashqueue);
        hashqueue = (HashQueue*) result.queue;
    }

    int old_size = hashqueue -> size;
    int old_capacity = hashqueue -> capacity;
    double old_load_factor = hashqueue -> load_factor;

    // Add 1 more element, forcing rehashing
    result = hashqueue -> enqueue(test_threads[64], (ThreadQueue*) hashqueue);
    hashqueue = (HashQueue*) result.queue;

    int new_size = hashqueue -> size;
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
        result = hashqueue -> enqueue(test_threads[i], (ThreadQueue*) hashqueue);
        hashqueue = (HashQueue*) result.queue;
    }

    u16 original_ids[64];
    Entry *curr = hashqueue -> head;
    
    // store old ids by following next pointers
    for (int i = 0; curr != NULL; ++i, curr = curr -> next) {
        original_ids[i] = curr -> t -> id;
    }

    // Add 1 more element, forcing rehashing
    result = hashqueue -> enqueue(test_threads[64], (ThreadQueue*) hashqueue);
    hashqueue = (HashQueue*) result.queue;

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
    assert(hashqueue -> isEmpty((ThreadQueue*) hashqueue) == 1);
    ++tests_passed;
}


static void isEmptyFalse(void) {
    for (int i = 0; i < 4; ++i) {
        hashqueue -> enqueue(threads[i], (ThreadQueue*) hashqueue);
    }

    assert(hashqueue -> isEmpty((ThreadQueue*) hashqueue) == 0);
    ++tests_passed;
}

static void isEmptyFollowingDequeues(void) {
    for (int i = 0; i < 4; ++i) {
        hashqueue -> enqueue(threads[i], (ThreadQueue*) hashqueue);
    }

    for (int i = 0; i < 4; ++i) {
        hashqueue -> dequeue((ThreadQueue*) hashqueue);
    }

    assert(hashqueue -> isEmpty((ThreadQueue*) hashqueue) == 1);
    ++tests_passed;
}

static void isEmptyFollowingRemoveByIDs(void) {
    for (int i = 0; i < 4; ++i) {
        hashqueue -> enqueue(threads[i], (ThreadQueue*) hashqueue);
    }

    for (int i = 0; i < 4; ++i) {
        hashqueue -> removeByID(i, (ThreadQueue*) hashqueue);
    }

    assert(hashqueue -> isEmpty((ThreadQueue*) hashqueue) == 1);
    ++tests_passed;
}

static void isEmptyPointersAgreePositive(void) {
    for (int i = 0; i < 4; ++i) {
        hashqueue -> enqueue(threads[i], (ThreadQueue*) hashqueue);
    }

    assert(hashqueue -> isEmpty((ThreadQueue*) hashqueue) == 
        (hashqueue -> head == NULL && hashqueue -> tail == NULL));

    ++tests_passed;
}

static void isEmptyPointersAgreeNegative(void) {
    for (int i = 0; i < 4; ++i) {
        hashqueue -> enqueue(threads[i], (ThreadQueue*) hashqueue);
    }

    assert(hashqueue -> isEmpty((ThreadQueue*) hashqueue) == 
        (hashqueue -> head == NULL && hashqueue -> tail == NULL));

    for (int i = 0; i < 4; ++i) {
        hashqueue -> removeByID(i, (ThreadQueue*) hashqueue);
    }

    assert(hashqueue -> isEmpty((ThreadQueue*) hashqueue) == 
        (hashqueue -> head == NULL && hashqueue -> tail == NULL));

    ++tests_passed;
}



static void ohNoBadBad1(void) {
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

    for (int i = 0; i < 3; ++i) {
        hashqueue -> enqueue(test_threads[i], (ThreadQueue*) hashqueue);
    }

    // Before removeByID(0)
    assert(hashqueue -> table[0] -> t -> id == 0);
    assert(hashqueue -> table[1] -> t -> id == 1);
    assert(hashqueue -> table[2] -> t -> id == 129);

    hashqueue -> removeByID(0, (ThreadQueue*) hashqueue);

    // After removeByID(126)
    assert(hashqueue -> table[0] -> t -> id == 129);
    assert(hashqueue -> table[1] -> t -> id == 1);
    assert(hashqueue -> table[2] == NULL);
    printf("bad test1 ran.\n");
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

    for (int i = 0; i < 3; ++i) {
        hashqueue -> enqueue(test_threads[i], (ThreadQueue*) hashqueue);
    }

    // Before removeByID(0)
    assert(hashqueue -> table[0] -> t -> id == 0);
    assert(hashqueue -> table[1] -> t -> id == 1);
    assert(hashqueue -> table[2] -> t -> id == 129);

    hashqueue -> removeByID(0, (ThreadQueue*) hashqueue);

    // After removeByID(126)
    assert(hashqueue -> table[0] == NULL);
    assert(hashqueue -> table[1] -> t -> id == 1);
    assert(hashqueue -> table[2] -> t -> id ==  129);
    
    ++tests_passed;
}

static void ohNoBadBad2(void) {
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

    for (int i = 0; i < 3; ++i) {
        hashqueue -> enqueue(test_threads[i], (ThreadQueue*) hashqueue);
    }

    // Before removeByID(0)
    assert(hashqueue -> table[0] -> t -> id == 0);
    assert(hashqueue -> table[1] -> t -> id == 128);
    assert(hashqueue -> table[127] -> t -> id == 127);

    hashqueue -> removeByID(127, (ThreadQueue*) hashqueue);

    // After removeByID(0)
    assert(hashqueue -> table[0] -> t -> id == 0);
    assert(hashqueue -> table[1] == NULL);
    assert(hashqueue -> table[127] -> t -> id == 128);
    printf("bad test2 ran.\n");
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

    for (int i = 0; i < 3; ++i) {
        hashqueue -> enqueue(test_threads[i], (ThreadQueue*) hashqueue);
    }

    // Before removeByID(0)
    assert(hashqueue -> table[0] -> t -> id == 0);
    assert(hashqueue -> table[1] -> t -> id == 128);
    assert(hashqueue -> table[127] -> t -> id == 127);

    hashqueue -> removeByID(127, (ThreadQueue*) hashqueue);

    // After removeByID(0)
    assert(hashqueue -> table[0] -> t -> id == 0);
    assert(hashqueue -> table[1] -> t -> id == 128);
    assert(hashqueue -> table[127] == NULL);

    ++tests_passed;
}



static void ohNoBadBad3(void) {
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

    for (int i = 0; i < 3; ++i) {
        hashqueue -> enqueue(test_threads[i], (ThreadQueue*) hashqueue);
    }

    // Before removeByID(126)
    assert(hashqueue -> table[0] -> t -> id == 255);
    assert(hashqueue -> table[126] -> t -> id == 126);
    assert(hashqueue -> table[127] -> t -> id == 127);

    hashqueue -> removeByID(126, (ThreadQueue*) hashqueue);

    // After removeByID(126)
    assert(hashqueue -> table[0] == NULL);
    assert(hashqueue -> table[126] -> t -> id == 255);
    assert(hashqueue -> table[127] -> t -> id == 127);

    printf("bad test3 ran.\n");

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

    for (int i = 0; i < 3; ++i) {
        hashqueue -> enqueue(test_threads[i], (ThreadQueue*) hashqueue);
    }

    // Before removeByID(126)
    assert(hashqueue -> table[0] -> t -> id == 255);
    assert(hashqueue -> table[126] -> t -> id == 126);
    assert(hashqueue -> table[127] -> t -> id == 127);

    hashqueue -> removeByID(126, (ThreadQueue*) hashqueue);

    // After removeByID(126)
    assert(hashqueue -> table[0] -> t -> id == 255);
    assert(hashqueue -> table[126] == NULL);
    assert(hashqueue -> table[127] -> t -> id == 127);

    ++tests_passed;
}

static void constructIteratorTest(void) {
    Iterator *iterator = new_Iterator((ThreadQueue*) hashqueue);
    assert(iterator != NULL);

    ++tests_passed;
}

static void iteratorHasNextPositive(void) {
    hashqueue -> enqueue(threads[0], (ThreadQueue*) hashqueue);
    Iterator *iterator = new_Iterator((ThreadQueue*) hashqueue);

    assert(iterator -> hasNext(iterator) != 0);

    ++tests_passed;
}

static void iteratorHasNextNegative(void) {
    Iterator *iterator = new_Iterator((ThreadQueue*) hashqueue);
    assert(iterator -> hasNext(iterator) == 0);

    ++tests_passed;
}

static void iteratorExampleUsage(void) {
    for (int i = 0; i < 8; ++i) {
        hashqueue -> enqueue(threads[i], (ThreadQueue*) hashqueue);
    }

    Iterator *it = new_Iterator((ThreadQueue*) hashqueue);
    int current_thread_id;

    int ideal_thread_id = 0;
    while (it -> hasNext(it)) {
        current_thread_id = (it -> next(it)) -> id;
        assert(ideal_thread_id == current_thread_id);
        ++ideal_thread_id;
    }

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
    runTest(removeByIDDuplicateIDs);

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


    //runTest(ohNoBadBad1);
    //runTest(ohNoBadBad2);
    //runTest(ohNoBadBad3);
    
    // Table repair tests
    runTest(tableRepairNoMoveTest1);
    runTest(tableRepairNoMoveTest2);
    runTest(tableRepairNoMoveTest3);

    // Iterator tests
    runTest(constructIteratorTest);
    runTest(iteratorHasNextPositive);
    runTest(iteratorHasNextNegative);
    runTest(iteratorExampleUsage);

    printf("Passed %u/%u tests.\n", tests_passed, tests_count);
    return 0; 
}