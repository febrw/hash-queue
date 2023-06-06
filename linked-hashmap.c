#include "linked-hashmap.h"
#include <stdio.h>
#include <stdlib.h>

static uint16_t FNV1_Hash(uint16_t thread_id) {
    uint32_t hash = FNV_OFFSET_BASIS;
    for (int i = 0; i < 32; i += 8) {
        hash *= FNV_PRIME;
        hash ^= ((thread_id >> i) & 0xFF);
    }

    hash = (uint16_t) (hash >> 16) ^ (hash & MASK_16);
    return hash;
}

static uint16_t FNV1A_Hash(uint16_t thread_id) {
    uint32_t hash = FNV_OFFSET_BASIS;
    for (int i = 0; i < 32; i += 8) {
        hash ^= ((thread_id >> i) & 0xFF);
        hash *= FNV_PRIME;      
    }

    hash = (uint16_t) (hash >> 16) ^ (hash & MASK_16);
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
    this -> head = NULL;
    this -> tail = NULL;
    this -> table = (Entry*) malloc(INITIAL_CAPACITY * sizeof(Entry));
    if (table == NULL) {
        return 0;
    }
    this -> get_hash = FNV1A_Hash;
    this -> free = free_table;
    return 1;
}

static void free_table(LinkedHashmap *this) {
    free(this -> table);
    free(this);
}

// enqueue
static thread *insert(thread * t, LinkedHashmap * lhm) {
    uint16_t table_index;
    const uint16_t thread_id = t -> id;
    table_index = lhm -> getHash(thread_id) & ((lhm -> table_size) - 1);

}