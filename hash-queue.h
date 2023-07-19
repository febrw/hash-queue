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

struct thread {
    u16 id;
};


struct Entry {
    Entry * prev;
    Entry * next;
    thread * t;         // value
    u16 table_index;    // allows dequeuing without search
};

typedef struct Iterator Iterator;
struct Iterator {
    int (*hasNext)(Iterator* this);
    thread* (*next)(Iterator* this);
    Entry* currentEntry;
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
    void (*resetIterator)(ThreadQueue* this);
};


struct HashQueue {
    // Common Queue Interface
    thread* (*dequeue) (ThreadQueue*);                     // Input: queue. Output: dequeued element
    int (*contains) (u16, ThreadQueue*);                   // success/failure return value
    QueueResultPair (*enqueue) (thread*, ThreadQueue*);    // Inputs: enqueue element, queue. Output: queue pointer, enqueue success/failure
    int (*isEmpty) (ThreadQueue*);                         // success/failure return value
    thread* (*removeByID) (u16, ThreadQueue*);               // Inputs: ID, queue. Output: removed element
    Iterator* (*iterator)(ThreadQueue* this);
 
    // Hash Queue only
    int size;
    int capacity;                                          // must be a power of 2
    double load_factor;                                    // [0,1]
    Entry * head;
    Entry * tail;
    Entry ** table;                                        // malloc table, uses double pointers to allow rehashing to maintain next and prev pointers
    void (*freeQueue) (HashQueue*);  
};
/*
thread* nextIt(Iterator* this) {
    Entry* current = this->currentEntry;
    this->currentEntry = this->currentEntry->next;
    return current->t;
}
*/
Iterator* (*iterator)(ThreadQueue* this);
    // malloc up space for itertair struct
    // set iteratir's currentEntry to be this->head
    // set the pointers to the functions inside th eiteratr stuct 
    // return pointer to the iterator


HashQueue *HashQueue_new();
int HashQueue_init(HashQueue*);
QueueResultPair HashQueue_rehash(HashQueue*); 

#endif /* HASH_QUEUE_H */