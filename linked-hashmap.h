#ifndef LINKED_HASHMAP_H
#define LINKED_HASHMAP_H


typedef unsigned short uint16_t; // defined in types.h
typedef unsigned int uint32_t;

#define INITIAL_CAPACITY 32
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

//Entry *new_Entry(thread *t);
//void init_Entry(Entry *this);

struct LinkedHashmap {
    // Common Queue Interface
    thread* (*dequeue) (void);
    thread* (*removeByID) (uint16_t);
    int (*contains) (uint16_t); // bool-ish
    int (*enqueue) (thread*); // success/failure return value
    int (*isEmpty) (void); // bool-ish
    // Linked hashmap only
    int entries_count;
    int table_size;
    double load_factor;
    Entry * head;
    Entry * tail;
    Entry * table; // malloc table
    uint16_t (*get_hash) (uint16_t);
    void (*free) (LinkedHashmap*)
};


LinkedHashmap *new_LinkedHashmap();
int init_LinkedHashmap(LinkedHashmap *this); // success1/failure0 return value



#endif /* LINKED_HASHMAP_H */