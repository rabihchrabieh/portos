/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Cf. po_memory.h for a module description
 */

#include <po_sys.h>
#include <po_lib.h>
#include <po_memory.h>
#include <po_list.h>

/* Heap full management: internal function reacts differently from
 * global function
 */
static inline void po_memory_heapfull(void)
{
  #if po_memory_HEAP_FULL_ABORT
  po_error(po_error_MEM_HEAP_FULL);
  #endif
}

/* Returns the header address of a given block
 */
static inline po_memory_HeaderType *po_memory_headerAddr(void *block)
{
  return (po_memory_HeaderType*)((char*)block - po_memory_HEADER_SIZE);
}

#if 0
/* Data structure for this module
 */
po_NEARFAR po_memory_DataType po_memory_Data = {{0}};
#endif

#if po_memory_VARIABLE_SIZE
/* Convert size to index.
 */
static int po_memory_size2indexInt(int size)
{
  int delta = size - po_memory_ADD - 1;
  if ( delta >= 0 ) {
    int index = po_lib_msb(delta) - 1;
    return ( index > 0 ) ? index : 1;  // Can be further optimized
  } else return 0;
}
#endif

/* Convert from index to effective size.
 */
static inline int po_memory_index2size(int index)
{
  int size = po_memory_ADD;
  if ( index > 0 ) size += 1<<(index+2);
  return size;
}

/*-GLOBAL-
 * A faster version of malloc. When the size of some data structure
 * is fixed and known, the caller can get and store the index corresponding
 * to this size (using po_memory_size2index). Then call this version of malloc
 * for fastest results.
 */
#if po_memory_TRACK_ALLOC <= 1
void *po_gmalloc_index(int index, po_memory_Region *region)
#else
void *po_gmalloc_index(int index, po_memory_Region *region, char *file, int line)
#endif
{
  int protectState;
  po_memory_FreeListType *freeList;
  void *block;
  po_memory_HeaderType *hdr;

  // If the index is too large, this block cannot be allocated in this
  // region.
  #if po_DEBUG > 1
  if ( index >= region->nFreeLists ) {
    po_error(po_error_MEM_BLOCK_TOO_LARGE);
    return NULL;
  }
  #endif

  freeList = &region->freeList[index];

  protectState = po_interrupt_disable();

  // If free list is not empty take one element from this list
  if ( (block = freeList->next) != NULL ) {
    // Remove the element from the list.
    freeList->next = ((po_memory_FreeListEntryType*)block)->next;

    #if po_memory_TRACK_ALLOC >= 1
    freeList->nFree--;
    #endif

    po_interrupt_restore(protectState);

    #if po_memory_SANITY_CHECK
    hdr = po_memory_headerAddr(block);
    hdr->freeList = freeList;
    #endif

  } else {
    char *heap_current;
    int size;

    po_interrupt_restore(protectState);

    // Free list is empty. Get a new block from the heap buffe
    size = po_memory_index2size(index) + po_memory_HEADER_SIZE;

    protectState = po_interrupt_disable();
    block = region->heap.current;
    heap_current = region->heap.current = (char*)block + size;
    po_interrupt_restore(protectState);

    // Check if heap buffer is full
    if ( heap_current - region->heap.end > po_memory_HEADER_SIZE ) {
      // Heap full
      po_memory_heapfull();
      return NULL;
    }

    #if po_memory_TRACK_ALLOC >= 1
    protectState = po_interrupt_disable();
    freeList->nAlloc++;
    po_interrupt_restore(protectState);
    #endif

    hdr = po_memory_headerAddr(block);
    hdr->freeList = freeList;
  }

  #if po_memory_TRACK_ALLOC >= 2
  hdr = po_memory_headerAddr(block);
  hdr->file = file;
  hdr->line = line;
  protectState = po_interrupt_disable();
  po_list_pushtail(&freeList->alloclist, &hdr->node);
  po_interrupt_restore(protectState);
  #endif

  return block;
}

#if po_memory_VARIABLE_SIZE
/*-GLOBAL-
 * malloc equivalent
 */
#if po_memory_TRACK_ALLOC <= 1
void *po_gmalloc(int size, po_memory_Region *region)
#else
void *po_gmalloc(int size, po_memory_Region *region, char *file, int line)
#endif
{
  // Get free list index
  int index = po_memory_size2indexInt(size);

  #if 0
  // In order to test internal size2index
  if ( index != po_memory_size2index(size) ) {
    po_error(-1);
  }
  #endif

  // Call mallocIndex
  #if po_memory_TRACK_ALLOC <= 1
  return po_gmalloc_index(index, region);
  #else
  return po_gmalloc_index(index, region, file, line);
  #endif
}
#endif

/*-GLOBAL-
 * free equivalent.
 */
void po_free(void *block)
{
  int protectState;
  po_memory_FreeListType *freeList;
  po_memory_HeaderType *hdr;

  #if po_DEBUG
  if ( !block ) {
    po_error(po_error_MEM_NULL_BLOCK);
    return;
  }
  #endif

  hdr = po_memory_headerAddr(block);
  freeList = (po_memory_FreeListType*)hdr->freeList;

  #if po_memory_SANITY_CHECK
  if ( freeList->sanity != po_memory_SANITY_VALUE ) {
    #if 0
    // Some of the tests below should be made before the sanity one to
    // prevent memory access error on certain platforms.
    if ( freeList == po_memory_ALLOCATED_FOREVER ) {
      // This is a block that was allocated forever and that should not be
      // freed.
      po_error(po_error_MEM_FREEING_FOREVER_BLOCK);
    } else
    #endif

    if ( freeList == po_memory_NON_ALLOCATED ) {
      // Attempt to free a block for a second time
      po_error(po_error_MEM_MULTIPLE_FREE);
    } else {
      // Memory corrupted!
      po_error(po_error_MEM_CORRUPT_MEMORY);
    }
    return;
  }
  #endif

  protectState = po_interrupt_disable();

  #if po_memory_SANITY_CHECK

  // Check the header once again in protected mode in case a preempting
  // process freed the same block just before this point
  if ( hdr->freeList != freeList  ) {
    // Attempt to free a block more than once
    po_interrupt_restore(protectState);
    po_error(po_error_MEM_MULTIPLE_FREE);
    return;
  }

  // Set the header to free
  hdr->freeList = po_memory_NON_ALLOCATED;

  #endif

  // Return the block to the free list
  ((po_memory_FreeListEntryType*)block)->next = freeList->next;
  freeList->next = (po_memory_FreeListEntryType*)block;

  #if po_memory_TRACK_ALLOC >= 1
  freeList->nFree++;
  #endif

  #if po_memory_TRACK_ALLOC >= 2
  po_list_pop(&hdr->node);
  #endif

  po_interrupt_restore(protectState);
}

#if 0

/*-GLOBAL-
 * Allocate forever. You should not attempt to free a buffer that has
 * been allocated with this function. This is useful to allocate certain
 * blocks that will never be freed, such as the stack of some task, etc.
 * This function allocates a block on the heap buffer and with the
 * precise size.
 * region is defined by po_memory_region().
 */
#if po_memory_TRACK_ALLOC <= 1
void *po_gmalloc_forever(int size, po_memory_Region *region)
#else
void *po_gmalloc_forever(int size, po_memory_Region *region, char *file, int line)
#endif
{
  int protectState;
  po_memory_HeaderType *hdr;
  void *block;
  char *heap_current;

  // After adding to size the header size, the result is rounded
  // up to a multiple of po_memory_ALIGN in order to keep the current heap
  // pointer correctly aligned at a boundary multiple of po_memory_ALIGN.
  if ( size <= 0 ) size = 1;
  size = (size + po_memory_HEADER_SIZE + po_memory_ALIGN - 1) & ~(po_memory_ALIGN - 1);

  // Get heap current pointer then update it
  protectState = po_interrupt_disable();
  block = region->heap.current;
  heap_current = region->heap.current = (char*)block + size;
  po_interrupt_restore(protectState);

  // Check if heap buffer is full (or about to be full?)
  if ( heap_current - region->heap.end > po_memory_HEADER_SIZE ) {
    // Do something
    po_memory_heapfull();
    return NULL;   // for the time being
  }

  // Fill out the header of this block
  hdr = po_memory_headerAddr(block);
  hdr->freeList = po_memory_ALLOCATED_FOREVER;

  #if po_memory_TRACK_ALLOC >= 2
  hdr->file = file;
  hdr->line = line;
  #endif

  return block;
}

#endif

/*-GLOBAL-
 * Init memory region. An inline function wrapper should be called.
 */
void po_memory_regioninit(po_memory_Region *region)
{
  int i;
  int nFreeLists = region->nFreeLists;

  // Properly align heap.current pointer (this can't be done at load time)
  region->heap.current = (char*)((size_t)region->heap.current & ~(po_memory_ALIGN-1));

  // The loop can be removed alltogether at least for release mode
  // if memory is preset to 0
  #if _TI_
  #pragma MUST_ITERATE(1);
  #endif

  // Init the free lists pointer
  for ( i = 0 ; i < nFreeLists ; i++ ) {
    po_memory_FreeListType *freeList = &region->freeList[i];

    freeList->next = NULL;

    #if po_memory_SANITY_CHECK
    freeList->sanity = po_memory_SANITY_VALUE;
    #endif

    #if po_memory_TRACK_ALLOC >= 1
    freeList->nFree = 0;
    freeList->nAlloc = 0;
    #endif

    #if po_memory_TRACK_ALLOC >= 2
    po_list_init(&freeList->alloclist);
    #endif
  }
}
