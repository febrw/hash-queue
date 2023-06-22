#ifndef LINKED_HASHMAP_H
#define LINKED_HASHMAP_H


typedef unsigned short u16; // defined in types.h
typedef unsigned int u32;
typedef unsigned char u8;

#define INITIAL_CAPACITY 64
#define REHASH_THRESHOLD 0.75
#define FNV_OFFSET_BASIS 2166136261 // http://www.isthe.com/chongo/tech/comp/fnv/#FNV-param
#define FNV_PRIME 16777619
#define MASK_16 (((u32) 1<<16) -1)
#define FNV1_32_INIT ((u32) 2166136261)


typedef struct thread thread;
typedef struct Entry Entry;
typedef struct LinkedHashmap LinkedHashmap;

struct thread {
    u16 id;
};


struct Entry {
    Entry * prev;
    Entry * next;
    thread * t; // value
    u16 hash; // key
};



struct LinkedHashmap {
    // Common Queue Interface
    thread* (*dequeue) (void*);
    thread* (*removeByID) (u16, void*);
    thread* (*contains) (u16, void*); // NULL if does not contain
    int (*enqueue) (thread*, void*); // success/failure return value
    int (*isEmpty) (void*); // bool-ish
    // Linked hashmap only
    int size;
    int capacity; // must be a power of 2
    double load_factor; // [0,1]
    double resize_threshold; // rehash when surpassed
    Entry * head;
    Entry * tail;
    Entry ** table; // malloc table, uses double pointers to allow rehashing to maintain next and prev pointers
    u8 * occupied_index; // char array indicating slot occupations
    u16 (*getHash) (u16);
    void (*free) (LinkedHashmap*);
};


LinkedHashmap *new_def_LinkedHashmap();
LinkedHashmap *new_rehashed_LinkedHashmap(LinkedHashmap *);
int init_def_LinkedHashmap(LinkedHashmap *); // success1/failure0 return value
int init_rehashed_LinkedHashmap(LinkedHashmap*, LinkedHashmap*); 


#endif /* LINKED_HASHMAP_H */