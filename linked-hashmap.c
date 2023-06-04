#include "linked-hashmap.h"
#include <stdio.h>
#include <stdlib.h>

LinkedHashmap *new_LinkedHashmap() {
    LinkedHashmap *this = malloc(sizeof(LinkedHashmap));
    init_LinkedHashmap(this);
    return this;
}

void init_LinkedHashmap(LinkedHashmap *this) {
    this -> table = (Entry*) malloc(INITIAL_CAPACITY * sizeof(Entry));
    this -> getHash = FNV1A_Hash;
    this -> head = NULL;
    this -> tail = NULL;
}

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