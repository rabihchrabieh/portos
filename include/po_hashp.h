/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Protected hash table.
 * This is a layer on top of po_hash that serializes accesses to the
 * hash table by using priority functions at a fixed priority level.
 */

#ifndef po_hashp__H
#define po_hashp__H

#include <portos.h>

/* Hash Table
 */
typedef struct {
  po_hash_Table *hashTable;
  int priority;
} po_hashp_Table;

/*-GLOBAL-INSERT-*/

/*-GLOBAL-
 * Destroys hashp table.
 */
void po_hashp_delete(po_priority(hTablep->priority),
		     po_hashp_Table *hTablep)
;

/*-GLOBAL-
 * Insert an object with some value in the hash table. The object is
 * referenced by the po_list_Node.
 */
void po_hashp_insert(po_priority(hTablep->priority),
		     po_hashp_Table *hTablep, int value, po_list_Node *listNode)
;

/*-GLOBAL-
 * Delete value and return a pointer to the branch of objects. NULL is
 * returned if the value does not exist. The branch of objects is a linked
 * list.
 * THIS FUNCTION CAN ONLY BE CALLED FROM A LOWER PRIORITY LEVEL in order
 * to return a value. An error is generated if called from a higher priority
 * level.
 */
po_list_List *po_hashp_remove(po_hashp_Table *hTablep, int value)
;

/*-GLOBAL-
 * Delete specific object from hash table. This code may crash if the
 * object is not really inserted in the hash table.
 */
void po_hashp_removeobj(po_priority(hTablep->priority),
			po_hashp_Table *hTablep, po_list_Node *listNode)
;

/*-GLOBAL-INSERT-END-*/


#endif // po_hashp__H
