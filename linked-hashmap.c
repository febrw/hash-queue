#include "linked-hashmap.h"
#include <stdio.h>
#include <stdlib.h>

//---------------------------------------- HASH FUNCTIONS ----------------------------------------
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

//----------------------------------- CONSTRUCTORS + DESTRUCTOR -----------------------------------


LinkedHashmap *new_def_LinkedHashmap() {
    LinkedHashmap *this = malloc(sizeof(LinkedHashmap));
    if (this == NULL) {
        return NULL;
    }
    init_def_LinkedHashmap(this);
    return this;
}

/*
    - Frees the entries in a table provided they are marked as occupied
*/
static void free_table(LinkedHashmap *this) {
    // Free each entry in the hash table
    for (int i = 0; i < this -> capacity; ++i) {
        if (this -> occupied_index[i] != 0) {
            free(this -> table[i]);
        }
    }
    free(this -> table);
    free(this -> occupied_index);
    free(this);
}


/*
    Returns 0 if any malloc failed, 1 otherwise.
*/
int init_def_LinkedHashmap(LinkedHashmap *this) {
    this -> size = 0;
    this -> capacity = INITIAL_CAPACITY;
    this -> load_factor = 0.0;
    this -> rehash_threshold = REHASH_THRESHOLD;
    this -> head = NULL;
    this -> tail = NULL;
    this -> table = malloc(INITIAL_CAPACITY * sizeof(Entry*));
    if (this -> table == NULL) {
        return 0;
    }
    this -> occupied_index = malloc((INITIAL_CAPACITY) * sizeof(u8));
    if (this -> occupied_index == NULL) {
        return 0;
    }
    for (int i = 0; i < INITIAL_CAPACITY; ++i) {
        this -> occupied_index[i] = 0; // all unoccupied to begin
    }
    this -> getHash = FNV1A_Hash;
    this -> free = free_table;
    return 1;
}

/*
    - Doubles the table size
    - Copies each Entry pointer into its new table slot
    - Sets occupied fields to zero in old table so entries are not freed
    - Frees the old table

*/
int init_rehashed_LinkedHashmap(LinkedHashmap* old, LinkedHashmap* new) {
    // New table initialisation
    new -> getHash = FNV1A_Hash;
    new -> free = free_table;
    new -> capacity = old -> capacity * 2;
    new -> rehash_threshold = REHASH_THRESHOLD;
    new -> head = old -> head;
    new -> tail = old -> tail;
    new -> table = malloc((new -> capacity) * sizeof(Entry*));
    if (new -> table == NULL) {
        return 0;
    }

    new -> occupied_index = malloc((new -> capacity) * sizeof(u8));
    if (new -> occupied_index == NULL) {
        return 0;
    }

    for (int i = 0; i < new -> capacity; ++i) {
        new -> occupied_index[i] = 0;
    }

    // Rehashing Procedure
    const u16 table_mask = (new -> capacity) - 1;
    u16 table_index;
    for (int i = 0; i < old -> capacity; ++i) {
        if (old -> occupied_index[i] != 0) {
            Entry * entry = old -> table[i];

            table_index = (entry -> hash) & table_mask;
            while (new -> occupied_index[table_index] != 0) {
                table_index = (table_index + 1) & table_mask;
            }
            new -> table[table_index] = entry;
            new -> occupied_index[table_index] = 1;

            old -> occupied_index[i] = 0; // set to zero so entries will not be freed
        }
    }

    free_table(old);

    return 1;
}



//------------------------------ LINKED HASHMAP ADT IMPLEMENTATIONS ------------------------------

/*
    Returns -1 if enqueue failed, 1 if succeeded, 2 if succeeded but rehashing required.
*/
static int enqueue(thread * t, void * queue) {
    LinkedHashmap * map = (LinkedHashmap*) queue;
    const u16 thread_id = t -> id;
    const u16 table_mask = (map -> capacity) - 1;
    const u16 hash = map -> getHash(thread_id);
    u16 table_index = hash & table_mask;

    int slots_searched = 0;
    while (map -> occupied_index[table_index] != 0 && slots_searched < map -> capacity) // iterate while positions unavailable
    {
        table_index = (table_index + 1) & table_mask;
        ++slots_searched;
    }

    if (slots_searched == map -> capacity) {
        printf("Table somehow full.\n");
        return -1;
    }

    // Create new entry
    Entry * new_entry = malloc(sizeof(Entry));
    if (new_entry == NULL) {
        printf("Entry malloc failed.\n");
        return -1;
    }

    new_entry -> t = t;
    new_entry -> hash = hash;
    new_entry -> table_index = table_index;
    
    // Linked List pointers update
    if (map -> isEmpty(map)) {
        map -> tail = map -> head = new_entry;
    } else {
        new_entry -> prev = map -> tail;
        new_entry -> next = NULL;
        map -> tail -> prev = new_entry;
        map -> tail = new_entry;
    }
    

    map -> table[table_index] = new_entry;
    map -> occupied_index[table_index] = 1;
    ++ map -> size;

    map -> load_factor = (double) map -> size / map -> capacity;
    if (map -> load_factor > REHASH_THRESHOLD) {
        return 2;
    } else {
        return 1;
    }
}


static thread *dequeue(void* queue) {
    LinkedHashmap* map = (LinkedHashmap*) queue;
    
    if (map -> isEmpty(map)) {
        return NULL;
    }

    Entry *entry = map -> head;
    const u16 table_index = entry -> table_index;

    // Update map fields
    map -> occupied_index[table_index] = 0;
    map -> table[table_index] = NULL;
    -- map -> size;

    // Update Linked List pointers
    if (map -> tail == map -> head) { // we are removing lone entry in map
        map -> tail = map -> head = NULL;
    } else {        // there is at least one other entry
        Entry * next = entry -> next;
        next -> prev = NULL;
        map -> head = next;
    }

    thread *thread = entry -> t;
    free(entry);
    return thread;

}

static thread *removeByID(u16 thread_id, void* queue) {
    LinkedHashmap* map = (LinkedHashmap*) queue;
    const u16 table_mask = (map -> capacity) - 1;
    const u16 hash = map -> getHash(thread_id);
    u16 table_index = hash & table_mask;

    int slots_searched = 0;
    while (slots_searched < map -> capacity)
    {
        Entry* curr = map -> table[table_index];

        if (curr -> t -> id == thread_id) { // Found
            Entry * prev = curr -> prev;
            Entry * next = curr -> next;
            if (prev == NULL && next == NULL) { // removing lone entry in the map
                map -> head = map -> tail = NULL;
            } else if (prev == NULL) {             // we are removing the HEAD -- TEST
                next -> prev = NULL;    // remove next queue node's pointer to curr
                map -> head = next;    // update the head of the list
            } else if (next == NULL) {  // we are removing the TAIL -- TEST
                prev -> next = NULL;
                map -> tail = prev;
            } else {                    // we are removing an intermediate node
                next -> prev = prev;
                prev -> next = next;
            }

            map -> occupied_index[table_index] = 0; // mark deleted
            map -> table[table_index] = NULL;       // remove entry from table
            -- map -> size;                         // record size change
            thread* found = curr -> t;
            free(curr);                             // and free its storage
            return found;
        } else {
            table_index = (table_index + 1) & table_mask;
            ++slots_searched;
        }
    }

    return NULL;

}

static int contains(u16 thread_id, void* queue) {
    LinkedHashmap* map = (LinkedHashmap*) queue;
    const u16 table_mask = (map -> capacity) - 1;
    const u16 hash = map -> getHash(thread_id);
    u16 table_index = hash & table_mask;

    int slots_searched = 0;
    while (slots_searched < map -> capacity)
    {
        Entry* curr = map -> table[table_index];
        if (curr -> t -> id == thread_id) {
            return 1;
        } else {
            table_index = (table_index + 1) & table_mask;
            ++slots_searched;
        }
    }
    return 0;
}

static inline int isEmpty(void* queue) {
    LinkedHashmap * map = (LinkedHashmap*) queue;
    return (map -> head == NULL) && (map -> tail == NULL); // size == 0?
}

int main() {
    return 0;
}