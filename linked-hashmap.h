#ifndef LINKED_HASHMAP_H
#define LINKED_HASHMAP_H


typedef unsigned short uint16_t; // defined in types.h
typedef unsigned int uint32_t;

#define INITIAL_CAPACITY 16
#define FNV_OFFSET_BASIS 2166136261 // http://www.isthe.com/chongo/tech/comp/fnv/#FNV-param
#define FNV_PRIME 16777619
#define MASK_16 (((uint32_t) 1<<16) -1)
#define FNV1_32_INIT ((uint32_t) 2166136261)


typedef struct thread thread;
typedef struct Entry Entry;
typedef struct LinkedHashmap LinkedHashmap;

struct thread {
    uint16_t id;
};


struct Entry {
    uint16_t hash;
    Entry * prev;
    Entry * next;
    thread * t;
};

struct LinkedHashmap {
    // Common Queue Interface
    thread* (*dequeue) (); // how to indicate failure? special pointer value?
    void (*removeByID) (uint16_t);
    int (*contains) (uint16_t); // bool-ish
    int (*enqueue) (thread*);
    int (*isEmpty) ();
    // Linked hashmap only
    Entry * head;
    Entry * tail;
    Entry * table; // malloc table
    uint16_t (*getHash) (uint16_t);
};


LinkedHashmap *new_LinkedHashmap();
void init_LinkedHashmap(LinkedHashmap *this);


#endif /* LINKED_HASHMAP_H */