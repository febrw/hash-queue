#include <stdio.h>
#include <stdlib.h>

#include "hash-queue.h"

//------------------------------ Hash Functions ---------------------------------------------

u16 IDHash(u16 data) {
    return data;
}

u16 FNV1AHash(u16 data) {
    u32 hash = FNV_32_OFFSET_BASIS;
    hash ^= (FIRST_OCTET_MASK) & data;      // XOR with first octet
    hash *= FNV_32_PRIME;                   // Multiply by FNV32 Prime

    hash ^= (SECOND_OCTET_MASK) & data;     // XOR with second octet
    hash *= FNV_32_PRIME;                   // Multiply by FNV32 prime

    return (hash >> 16) ^ (hash & 0xffff); // XOR Fold the hash before returning
}

//------------------------------ HashQueue ADT IMPLEMENTATIONS ------------------------------

/*
    Returns -1 if enqueue failed, 1 if succeeded, 2 if succeeded but rehashing required.
*/
static QueueResultPair HashQueue_enqueue(Thread *t, ThreadQueue *queue) {

    HashQueue *hashqueue = (HashQueue*) queue;
    const u16 thread_id = t -> id;
    const u16 table_mask = (hashqueue -> capacity) - 1;
    u16 table_index = hashqueue -> getHash(thread_id) & table_mask;

    while (hashqueue -> table[table_index] != NULL) // iterate while positions unavailable
    {
        table_index = (table_index + 1) & table_mask;
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
    new_entry -> table_index = table_index;
    
    // Linked List pointers update
    if (hashqueue -> isEmpty((ThreadQueue*) hashqueue)) {
        hashqueue -> tail = hashqueue -> head = new_entry;
    } else {
        new_entry -> prev = hashqueue -> tail;
        new_entry -> next = NULL;
        hashqueue -> tail -> next = new_entry;
        hashqueue -> tail = new_entry;
    }
    

    hashqueue -> table[table_index] = new_entry;
    ++ hashqueue -> _size;

    hashqueue -> load_factor = (double) hashqueue -> _size / hashqueue -> capacity;
    QueueResultPair result;

    // Check if rehashing required
    if (hashqueue -> load_factor > REHASH_THRESHOLD) {
        result = HashQueue_rehash(hashqueue);
    } else {
        result.queue = (ThreadQueue*) hashqueue;
        result.result = 1;
    }

    return result;
}

/*  
    Table Repair procedure
        - Continue forward search until empty found
        - Check each entry's ideal index.
        - If it is less than the index we emptied, move this entry into the empty index
        - continue until we find an empty slot
*/

static void HashQueue_tableRepair(u16 empty_index, HashQueue *hashqueue) {
    const u16 table_mask = (hashqueue -> capacity) - 1;
    u16 inspect_index = (empty_index + 1) & table_mask;                     // we inspect the following index
    u16 thread_id;
    u16 ideal_index;                                                        // will store where an entry would ideally be placed

    while (hashqueue -> table[inspect_index] != NULL) {
        thread_id = hashqueue -> table[inspect_index] -> t -> id;
        ideal_index = hashqueue -> getHash(thread_id) & table_mask;

        /* 
            current entry is 'out of place'
            or should not be moved backwards
        */
        if (!(ideal_index == inspect_index ||
            (empty_index < ideal_index && ideal_index < inspect_index) ||
            (ideal_index < inspect_index && inspect_index < empty_index) ||
            (inspect_index < empty_index && empty_index < ideal_index)))
        {                                         
            hashqueue -> table[empty_index] = hashqueue -> table[inspect_index];    // store the out of place entry into the empty slot
            hashqueue -> table[empty_index] -> table_index = empty_index;           // reflect new position inside the Entry
            hashqueue -> table[inspect_index] = NULL;                               // empty the inspect index, as we have moved the entry

            empty_index = inspect_index;                                    
        } 
        
        inspect_index = (inspect_index + 1) & table_mask;                           // move on to inspect next slot    
    }
}

/*
    - Once a cell is deleted, continue iterating to 'repair' any out of place entries, or until an empty cell is found
*/
static Thread *HashQueue_dequeue(ThreadQueue *queue) {
    HashQueue* hashqueue = (HashQueue*) queue;
    
    if (hashqueue -> isEmpty((ThreadQueue*) hashqueue)) {
        return NULL;
    }

    Entry *entry = hashqueue -> head;
    const u16 table_index = entry -> table_index;   // attained directly without search

    // Update hashqueue fields
    hashqueue -> table[table_index] = NULL;
    -- hashqueue -> _size;
    hashqueue -> load_factor = (double) hashqueue -> _size / hashqueue -> capacity;
    

    // Update Linked List pointers
    if (hashqueue -> tail == hashqueue -> head) {   // we are removing lone entry in hashqueue
        hashqueue -> tail = NULL;
        hashqueue -> head = NULL;
    } else {                            // there is at least one other entry
        Entry *next = entry -> next;
        next -> prev = NULL;
        hashqueue -> head = next;
    }

    HashQueue_tableRepair(table_index, hashqueue);

    Thread *th = entry -> t;
    free(entry);
    return th;

}


static Thread *HashQueue_removeByID(u16 thread_id, ThreadQueue *queue) {
    HashQueue *hashqueue = (HashQueue*) queue;
    const u16 table_mask = (hashqueue -> capacity) - 1;
    u16 inspect_index = hashqueue -> getHash(thread_id) & table_mask;

    while (hashqueue -> table[inspect_index] != NULL)
    {
        Entry *curr = hashqueue -> table[inspect_index];

        if (curr -> t -> id == thread_id) { // Found
            // Linked List pointers update
            Entry * prev = curr -> prev;
            Entry * next = curr -> next;
            if (prev == NULL && next == NULL) { // removing lone entry in the hashqueue
                hashqueue -> head = hashqueue -> tail = NULL;
            } else if (prev == NULL) {          // we are removing the head
                next -> prev = NULL;    
                hashqueue -> head = next;    
            } else if (next == NULL) {          // we are removing the tail
                prev -> next = NULL;
                hashqueue -> tail = prev;
            } else {                            // we are removing an intermediate node
                next -> prev = prev;
                prev -> next = next;
            }
            // Table fields update
            hashqueue -> table[inspect_index] = NULL;       // remove entry from table
            -- hashqueue -> _size;                           // record _size change
            hashqueue -> load_factor = (double) hashqueue -> _size / hashqueue -> capacity;
            Thread* found = curr -> t;

            HashQueue_tableRepair(inspect_index, hashqueue);
            free(curr);                                     // free Entry
            return found;
        } else {
            inspect_index = (inspect_index + 1) & table_mask;
        }
    }

    return NULL;

}

static Thread *HashQueue_getByID(u16 thread_id, ThreadQueue* queue) {
    HashQueue* hashqueue = (HashQueue*) queue;
    const u16 table_mask = (hashqueue -> capacity) - 1;
    u16 table_index = hashqueue -> getHash(thread_id) & table_mask;

    while (hashqueue -> table[table_index] != NULL)
    {
        Entry* curr = hashqueue -> table[table_index];
        if (curr -> t -> id == thread_id) {
            return curr -> t;
        } else {
            table_index = (table_index + 1) & table_mask;
        }
    }
    return NULL;
}

static int HashQueue_contains(u16 thread_id, ThreadQueue* queue) {
    return (queue -> getByID(thread_id, queue) != NULL);
}

static int HashQueue_isEmpty(ThreadQueue* queue) {
    HashQueue *hashqueue = (HashQueue*) queue;
    return (hashqueue -> _size == 0);
}

static int HashQueue_size(ThreadQueue* queue) {
    HashQueue *hashqueue = (HashQueue*) queue;
    return hashqueue -> _size;
}

static int HashQueue_getTableIndexByID(u16 thread_id, ThreadQueue* queue) {
    HashQueue* hashqueue = (HashQueue*) queue;
    const u16 table_mask = (hashqueue -> capacity) - 1;
    u16 table_index = hashqueue -> getHash(thread_id) & table_mask;

    while (hashqueue -> table[table_index] != NULL)
    {
        Entry *curr = hashqueue -> table[table_index];
        if (curr -> t -> id == thread_id) {
            return (int) table_index;
        } else {
            table_index = (table_index + 1) & table_mask;
        }
    }
    return -1;
}

static Entry *HashQueue_getEntryByID(u16 thread_id, ThreadQueue* queue) {
    HashQueue* hashqueue = (HashQueue*) queue;
    const u16 table_mask = (hashqueue -> capacity) - 1;
    u16 table_index = hashqueue -> getHash(thread_id) & table_mask;

    while (hashqueue -> table[table_index] != NULL)
    {
        Entry* curr = hashqueue -> table[table_index];
        if (curr -> t -> id == thread_id) {
            return curr;
        } else {
            table_index = (table_index + 1) & table_mask;
        }
    }
    return NULL;
}

//----------------------------------- CONSTRUCTORS + DESTRUCTOR -----------------------------------

 // Iterator functions
 static Thread* Iterator_next(Iterator *iterator) {
    Entry *current = iterator -> currentEntry;
    iterator -> currentEntry = iterator -> currentEntry -> next;
    return current -> t;
}

static int Iterator_hasNext(Iterator *iterator) {
    return iterator -> currentEntry != NULL;  
}

static Iterator* new_Iterator(ThreadQueue* queue) {
    HashQueue *hashqueue = (HashQueue*) queue;
    Iterator *iterator = malloc(sizeof(Iterator));
    if (iterator == NULL) {
        return NULL;
    }

    iterator -> hasNext = Iterator_hasNext;
    iterator -> next = Iterator_next;
    iterator -> currentEntry = hashqueue -> head;

    return iterator;
}

/*
    - Frees the entries in a table provided they are marked as occupied
*/
static void HashQueue_free(ThreadQueue *queue) {
    HashQueue *hashqueue = (HashQueue*) queue;
    // Free each entry in the hash table
    for (int i = 0; i < hashqueue -> capacity; ++i) {
        if (hashqueue -> table[i] != NULL) {
            free(hashqueue -> table[i]);
        }
    }
    free(hashqueue -> table);
    free(hashqueue);
}


/*
    Returns 0 if any malloc failed, 1 otherwise.
*/
int init_HashQueue(HashQueue *this) {
    this -> _size = 0;
    this -> capacity = INITIAL_CAPACITY;
    this -> load_factor = 0.0;
    this -> head = NULL;
    this -> tail = NULL;
    this -> table = malloc(INITIAL_CAPACITY * sizeof(Entry*));
    if (this -> table == NULL) {
        return 0;
    }
    
    for (int i = 0; i < INITIAL_CAPACITY; ++i) {
        this -> table[i] = NULL;        // explicitly set Entry pointers to null   
    }

    
    this -> dequeue = HashQueue_dequeue;
    this -> contains = HashQueue_contains;
    this -> enqueue = HashQueue_enqueue;
    this -> isEmpty = HashQueue_isEmpty;
    this -> removeByID = HashQueue_removeByID;
    this -> getByID = HashQueue_getByID;
    this -> iterator = new_Iterator;
    this -> size = HashQueue_size;
    this -> freeQueue = HashQueue_free;
    this -> getHash = FNV1AHash;
    this -> getTableIndexByID = HashQueue_getTableIndexByID;
    this -> getEntryByID = HashQueue_getEntryByID;
    
    return 1;
}

/*
    - Allocates Memory for the HashQueue, then populates with init_HashQueue
*/
HashQueue *new_HashQueue() {
    HashQueue *this = malloc(sizeof(HashQueue));
    if (this == NULL) {
        return NULL;
    }
    int successful_initialisation = init_HashQueue(this);
    if (successful_initialisation == 0) { // if some memory allocoation failed during construction
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
    HashQueue *new_queue = malloc(sizeof(HashQueue));

    if (new_queue == NULL) {    // If memory allocation failed return the old queue
        QueueResultPair result = {(ThreadQueue*) old_queue, 0};
        return result;
    }

    new_queue -> _size = old_queue -> _size;
    new_queue -> capacity = (old_queue -> capacity) * 2;
    new_queue -> load_factor = (double) new_queue -> _size / new_queue -> capacity;

    new_queue -> head = old_queue -> head;
    new_queue -> tail = old_queue -> tail;
    new_queue -> table = malloc((new_queue -> capacity) * sizeof(Entry*));
    
    if (new_queue -> table == NULL) {
        // returns the old queue as malloc failed
        QueueResultPair result = {(ThreadQueue*) old_queue, 0};
        return result;
    }

    // Set occupied indices to 0 to begin
    for (int i = 0; i < new_queue -> capacity; ++i) {
        new_queue -> table[i] = NULL;
    }

    new_queue -> dequeue = HashQueue_dequeue;
    new_queue -> contains = HashQueue_contains;
    new_queue -> enqueue = HashQueue_enqueue;
    new_queue -> isEmpty = HashQueue_isEmpty;
    new_queue -> removeByID = HashQueue_removeByID;
    new_queue -> getByID = HashQueue_getByID;
    new_queue -> iterator = new_Iterator;
    new_queue -> size = HashQueue_size;
    new_queue -> freeQueue = HashQueue_free;
    new_queue -> getHash = old_queue -> getHash;
    new_queue -> getTableIndexByID = HashQueue_getTableIndexByID;
    new_queue -> getEntryByID = HashQueue_getEntryByID;


    // Rehashing procedure
    const u16 table_mask = (new_queue -> capacity) - 1;
    u16 new_table_index;
    u16 old_table_index;
    Entry *curr = old_queue -> head;
    
    while (curr != NULL) {
        old_table_index = curr -> table_index;
        new_table_index = new_queue -> getHash(curr -> t -> id) & table_mask;
        while (new_queue -> table[new_table_index] != NULL) {
                new_table_index = (new_table_index + 1) & table_mask;
        }
        new_queue -> table[new_table_index] = curr;
        old_queue -> table[old_table_index] = NULL;
        curr -> table_index = new_table_index;
        curr = curr -> next;
    }

    

    old_queue -> freeQueue((ThreadQueue*) old_queue);
    QueueResultPair result;
    result.queue = (ThreadQueue*) new_queue;
    result.result = 1;
    return result;
}
