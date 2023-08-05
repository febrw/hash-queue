#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>

#include "hash-queue.h"
#include "list.h"

static struct thread *threads[65535];
static LIST_HEAD(thread_list);


static void setup() {
    for (int i = 0; i < 65535; ++i) {
        threads[i] = malloc(sizeof(struct thread));
        if (threads[i] == NULL) {
            printf("malloc failed");
            exit(0);
        } else {
            threads[i] -> id = i;
        }
    }
}

static void wrapUp() {
    for (int i = 0; i < 65535; ++i) {
        free(threads[i]);
    }
}


static void enqueueAll(void) {
    struct thread *thread;
    for (int i = 0; i < 65535; ++i) {
        thread = threads[i];
        list_add_tail(&thread->thread_list, &thread_list);
    }
}

static void removeByIDAll(void) {
    struct thread *thread;
    struct list_head *list_head;
    for (int i = 0; i < 65535; ++i) {

        list_for_each(list_head, &thread_list)
        {
            thread = list_entry(list_head, struct thread, thread_list);
            if(thread->id == i)
            {
                goto found;
            }
        }
        found:
        list_del_init(&thread->thread_list);
    }
}

static void removeByIDReversedAll(void) {
    struct thread *thread;
    struct list_head *list_head;
    for (int i = 65534; i >= 0; --i) {

        list_for_each(list_head, &thread_list)
        {
            thread = list_entry(list_head, struct thread, thread_list);
            if(thread->id == i)
            {
                goto found;
            }
        }
        found:
        list_del_init(&thread->thread_list);
    }
}


static void containsAll(void) {
    struct thread *thread;
    struct list_head *list_head;
    for (int i = 0; i < 65535; ++i) {

        list_for_each(list_head, &thread_list)
        {
            thread = list_entry(list_head, struct thread, thread_list);
            if(thread->id == i)
            {
                goto found;
            }
        }
        found:
        ;
    }
}

static void containsAllReversed(void) {
    struct thread *thread;
    struct list_head *list_head;
    for (int i = 65534; i >= 0; --i) {

        list_for_each(list_head, &thread_list)
        {
            thread = list_entry(list_head, struct thread, thread_list);
            if(thread->id == i)
            {
                goto found;
            }
        }
        found:
        ;
    }
}




static double timeFunction(void (*testFunction) (void)) {
    clock_t begin = clock();
    testFunction();
    clock_t end = clock();
    return (double) (end - begin) * 1000 / CLOCKS_PER_SEC;
}



int main(void) {
    setup();

    const double enqueue_time = timeFunction(enqueueAll);
    printf("Enqueue all time elapsed (ms): %f\n", enqueue_time);

    const double remove_time = timeFunction(removeByIDAll);
    printf("remove all time elapsed (ms): %f\n", remove_time);

    enqueueAll();
    const double remove_reverse = timeFunction(removeByIDReversedAll);
    printf("remove all by reverse order time elapsed (ms): %f\n", remove_reverse);

    enqueueAll();
    const double contains_time = timeFunction(containsAll);
    printf("contains all time elapsed (ms): %f\n", contains_time);

    const double contains_reversed = timeFunction(containsAllReversed);
    printf("contains all reversed time elapsed (ms): %f\n", contains_reversed);

    return 0;
}