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
#include <po_function.h>
#include <po_hash.h>
#include <po_hashp.h>

/*-Global-
 * Creates a hashp table.
 * priority is the fixed level of the priority functions that will
 * access the hash table.
 */
po_hashp_Table *po_hashp_create(int hashSize, po_memory_Region *memRegion, int priority)
{
  po_hashp_Table *hTablep;

  // Allocate hashp table
  hTablep = (po_hashp_Table*)po_rmalloc_const(sizeof(*hTablep), memRegion);
  if ( !hTablep ) po_memory_error();

  // Create hTable
  hTablep->hashTable = po_hash_create(hashSize, memRegion);
  hTablep->priority = priority;
  return hTablep;
}

/*-GLOBAL-
 * Destroys hashp table.
 */
void po_hashp_delete(po_priority(hTablep->priority),
		     po_hashp_Table *hTablep)
{
  po_hash_delete(hTablep->hashTable);
  #if po_DEBUG // DEBUG_MODE
  hTablep->hashTable = NULL;
  #endif // DEBUG_MODE
  po_free(hTablep);
}

/*-GLOBAL-
 * Insert an object with some value in the hash table. The object is
 * referenced by the po_list_Node.
 */
void po_hashp_insert(po_priority(hTablep->priority),
		     po_hashp_Table *hTablep, int value, po_list_Node *listNode)
{
  po_hash_insert(hTablep->hashTable, value, listNode);
}

/*-GLOBAL-
 * Delete value and return a pointer to the branch of objects. NULL is
 * returned if the value does not exist. The branch of objects is a linked
 * list.
 * THIS FUNCTION CAN ONLY BE CALLED FROM A LOWER PRIORITY LEVEL in order
 * to return a value. An error is generated if called from a higher priority
 * level.
 */
po_list_List *po_hashp_remove(po_hashp_Table *hTablep, int value)
{
  po_list_List *list;
  int prevpri = po_function_raisepri(hTablep->priority);
  list = po_hash_remove(hTablep->hashTable, value);
  po_function_restorepri(prevpri);
  return list;
}

/*-GLOBAL-
 * Delete specific object from hash table. This code may crash if the
 * object is not really inserted in the hash table.
 */
void po_hashp_removeobj(po_priority(hTablep->priority),
			po_hashp_Table *hTablep, po_list_Node *listNode)
{
  po_hash_removeobj(hTablep->hashTable, listNode);
}
