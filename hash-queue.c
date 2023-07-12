#include "hash-queue.h"
#include <stdio.h>
#include <stdlib.h>

//---------------------------------------- HASH FUNCTIONS ----------------------------------------
// NYI
static u16 FNV1_Hash(u16 thread_id) {
    u32 hash = FNV_OFFSET_BASIS;
    for (int i = 0; i < 32; i += 8) {
        hash *= FNV_PRIME;
        hash ^= ((thread_id >> i) & 0xFF);
    }

    hash = (u16) (hash >> 16) ^ (hash & MASK_16);
    return hash;
}
// NYI
static u16 FNV1A_Hash(u16 thread_id) {
    u32 hash = FNV_OFFSET_BASIS;
    for (int i = 0; i < 32; i += 8) {
        hash ^= ((thread_id >> i) & 0xFF);
        hash *= FNV_PRIME;      
    }

    hash = (u16) (hash >> 16) ^ (hash & MASK_16);
    return hash;
}

static inline u16 ID_Hash(u16 thread_id) {
    return thread_id;
}



//------------------------------ LINKED HASHMAP ADT IMPLEMENTATIONS ------------------------------

/*
    Returns -1 if enqueue failed, 1 if succeeded, 2 if succeeded but rehashing required.
*/
static QueueResultPair HashQueue_enqueue(void * thread_ptr, void * queue) {

    thread * t = (thread*) thread_ptr;
    HashQueue *hash_queue = (HashQueue*) queue;
    const u16 thread_id = t -> id;
    const u16 table_mask = (hash_queue -> capacity) - 1;
    const u16 hash = hash_queue -> getHash(thread_id);
    u16 table_index = hash & table_mask;

    int slots_searched = 0;
    while (hash_queue -> occupied_index[table_index] != 0 && slots_searched < hash_queue -> capacity) // iterate while positions unavailable
    {
        table_index = (table_index + 1) & table_mask;
        ++slots_searched;
    }

    if (slots_searched == hash_queue -> capacity) {
        printf("Table somehow full.\n");
        QueueResultPair result = {queue, 0};        // return old queue
        return result;
    }


    // Create new entry
    Entry * new_entry = malloc(sizeof(Entry));
    if (new_entry == NULL) {
        printf("Entry memory allocation failed.\n");
        QueueResultPair result = {queue, 0};        // return old queue
        return result;
    }

    new_entry -> prev = NULL;
    new_entry -> next = NULL;
    new_entry -> t = t;
    new_entry -> hash = hash;
    new_entry -> table_index = table_index;
    
    // Linked List pointers update
    if (hash_queue -> isEmpty(hash_queue)) {
        hash_queue -> tail = hash_queue -> head = new_entry;
    } else {
        new_entry -> prev = hash_queue -> tail;
        new_entry -> next = NULL;
        hash_queue -> tail -> next = new_entry;
        hash_queue -> tail = new_entry;
    }
    

    hash_queue -> table[table_index] = new_entry;
    hash_queue -> occupied_index[table_index] = 1;
    ++ hash_queue -> size;

    hash_queue -> load_factor = (double) hash_queue -> size / hash_queue -> capacity;
    QueueResultPair result;

    // Check if rehashing required
    if (hash_queue -> load_factor > REHASH_THRESHOLD) {
        result = HashQueue_rehash(hash_queue);
    } else {
        result.queue = (void*) hash_queue;
        result.result = 1;
    }
    
    return result;
}

/*
    - Once a cell is deleted, continue iterating to 'repair' any out of place entries, or until an empty cell is found
*/
static void *HashQueue_dequeue(void* queue) {
    HashQueue* map = (HashQueue*) queue;
    
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
    if (map -> tail == map -> head) {   // we are removing lone entry in map
        map -> tail = NULL;
        map -> head = NULL;
    } else {                            // there is at least one other entry
        Entry * next = entry -> next;
        next -> prev = NULL;
        map -> head = next;
    }
    
    /*  Table Repair procedure
        - Continue forward search until empty found
        - Check each entry's ideal index.
        - If it is less than the index we emptied, move this entry into the empty index
        - continue until we find an empty slot
    */
    const u16 table_mask = (map -> capacity) - 1;
    u16 empty_index = table_index;                                          // stores the empty slot which we want to fill with an 'out of place' entry
    u16 inspect_index = (empty_index + 1) & table_mask;                     // we inspect the following index
    u16 ideal_index;                                                        // will store where an entry would ideally be placed

    while (map -> occupied_index[inspect_index] != 0) {
        ideal_index = (map -> table[inspect_index] -> hash) & table_mask;   // stores the index would the current entry ideally go

        if (ideal_index != inspect_index) {                                 // current entry is 'out of place'
            map -> table[empty_index] = map -> table[inspect_index];        // store the out of place entry into the empty slot
            map -> table[inspect_index] = NULL;                             // empty the inspect index, as we have moved the entry
            map -> occupied_index[empty_index] = 1;
            map -> occupied_index[inspect_index] = 0;                       // record movement in occupation indexes

            empty_index = inspect_index;                                    
        } 
        
        inspect_index = (inspect_index + 1) & table_mask;                   // move on to inspect next slot
        
    }

    thread *thread = entry -> t;
    free(entry);
    return (void*) thread;

}

static void *HashQueue_removeByID(u16 thread_id, void* queue) {
    HashQueue* map = (HashQueue*) queue;
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
            return (void*) found;
        } else {
            table_index = (table_index + 1) & table_mask;
            ++slots_searched;
        }
    }

    return NULL;

}

static int HashQueue_contains(u16 thread_id, void* queue) {
    HashQueue* map = (HashQueue*) queue;
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

static inline int HashQueue_isEmpty(void* queue) {
    HashQueue * map = (HashQueue*) queue;
    return (map -> head == NULL) && (map -> tail == NULL); // size == 0?
}

//----------------------------------- CONSTRUCTORS + DESTRUCTOR -----------------------------------

/*
    - Frees the entries in a table provided they are marked as occupied
*/
static void HashQueue_free(HashQueue *this) {
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
int HashQueue_init(HashQueue *this) {
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
    this -> getHash = ID_Hash;
    this -> freeQueue = HashQueue_free;
    this -> dequeue = HashQueue_dequeue;
    this -> enqueue = HashQueue_enqueue;
    this -> contains = HashQueue_contains;
    this -> isEmpty = HashQueue_isEmpty;
    return 1;
}

/*
    - Allocates Memory for the HashQueue, then populates with HashQueue_init
*/
HashQueue *HashQueue_new() {
    HashQueue *this = malloc(sizeof(HashQueue));
    if (this == NULL) {
        return NULL;
    }
    if (HashQueue_init(this) == 0) { // if some memory allocoation failed during construction
        return NULL;
    } else {
        return this;
    }
}


/*
    - Doubles the table size
    - Copies each Entry pointer into its new table slot
    - Sets occupied fields to zero in old table so entries are not freed
    - Frees the old table
*/
QueueResultPair HashQueue_rehash(HashQueue* old_queue) {
    // New table initialisation
    HashQueue * new_queue = malloc(sizeof(HashQueue));

    if (new_queue == NULL) {    // If memory allocation failed return the old queue
        QueueResultPair result = {(void*) old_queue, 0};
        return result;
    }

    new_queue -> size = old_queue -> size;
    new_queue -> capacity = (old_queue -> capacity) * 2;
    new_queue -> load_factor = (double) new_queue -> size / new_queue -> capacity;
    new_queue -> rehash_threshold = REHASH_THRESHOLD;

    new_queue -> head = old_queue -> head;
    new_queue -> tail = old_queue -> tail;
    new_queue -> table = malloc((new_queue -> capacity) * sizeof(Entry*));
    if (new_queue -> table == NULL) {
        // returns the old queue as malloc failed
        QueueResultPair result = {(void*) old_queue, 0};
    }

    new_queue -> occupied_index = malloc((new_queue -> capacity) * sizeof(u8));
    if (new_queue -> occupied_index == NULL) {
        QueueResultPair result = {(void*) old_queue, 0};
        return result;
    }

    new_queue -> getHash = ID_Hash;
    new_queue -> freeQueue = HashQueue_free;

    // Set occupied indices to 0 to begin
    for (int i = 0; i < new_queue -> capacity; ++i) {
        new_queue -> occupied_index[i] = 0;
    }

    // Rehashing Procedure
    const u16 table_mask = (new_queue -> capacity) - 1;
    u16 table_index;
    for (int i = 0; i < old_queue -> capacity; ++i) {
        if (old_queue -> occupied_index[i] != 0) {
            Entry * entry = old_queue -> table[i];

            table_index = (entry -> hash) & table_mask;
            while (new_queue -> occupied_index[table_index] != 0) {
                table_index = (table_index + 1) & table_mask;
            }
            new_queue -> table[table_index] = entry;
            new_queue -> occupied_index[table_index] = 1;

            old_queue -> occupied_index[i] = 0; // set to zero so entries will not be freed
        }
    }

    HashQueue_free(old_queue);

    QueueResultPair result = {(void*) new_queue, 1};
    return result;
}
