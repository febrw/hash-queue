#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>


#include "hash-queue.h"

static ThreadQueue *threadqueue;
static Thread* threads[65535];

static void test1(void) {
    QueueResultPair result;
    for (int i = 0; i < 65535; ++i) {
        result = threadqueue -> enqueue(threads[i], threadqueue);
        threadqueue = result.queue;
    }
}

static void test2(void) {
    /*
    for (int i = 65534; i >= 0; --i) {
        threadqueue -> removeByID(i, threadqueue);
    }
    */
    for (int i = 0; i < 65535; ++i) {
        threadqueue -> removeByID(i, threadqueue);
    }
}

static void test3(void) {
    Thread *dequeued;
    for (int i = 0; i < 65535; ++i) {
        dequeued = threadqueue -> dequeue(threadqueue);
    }
}

int main() {
    threadqueue = (ThreadQueue*) new_HashQueue();

    // setup
    for (int i = 0; i < 65535; ++i) {
        threads[i] = malloc(sizeof(Thread));
        if (threads[i] == NULL) {
            printf("malloc failed");
            exit(0);
        } else {
            threads[i] -> id = i;
        }
    }


    clock_t begin = clock();
    test1();
    clock_t enqueue_clock = clock();

    double enqueue_time = (double) (enqueue_clock - begin) * 1000 / CLOCKS_PER_SEC;

    test2();
    clock_t removeByID_clock = clock();
    double removeByID_time = (double) (removeByID_clock - enqueue_clock) * 1000 / CLOCKS_PER_SEC;
    assert(threadqueue -> isEmpty(threadqueue));

    test1(); // enqueue all again
    HashQueue *hashqueue = (HashQueue*) threadqueue;

    //assert(hashqueue -> head -> table_index == 0);

    clock_t deq_begin_clock = clock();
    test3(); // dequeue all
    clock_t deq_end_clock = clock();

    double dequeue_time = (double) (deq_end_clock - deq_begin_clock) * 1000 / CLOCKS_PER_SEC;
    printf("Enqueue all time elapsed: %fms\n", enqueue_time);
    printf("Remove all by ID time elapsed: %fms\n", removeByID_time);
    printf("Dequeue all time elapsed: %fms\n", dequeue_time);

    
}