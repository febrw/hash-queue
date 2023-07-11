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
    Enqueue tests
*/
static void sizeModified(void) {
    QueueResultPair result =  hashqueue -> enqueue((void*) threads[0], (void*) hashqueue);
    assert(hashqueue -> size == 1);                 // size is 1
    assert(hashqueue -> isEmpty(hashqueue) == 0);   // queue not empty
}



int main() {
    initialise_test_vars();
    runTest(sizeModified);
    return 0; 
}