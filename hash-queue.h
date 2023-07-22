#ifndef HASH_QUEUE_H
#define HASH_QUEUE_H

typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned char u8;

#define INITIAL_CAPACITY 128
#define REHASH_THRESHOLD 0.5

typedef struct thread thread;
typedef struct Entry Entry;
typedef struct HashQueue HashQueue;
typedef struct QueueResultPair QueueResultPair;
typedef struct ThreadQueue ThreadQueue;
typedef struct Iterator Iterator;


struct thread {
    u16 id;
};

struct Entry {
    Entry *prev;
    Entry *next;
    thread *t;         // value
    u16 table_index;    // allows dequeuing without search
};

struct Iterator {
    int (*hasNext) (Iterator*);
    thread* (*next) (Iterator*);
    Entry *currentEntry;
};


/*
    Enqueue may require rehashing, changing the queue's pointer
    The new address is returned along with the enqueue result (success/failure)

    The rehash procedure will also return a QueueResultPair
*/
struct QueueResultPair {
    ThreadQueue *queue;             // in case of rehashing, new memory
    int result;                     // success/failure value
};

struct ThreadQueue {
    // Common Queue Interface
    thread* (*dequeue) (ThreadQueue*);                     // Input: queue. Output: dequeued element       
    int (*contains) (u16, ThreadQueue*);                   // success/failure return value
    QueueResultPair (*enqueue) (thread*, ThreadQueue*);    // Inputs: enqueue element, queue. Output: queue pointer, enqueue success/failure
    int (*isEmpty) (ThreadQueue*);                         // success/failure return value
    thread* (*removeByID) (u16, ThreadQueue*);             // Inputs: ID, queue. Output: removed element
    Iterator* (*iterator)(ThreadQueue*);                   // Constructs an iterator over the ThreadQueue
    int (*size) (ThreadQueue*);                            // Returns the number of elements in the ThreadQueue
    void (*freeQueue) (ThreadQueue*);
};


struct HashQueue {
    // Common Queue Interface
    thread* (*dequeue) (ThreadQueue*);                     // Input: queue. Output: dequeued element
    int (*contains) (u16, ThreadQueue*);                   // success/failure return value
    QueueResultPair (*enqueue) (thread*, ThreadQueue*);    // Inputs: enqueue element, queue. Output: queue pointer, enqueue success/failure
    int (*isEmpty) (ThreadQueue*);                         // success/failure return value
    thread* (*removeByID) (u16, ThreadQueue*);             // Inputs: ID, queue. Output: removed element
    Iterator* (*iterator)(ThreadQueue*);
    int (*size) (ThreadQueue*);                            // Returns the number of elements in the HashQueue
    void (*freeQueue) (ThreadQueue*);

    // Hash Queue only
    int _size;
    int capacity;                                          // must be a power of 2
    double load_factor;                                    // [0,1]
    Entry *head;
    Entry *tail;
    Entry **table;                                        // malloc table, uses double pointers to allow rehashing to maintain next and prev pointers
      
};


HashQueue *new_HashQueue();
int init_HashQueue(HashQueue*);
QueueResultPair HashQueue_rehash(HashQueue*); 

Iterator *new_Iterator(ThreadQueue*);

#endif /* HASH_QUEUE_H */