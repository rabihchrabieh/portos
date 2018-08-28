/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * The hash table is an efficient database for storing and removing objects
 * in a random fashion.
 * An object, or a list of objects, is characterized by a unique value.
 * An object is inserted and deleted in a random fashion using
 * the object's unique value.
 *
 * The hash table consists of some number of hash entries (the number is
 * a power of 2 for fast modulo operation). Each entry points at a sorted
 * linked list of nodes. These linked lists are called trunks. Each node
 * has a unique value, the one corresponding to the list of objects the node
 * is pointing at.
 *
 * When an object O with value V is inserted in the database, a node is
 * created and O is inserted in the branch (linked list) under the node.
 * If more objects with the same value V are inserted, the objects are
 * inserted in the branch without creating new nodes.
 *
 * The current implementation uses a simple linked list of nodes in the
 * trunk. Deletion can sometimes be achieved faster with a
 * double linked list.
 *
 * To increase performance, each hash table is assigned one dummy node. The
 * dummy node contains a value which is set above the search value.
 * The dummy node ensures the search loops will terminate. The end of the
 * trunk points at the dummy node.
 *
 * This code is non-preemptable. It cannot be accessed from different
 * priority levels without protection.
 *
 * Additional info:
 * A node contains a value and a pointer to an object or list of objects.
 * The value of the node is unique, ie, in the database there are no 2 nodes
 * with the same value. The node's value is mapped to a hash key.
 * The hash table consists of an array of hash entries. Each hash entry
 * corresponds to a hash key. A hash entry is a pointer to a linked list
 * of nodes all of which have the same hash key (but different values).
 * The linked lists are sorted in increasing order of values. The linked
 * lists are simple linked lists. The last node points at one dummy
 * node shared by all linked lists in this hash table. The dummy node is
 * set to a value that terminates searches.
 * The entry pointer is NULL when the linked list is empty.
 */

#ifndef po_hash__H
#define po_hash__H

#include <po_list.h>

/* Hash table node. The node has a value and a linked list of objects.
 * The value of the node is transformed into a hash
 * key and the node is stored in the corresonding hash entry.
 */
typedef struct _po_hash_Node {
  po_list_List list;  /* MUST BE FIRST */
  struct _po_hash_Node *next;
  int value;
} po_hash_Node;

/* One hash table entry. It is the start of a linked list of nodes
 * having the same hash key.
 */
typedef struct {
  po_hash_Node *trunk;
} po_hash_Entry;

/* Hash Table
 */
typedef struct {
  // hash size - 1 (for hash size power of 2)
  int modulo;

  // dummyValue shared by all linked lists. This should be po_hash_Node
  // but we use just one field of this structure here.
  int dummyValue;
  
  // Memory region used for hash table and all internal dynamic allocations
  po_memory_Region *memRegion;

  // This should be last. The default size is 1, but we will allocate
  // a larger vector.
  po_hash_Entry entry[1];
} po_hash_Table;

/*-GLOBAL-INSERT-*/

/*-GLOBAL-
 * Creates a hash table. hashSize is the number of entries in the table. 
 * It must be a power of 2.
 * memRegion is the memory region where to allocate the table and all
 * internal handles.
 */
po_hash_Table *po_hash_create(int hashSize, po_memory_Region *memRegion)
;

/*-GLOBAL-
 * Destroys hash table.
 */
void po_hash_delete(po_hash_Table *hashTable)
;

/*-GLOBAL-
 * Insert an object with some value in the hash table. The object is
 * referenced by the po_list_Node.
 */
void po_hash_insert(po_hash_Table *hashTable, int value, po_list_Node *listNode)
;

/*-GLOBAL-
 * Delete value and return a pointer to the branch of objects. NULL is
 * returned if the value does not exist. The branch of objects is a linked
 * list that contains a dummy node and that should be freed by the caller.
 */
po_list_List *po_hash_remove(po_hash_Table *hashTable, int value)
;

/*-GLOBAL-
 * Delete specific object from hash table. This code may crash if the
 * object is not really inserted in the hash table.
 */
void po_hash_removeobj(po_hash_Table *hashTable, po_list_Node *listNode)
;

/*-GLOBAL-INSERT-END-*/


#endif // po_hash__H
