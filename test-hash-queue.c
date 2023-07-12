#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "hash-queue.h"


static const int tests_count = 0;
static int tests_passed = 0;

static HashQueue * hashqueue;
static thread* threads[128];

static void initialise_test_vars(void) {

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

    assert(hashqueue -> head -> next -> t == threads[1]);   // head next OK
    assert(hashqueue -> tail -> prev -> t == threads[1]);   // tail prev OK

    Entry *middle = hashqueue -> tail -> prev;
    assert(middle -> prev == hashqueue -> head);            // middle elem pointers OK
    assert(middle -> next == hashqueue -> tail);

    // only valid for ID_hash
    assert(hashqueue -> head -> table_index == 0);
    assert(middle -> table_index == 1);
    assert(hashqueue -> tail -> table_index == 2);

}

int main() {
    initialise_test_vars();
    runTest(constructionTest);

    // Enqueue first element tests
    runTest(sizeModified);
    runTest(queuePointersUpdatedFirstInsertion);
    runTest(tableUpdatedFirstInsertion);
    runTest(addressUnmodified);
    runTest(enqueueSuccessfulReturnValue);

    // Enqueue further elements tests
    runTest(queuePointersUpdatedSecondInsertion);
    runTest(insert3);

    return 0; 
}