#include "linked-hashmap.h"
#include <stdio.h>
#include <stdlib.h>

static u16 FNV1_Hash(u16 thread_id) {
    u32 hash = FNV_OFFSET_BASIS;
    for (int i = 0; i < 32; i += 8) {
        hash *= FNV_PRIME;
        hash ^= ((thread_id >> i) & 0xFF);
    }

    hash = (u16) (hash >> 16) ^ (hash & MASK_16);
    return hash;
}

static u16 FNV1A_Hash(u16 thread_id) {
    u32 hash = FNV_OFFSET_BASIS;
    for (int i = 0; i < 32; i += 8) {
        hash ^= ((thread_id >> i) & 0xFF);
        hash *= FNV_PRIME;      
    }

    hash = (u16) (hash >> 16) ^ (hash & MASK_16);
    return hash;
}


LinkedHashmap *new_LinkedHashmap() {
    LinkedHashmap *this = malloc(sizeof(LinkedHashmap));
    if (this == NULL) {
        return NULL;
    }
    init_LinkedHashmap(this);
    return this;
}

int init_LinkedHashmap(LinkedHashmap *this) {
    this -> entries_count = 0;
    this -> table_size = INITIAL_CAPACITY;
    this -> load_factor = 0.0;
    this -> resize_threshold = REHASH_THRESHOLD;
    this -> head = NULL;
    this -> tail = NULL;
    this -> table = malloc(INITIAL_CAPACITY * sizeof(Entry));
    if (this -> table == NULL) {
        return 0;
    }
    this -> occupied_index = malloc((INITIAL_CAPACITY >> 3) * sizeof(u8));
    if (this -> occupied_index == NULL) {
        return 0;
    }
    for (int i = 0; i < INITIAL_CAPACITY >> 3; ++i) {
        this -> occupied_index[i] = 0; // all unoccupied to begin
    }
    this -> getHash = FNV1A_Hash;
    this -> free = free_table;
    return 1;
}

static void free_table(LinkedHashmap *this) {
    free(this -> table);
    free(this);
}

// enqueue
static thread *enqueue(thread * t, LinkedHashmap * lhm) {
    const u16 thread_id = t -> id;
    u16 table_index = lhm -> getHash(thread_id) & ((lhm -> table_size) - 1);

    while (lhm -> table) {

    }

}

// dequeue
static thread *dequeue(void) {

}