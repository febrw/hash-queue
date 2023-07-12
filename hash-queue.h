#ifndef HASH_QUEUE_H
#define HASH_QUEUE_H


typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned char u8;

#define INITIAL_CAPACITY 128
#define REHASH_THRESHOLD 0.5
#define FNV_OFFSET_BASIS 2166136261 // http://www.isthe.com/chongo/tech/comp/fnv/#FNV-param
#define FNV_PRIME 16777619
#define MASK_16 (((u32) 1<<16) -1)
#define FNV1_32_INIT ((u32) 2166136261)


typedef struct thread thread;
typedef struct Entry Entry;
typedef struct HashQueue HashQueue;
typedef struct QueueResultPair QueueResultPair;

struct thread {
    u16 id;
};


struct Entry {
    Entry * prev;
    Entry * next;
    thread * t;         // value
    u16 hash;           // key
    u16 table_index;    // allows dequeuing without search
};

/*
    Enqueue may require rehashing, changing the queue's pointer
    The new address is returned along with the enqueue result (success/failure)

    The rehash procedure will also return a QueueResultPair
*/
struct QueueResultPair {
    void *queue;        // in case of rehashing, new memory
    int result;            // success/failure value
};

struct HashQueue {
    // Common Queue Interface
    void* (*dequeue) (void*);                       // Input: queue. Output: dequeued element       
    void* (*removeByID) (u16, void*);               // Inputs: ID, queue. Output: removed element
    int (*contains) (u16, void*);                   // success/failure return value
    QueueResultPair (*enqueue) (void*, void*);      // Inputs: enqueue element, queue. Output: queue pointer, enqueue success/failure
    int (*isEmpty) (void*);                         // success/failure return value
    // Hash Queue only
    int size;
    int capacity; // must be a power of 2
    double load_factor; // [0,1]
    double rehash_threshold; // rehash when surpassed
    Entry * head;
    Entry * tail;
    Entry ** table; // malloc table, uses double pointers to allow rehashing to maintain next and prev pointers
    u8 * occupied_index; // char array indicating slot occupations
    u16 (*getHash) (u16);
    void (*freeQueue) (HashQueue*);
};


HashQueue *HashQueue_new();
int HashQueue_init(HashQueue*);
QueueResultPair HashQueue_rehash(HashQueue*); 

#endif /* HASH_QUEUE_H */