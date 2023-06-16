#ifndef LINKED_HASHMAP_H
#define LINKED_HASHMAP_H


typedef unsigned short u16; // defined in types.h
typedef unsigned int u32;
typedef unsigned char u8;

#define INITIAL_CAPACITY 32
#define REHASH_THRESHOLD 0.8
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
    thread* (*dequeue) (void);
    thread* (*removeByID) (u16);
    int (*contains) (u16); // bool-ish
    int (*enqueue) (thread*); // success/failure return value
    int (*isEmpty) (void); // bool-ish
    // Linked hashmap only
    int entries_count;
    int table_size; // must be a power of 2
    double load_factor; // [0,1]
    double resize_threshold; // rehash when surpassed
    Entry * head;
    Entry * tail;
    Entry * table; // malloc table
    u8 * occupied_index; // bitfields indicating occupied / not occupied
    u16 (*getHash) (u16);
    void (*free) (LinkedHashmap*);

};


LinkedHashmap *new_LinkedHashmap();
int init_LinkedHashmap(LinkedHashmap *this); // success1/failure0 return value



#endif /* LINKED_HASHMAP_H */