/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Cf. po_hash.h for a module description.
 */

#include <po_sys.h>
#include <po_lib.h>
#include <po_memory.h>
#include <po_list.h>
#include <po_hash.h>

/* Macros to obtain the dummy node and the trunk node.
 * The dummy node is the one such that hashTable->dummyValue is
 * superimposed on node->value. Access it as
 *   po_hash_Node_DUMMY(hashTable)
 *   po_hash_Node_DUMMY(hashTable)->value
 * Do not access any other field in this node since they don't exist.
 */
#define po_hash_Node_DUMMY(hashTable) \
  ((po_hash_Node*)((char*)&(hashTable)->dummyValue-offsetof(po_hash_Node, value)))

/* The TRUNK node is the one such that hashTable->entry[i].trunk is
 * superimposed on node->next. Access it as
 *   po_hash_Node_TRUNK(hashTable->entry[i].trunk)
 *   po_hash_Node_TRUNK(hashTable->entry[i].trunk)->next
 * Do not access any other field in this node since they don't exist
 */
#define po_hash_Node_TRUNK(trunk) \
  ((po_hash_Node*)((char*)&(trunk) - offsetof(po_hash_Node, next)))

/* The NODE is the node address given its embedded field list.
 */
#define po_hash_Node_NODE(listP) \
  ((po_hash_Node*)((char*)(listP) - offsetof(po_hash_Node, list)))

/*-GLOBAL-
 * Creates a hash table. hashSize is the number of entries in the table. 
 * It must be a power of 2.
 * memRegion is the memory region where to allocate the table and all
 * internal handles.
 */
po_hash_Table *po_hash_create(int hashSize, po_memory_Region *memRegion)
{
  po_hash_Table *hashTable;
  int i;

  // If hash size is not a power of 2, return NULL.
  #if po_DEBUG // DEBUG_MODE
  if ( hashSize < 1 || (hashSize & (hashSize - 1)) ) {
    po_error(po_error_HASH_NOT_POWER_OF_2);
    return NULL;
  }
  #endif // DEBUG_MODE

  // Allocate hash table
  hashTable = (po_hash_Table*)
    po_rmalloc(sizeof(po_hash_Table) + (hashSize-1)*sizeof(po_hash_Entry),
	       memRegion);
  if ( !hashTable ) po_memory_error();

  // Init hash table fields
  hashTable->modulo = hashSize - 1;
  hashTable->memRegion = memRegion;
  for ( i = 0 ; i < hashSize ; i++ )
    hashTable->entry[i].trunk = po_hash_Node_DUMMY(hashTable);

  return hashTable;
}

/*-GLOBAL-
 * Destroys hash table.
 */
void po_hash_delete(po_hash_Table *hashTable)
{
  int hash;
  int last = hashTable->modulo;
  for ( hash = 0 ; hash <= last ; hash++ ) {
    po_hash_Node *node = hashTable->entry[hash].trunk;
    while ( node != po_hash_Node_DUMMY(hashTable) ) {
      po_hash_Node *nodeNext = node->next;
      po_free(node);
      node = nodeNext;
    }
  }
  #if po_DEBUG // DEBUG_MODE
  hashTable->modulo = 0;
  hashTable->entry[0].trunk = NULL;
  #endif // DEBUG_MODE
  po_free(hashTable);
}

/*-GLOBAL-
 * Insert an object with some value in the hash table. The object is
 * referenced by the po_list_Node.
 */
void po_hash_insert(po_hash_Table *hashTable, int value, po_list_Node *listNode)
{
  po_hash_Node *nodePrev, *nodeNext, *node;
  int delta;

  // Locate the hash table entry
  int hash = value & hashTable->modulo;
  po_hash_Node *trunk = hashTable->entry[hash].trunk;

  if ( trunk == po_hash_Node_DUMMY(hashTable) ) {
    // Create node and insert object as first in branch
    node = (po_hash_Node*)po_rmalloc_const(sizeof(po_hash_Node),
					      hashTable->memRegion);
    if ( !node ) po_memory_error();

    node->value = value;
    po_list_init(&node->list);
    po_list_pushtail(&node->list, listNode);
    node->next = trunk;
    hashTable->entry[hash].trunk = node;
    return;
  }

  // Set the dummy node to value + 1 so that the loop below stops
  hashTable->dummyValue = value + 1;

  // Search for value. The loop will stop since the dummy node contains
  // value + 1
  nodePrev = po_hash_Node_TRUNK(hashTable->entry[hash].trunk);
  nodeNext = trunk;
  delta = po_lib_CMP(value, nodeNext->value);
  while ( delta > 0 ) {
    nodePrev = nodeNext;
    nodeNext = nodeNext->next;
    delta = po_lib_CMP(value, nodeNext->value);
  }

  // Check if nodeNext has the same value
  if ( delta == 0 ) {
    // Node has same value. Do not create node. Insert object at end of
    // branch
    po_list_pushtail(&nodeNext->list, listNode);
    return;
  }

  // Create a node and insert it after nodePrev and before node. Insert
  // object as first in branch
  node = (po_hash_Node*)po_rmalloc_const(sizeof(po_hash_Node),
					    hashTable->memRegion);
  if ( !node ) po_memory_error();

  node->value = value;
  po_list_init(&node->list);
  po_list_pushtail(&node->list, listNode);
  node->next = nodeNext;
  nodePrev->next = node;
}

/*-GLOBAL-
 * Delete value and return a pointer to the branch of objects. NULL is
 * returned if the value does not exist. The branch of objects is a linked
 * list that contains a dummy node and that should be freed by the caller.
 */
po_list_List *po_hash_remove(po_hash_Table *hashTable, int value)
{
  po_hash_Node *nodePrev, *node;
  int delta;

  // Locate the hash table entry
  int hash = value & hashTable->modulo;
  po_hash_Node *trunk = hashTable->entry[hash].trunk;

  // If empty linked list, return NULL
  if ( trunk == po_hash_Node_DUMMY(hashTable) ) return NULL;

  // Set the dummy node to value + 1 so that the loop below stops
  hashTable->dummyValue = value + 1;

  // Search for value. The loop will stop since the dummy node contains
  // value + 1
  nodePrev = po_hash_Node_TRUNK(hashTable->entry[hash].trunk);
  node = trunk;
  delta = po_lib_CMP(value, node->value);
  while ( delta > 0 ) {
    nodePrev = node;
    node = node->next;
    delta = po_lib_CMP(value, node->value);
  }

  // Check if nodeNext has the same value
  if ( delta == 0 ) {
    // Node has same value. Remove this node.
    nodePrev->next = node->next;
    // Return list: note that the dummy node of the list is the same
    // as hash node.
    return (po_list_List*)node;
  }

  // The value was not found. Return NULL
  return NULL;
}

/*-GLOBAL-
 * Delete specific object from hash table. This code may crash if the
 * object is not really inserted in the hash table.
 */
void po_hash_removeobj(po_hash_Table *hashTable, po_list_Node *listNode)
{
  // If this is only listNode in branch, remove the node
  if ( po_list_iscount1(listNode) ) {
    // Locate the hash table node in the trunk. A slightly faster and
    // easier search can be done to locate the node itself. However,
    // the code is more robust if we locate the value.
    po_hash_Node *nodePrev, *node;
    int delta;
    // listNode->next is the list field inside the node
    int value = po_hash_Node_NODE(listNode->next)->value;
    int hash = value & hashTable->modulo;
    po_hash_Node *trunk = hashTable->entry[hash].trunk;

    // If empty linked list, return NULL
    if ( trunk == po_hash_Node_DUMMY(hashTable) ) {
      // Error: this object is not in hash table
      po_error(po_error_HASH_NODE_NOT_IN_TABLE);
      return;
    }

    // Set the dummy node to value + 1 so that the loop below stops
    hashTable->dummyValue = value + 1;

    // Search for value. The loop will stop since the dummy node contains
    // value + 1
    nodePrev = po_hash_Node_TRUNK(hashTable->entry[hash].trunk);
    node = trunk;
    delta = po_lib_CMP(value, node->value);
    while ( delta > 0 ) {
      nodePrev = node;
      node = node->next;
      delta = po_lib_CMP(value, node->value);
    }

    // Check if nodeNext has the same value
    if ( delta == 0 ) {
      // Node has same value. Delete this node.
      nodePrev->next = node->next;
      po_free(node);
    } else {
      // Error: this object is not in hash table
      po_error(po_error_HASH_NODE_NOT_IN_TABLE);
      return;
    }

  } else {
    // Simply remove from branch
    po_list_pop(listNode);
  }
}

