#include "hash-queue.h"
#include <stdio.h>
#include <stdlib.h>

//------------------------------ HashQueue ADT IMPLEMENTATIONS ------------------------------

/*
    Returns -1 if enqueue failed, 1 if succeeded, 2 if succeeded but rehashing required.
*/
static QueueResultPair HashQueue_enqueue(thread *t, ThreadQueue *queue) {

    HashQueue *hash_queue = (HashQueue*) queue;
    const u16 thread_id = t -> id;
    const u16 table_mask = (hash_queue -> capacity) - 1;
    u16 table_index = thread_id & table_mask;

    while (hash_queue -> table[table_index] != NULL) // iterate while positions unavailable
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
    if (hash_queue -> isEmpty((ThreadQueue*) hash_queue)) {
        hash_queue -> tail = hash_queue -> head = new_entry;
    } else {
        new_entry -> prev = hash_queue -> tail;
        new_entry -> next = NULL;
        hash_queue -> tail -> next = new_entry;
        hash_queue -> tail = new_entry;
    }
    

    hash_queue -> table[table_index] = new_entry;
    ++ hash_queue -> size;

    hash_queue -> load_factor = (double) hash_queue -> size / hash_queue -> capacity;
    QueueResultPair result;

    // Check if rehashing required
    if (hash_queue -> load_factor > REHASH_THRESHOLD) {
        result = HashQueue_rehash(hash_queue);
    } else {
        result.queue = (ThreadQueue*) hash_queue;
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

    BAD BAD:
        - fix if condition:
        - we want to skip over:
            - 1: entries in place, i.e. their ideal index is at the current inspect index
            - 2: entries who's ideal index is lower than the empty index

*/

static void HashQueue_tableRepair(u16 empty_index, HashQueue *hashqueue) {
    const u16 table_mask = (hashqueue -> capacity) - 1;
    u16 inspect_index = (empty_index + 1) & table_mask;                     // we inspect the following index
    u16 thread_id;
    u16 ideal_index;                                                        // will store where an entry would ideally be placed

    while (hashqueue -> table[inspect_index] != NULL) {
        thread_id = hashqueue -> table[inspect_index] -> t -> id;
        ideal_index = thread_id & table_mask;

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
static thread *HashQueue_dequeue(ThreadQueue *queue) {
    HashQueue* hashqueue = (HashQueue*) queue;
    
    if (hashqueue -> isEmpty((ThreadQueue*) hashqueue)) {
        return NULL;
    }

    Entry *entry = hashqueue -> head;
    const u16 table_index = entry -> table_index;   // attained directly without search

    // Update hashqueue fields
    hashqueue -> table[table_index] = NULL;
    -- hashqueue -> size;
    hashqueue -> load_factor = (double) hashqueue -> size / hashqueue -> capacity;
    

    // Update Linked List pointers
    if (hashqueue -> tail == hashqueue -> head) {   // we are removing lone entry in hashqueue
        hashqueue -> tail = NULL;
        hashqueue -> head = NULL;
    } else {                            // there is at least one other entry
        Entry * next = entry -> next;
        next -> prev = NULL;
        hashqueue -> head = next;
    }

    HashQueue_tableRepair(table_index, hashqueue);

    thread *thread = entry -> t;
    free(entry);
    return thread;

}


static thread *HashQueue_removeByID(u16 thread_id, ThreadQueue *queue) {
    HashQueue *hashqueue = (HashQueue*) queue;
    const u16 table_mask = (hashqueue -> capacity) - 1;
    u16 inspect_index = thread_id & table_mask;

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
            -- hashqueue -> size;                           // record size change
            hashqueue -> load_factor = (double) hashqueue -> size / hashqueue -> capacity;
            thread* found = curr -> t;

            HashQueue_tableRepair(inspect_index, hashqueue);
            free(curr);                                     // free Entry
            return found;
        } else {
            inspect_index = (inspect_index + 1) & table_mask;
        }
    }

    return NULL;

}

static int HashQueue_contains(u16 thread_id, ThreadQueue* queue) {
    HashQueue* hashqueue = (HashQueue*) queue;
    const u16 table_mask = (hashqueue -> capacity) - 1;
    u16 table_index = thread_id & table_mask;

    while (hashqueue -> table[table_index] != NULL)
    {
        Entry* curr = hashqueue -> table[table_index];
        if (curr -> t -> id == thread_id) {
            return 1;
        } else {
            table_index = (table_index + 1) & table_mask;
        }
    }
    return 0;
}

static int HashQueue_isEmpty(ThreadQueue* queue) {
    HashQueue * hashqueue = (HashQueue*) queue;
    return (hashqueue -> size == 0);
}

//----------------------------------- CONSTRUCTORS + DESTRUCTOR -----------------------------------

/*
    - Frees the entries in a table provided they are marked as occupied
*/
static void HashQueue_free(HashQueue *this) {
    // Free each entry in the hash table
    for (int i = 0; i < this -> capacity; ++i) {
        if (this -> table[i] != NULL) {
            free(this -> table[i]);
        }
    }
    free(this -> table);
    free(this);
}


/*
    Returns 0 if any malloc failed, 1 otherwise.
*/
int HashQueue_init(HashQueue *this) {
    this -> size = 0;
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

    this -> freeQueue = HashQueue_free;
    this -> dequeue = HashQueue_dequeue;
    this -> enqueue = HashQueue_enqueue;
    this -> contains = HashQueue_contains;
    this -> removeByID = HashQueue_removeByID;
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
    int successful_initialisation = HashQueue_init(this);
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

    new_queue -> size = old_queue -> size;
    new_queue -> capacity = (old_queue -> capacity) * 2;
    new_queue -> load_factor = (double) new_queue -> size / new_queue -> capacity;

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

    new_queue -> freeQueue = HashQueue_free;
    new_queue -> dequeue = HashQueue_dequeue;
    new_queue -> enqueue = HashQueue_enqueue;
    new_queue -> contains = HashQueue_contains;
    new_queue -> removeByID = HashQueue_removeByID;
    new_queue -> isEmpty = HashQueue_isEmpty;

    // Rehashing Procedure
    const u16 table_mask = (new_queue -> capacity) - 1;
    u16 table_index;
    for (int i = 0; i < old_queue -> capacity; ++i) {
        if (old_queue -> table[i] != NULL) {
            Entry * entry = old_queue -> table[i];

            table_index = (entry -> t -> id) & table_mask;
            while (new_queue -> table[table_index] != NULL) {
                table_index = (table_index + 1) & table_mask;
            }
            new_queue -> table[table_index] = entry;

            old_queue -> table[i] = NULL; // set to zero so entries will not be freed
        }
    }
    

    old_queue -> freeQueue(old_queue);
    QueueResultPair result;
    result.queue = (ThreadQueue*) new_queue;
    result.result = 1;
    return result;
}
