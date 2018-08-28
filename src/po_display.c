/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Cf. po_display.h for a module description
 */

#include <po_memory.h>
#include <po_hash.h>
#include <po_hashp.h>

/* Convert from index to effective size
 */
static inline int po_memory_sizeEffGet(int index)
{
  return (index > 0) ? (1<<(index+2))+po_memory_ADD : po_memory_ADD;
}

#if po_memory_TRACK_ALLOC >= 1  // A version can be written for == 0
/*-GLOBAL-
 * Displays all memory allocations and the source of the allocation, file
 * name and line number.
 */
int po_memory_display(po_memory_Region *region)
{
  int index;
  int nAllocTotal = 0;

  //po_log("po_memory: region 0x%p: currently allocated blocks:\n", (int)region, 0);

  for ( index = 0 ; index < region->nFreeLists ; index++ ) {
    int nAlloc = region->freeList[index].nAlloc -
      region->freeList[index].nFree;
    nAllocTotal += nAlloc;
    if ( !nAlloc ) continue;
    //po_log("  size bracket %d to %d bytes: ",
    // index == 0 ? 1 : po_memory_sizeEffGet(index-1)+1,
    // po_memory_sizeEffGet(index));
    //po_log("%d blocks\n", nAlloc, 0);
    
    #if po_memory_TRACK_ALLOC >= 2
    {
      po_list_List *list = &region->freeList[index].alloclist;
      po_list_List *node = po_list_head(list);
      char *file = NULL;
      int line = 0;
      do {
	po_memory_HeaderType *hdr = po_list_getobj(node, po_memory_HeaderType, node);
	if ( hdr->file != file || hdr->line != line ) {
	  //po_log("    %s:%d\n", (int)hdr->file, hdr->line);
	  file = hdr->file;
	  line = hdr->line;
	}
	node = po_list_next(node);
      } while ( node != list );
    }
    #endif
  }
  return nAllocTotal;
}
#endif

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

#if po_DEBUG // DEBUG_MODE
/*-GLOBAL-
 * Display the hash table content. It returns the total number of objects
 * in the hash table.
 */
int po_hash_display(po_hash_Table *hashTable)
{
  int i;
  int count = 0;

  //po_log("po_hash: HASH TABLE %p:", (int)hashTable, 0);
  for ( i = 0 ; i <= hashTable->modulo ; i++ ) {
    po_hash_Node *node = hashTable->entry[i].trunk;
    while ( node != po_hash_Node_DUMMY(hashTable) ) {
      po_list_Node *listNode = po_list_head(&node->list);
      // Display the value
      //po_log("\n%3d: ", node->value, 0);
      // Display the objects assigned this value
      while ( listNode != (void*)&node->list ) {
	//po_log("%p ", (int)listNode, 0);
	count++;
	listNode = po_list_next(listNode);
      }
      node = node->next;
    }
  }
  //po_log("\n", 0, 0);
  return count;
}
#endif // DEBUG_MODE

#if po_DEBUG // DEBUG_MODE
/*-GLOBAL-
 * Display the hash table content. It returns the total number of objects
 * in the hash table.
 * THIS FUNCTION CAN ONLY BE CALLED FROM A LOWER PRIORITY LEVEL in order
 * to return a value. An error is generated if called from a higher priority
 * level.
 */
int po_hashp_display(po_hashp_Table *hTablep)
{
  int count;
  int prevpri = po_function_raisepri(hTablep->priority);
  count = po_hash_display(hTablep->hashTable);
  po_function_restorepri(prevpri);
  return count;
}
#endif // DEBUG_MODE
