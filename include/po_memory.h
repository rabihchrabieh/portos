/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Dynamic allocation and freeing.
 * A very efficient dynamic allocation is implemented that can be called
 * from task or interrupt context.
 * Free lists, each having a fixed block size, are maintained. When the
 * free list is empty, a block is allocated from a large buffer which is
 * accessed sequentially.
 */

#ifndef po_memory__H
#define po_memory__H

#include <limits.h>
#include <po_sys.h>
#include <po_list.h>

/* Report memory allocation error.
 */
static inline void po_memory_error(void)
{
  #if !po_memory_HEAP_FULL_ABORT
  po_error(po_error_MEM_HEAP_FULL);
  #endif
}

/* Structure used when the block is free. It is written in the block's
 * useful area (not in the header) in order to keep the header intact
 * so we don't have to rewrite it. Note that the pointer points at the
 * start of the block, not at the header. This is the same pointer as
 * the one returned by po_memory_ and used to call po_free.
 * All _po_memory_FreeListType pointers should be initialized to NULL at
 * start up.
 */
typedef struct _po_memory_FreeListEntryType {
    struct _po_memory_FreeListEntryType *next;
} po_memory_FreeListEntryType;

/* Structure describing a free list of some given size (power of 2).
 */
typedef struct {
  po_memory_FreeListEntryType *next; // pointer to next free block

  #if po_memory_SANITY_CHECK
  int sanity;             // For sanity checking
  #endif

  #if po_memory_TRACK_ALLOC >= 1
  int nFree;              // number of free elements in this list
  int nAlloc;             // total allocated elements in this list
  #endif

  #if po_memory_TRACK_ALLOC >= 2
  po_list_List alloclist; // List to track allocated blocks
  #endif
} po_memory_FreeListType;

/* Header that prepends every block. When the block is allocated, it
 * contains information on the free list index and some sanity checking.
 * When the block is not allocated, we keep this information so that we
 * don't have to rewrite it. The information does not change except to
 * signal if a block is allocated or free (for debugging purposes).
 */
typedef struct {
  // Pointer to corresponding free list
  volatile po_memory_FreeListType *freeList;

  #if po_memory_TRACK_ALLOC >= 2
  char *file;
  int line;
  po_list_Node node;  /* allocated list as opposed to free list */
  #endif
} po_memory_HeaderType;

/* Extra space at the end of each block in order to bring the next block
 * to a multiple of po_memory_ALIGN boundary.
 */
#define po_memory_HES (- sizeof(po_memory_HeaderType) & (po_memory_ALIGN-1))

/* Additional offset to make some more room for structures that have a
 * size a bit more than some power of 2. The addition must be a multiple
 * of po_memory_ALIGN to maintain alignment.
 * The minimum size of a block should be able to hold a pointer.
 */
#define po_memory_ADD (po_memory_HES + 8)

/* Converts a memory block size into index in free lists.
 * We define it as a macro rather than an inline so that it can be more
 * often reduced to a single number by the compiler.
 * WARNING:
 * But as a macro it has a major side effect problem if called with a
 * size that is an expression or a function that keeps changing value.
 * For proper behavior, "size" must be a constant.
 */
#define po_memory_size2index(size) (	  \
  ( (size) <=         po_memory_ADD ) ?  0 : \
  ( (size) <= (1<< 3)+po_memory_ADD ) ?  1 : \
  ( (size) <= (1<< 4)+po_memory_ADD ) ?  2 : \
  ( (size) <= (1<< 5)+po_memory_ADD ) ?  3 : \
  ( (size) <= (1<< 6)+po_memory_ADD ) ?  4 : \
  ( (size) <= (1<< 7)+po_memory_ADD ) ?  5 : \
  ( (size) <= (1<< 8)+po_memory_ADD ) ?  6 : \
  ( (size) <= (1<< 9)+po_memory_ADD ) ?  7 : \
  ( (size) <= (1<<10)+po_memory_ADD ) ?  8 : \
  ( (size) <= (1<<11)+po_memory_ADD ) ?  9 : \
  ( (size) <= (1<<12)+po_memory_ADD ) ? 10 : \
  ( (size) <= (1<<13)+po_memory_ADD ) ? 11 : \
  ( (size) <= (1<<14)+po_memory_ADD ) ? 12 : 13)

/* Constants
 */
enum {
  // Size of header
  po_memory_HEADER_SIZE = sizeof(po_memory_HeaderType),

  // Sanity check value
  po_memory_SANITY_VALUE = (int)0xA4B5C6D7
};

// Block allocated forever
#define po_memory_ALLOCATED_FOREVER ((po_memory_FreeListType*)0xA2B3)

// Block not currently allocated (waiting in free list)
#define po_memory_NON_ALLOCATED ((po_memory_FreeListType*)0xA6B7)

/* A region's environment. This contains all the state variables needed to
 * manage the heap in some region in memory. Multiple regions can be defined,
 * eg, one in internal RAM, one in external RAM, etc.
 * This struct could be stored at the start of the heap or in some other
 * memory region, eg, internal memory for more efficient processing.
 */
typedef struct {
  /* Initial large buffer used to allocate blocks when the free list
   * is empty. When the buffer is full or about to become full, another
   * can be allocated if needed and if available. The init code should
   * allocate memory in this buffer.
   */
  struct {
    #if po_memory_TRACK_ALLOC >= 1
    char *start;     // start of heap (only needed for reporting info)
    #endif
    char *current;   // current position in heap buffer
    char *end;       // end of heap pointer
  } heap;

  /* Number of free lists in this region, needed for debugging. But it's
   * tricky to remove it in initialization phase.
   */
  //#if po_DEBUG || po_memory_TRACK_ALLOC >= 1
  int nFreeLists;
  //#endif

  /* Array of free lists. Each index has a corresponding power of 2 size.
   * There is a set of free lists per memory region. Each memory region
   * may have a number of free lists, depending on the largest block
   * that can be allocated.
   * freeList[i] is free list number i for this region. We allocate here
   * only freeList[0] but this structure will be extended to more indexes
   * when allocated.
   */
  po_memory_FreeListType freeList[1];

  // DO NOT PUT ANYTHING AFTER FIELD freeList
} po_memory_Region;

/* Macro for region init. po_memory_regioninit is also needed for complete
 * initialization.
 */
#if po_memory_TRACK_ALLOC >= 1
#  define po_memory_REGIONINIT(region, heap, length)			\
  {{                             					\
    {(char*)(heap),							\
     (char*)(heap) + (po_memory_HEADER_SIZE + po_memory_ALIGN - 1),	\
     (char*)(heap) + (length)},   					\
     sizeof(region.storage)/sizeof(region.storage[0])+1 	\
  }}
#else
#  define po_memory_REGIONINIT(region, heap, length)			\
  {{									\
    {(char*)(heap) + (po_memory_HEADER_SIZE + po_memory_ALIGN - 1),	\
     (char*)(heap) + (length)},   					\
     sizeof(region.storage)/sizeof(region.storage[0])+1 		\
  }}
#endif

/* Macro to define the memory region structure with variable number of
 * free lists. A compounded structure is created.
 */
#define po_memory_RegionLocal(region_, maxblksize)			\
  struct {								\
    po_memory_Region region;						\
    po_memory_FreeListType storage[po_memory_size2index(maxblksize)];	\
  } region_

// This method requires separate files for declarations
#define po_memory_Region(region, heap, length, maxblksize)		\
  po_memory_RegionLocal(region, (maxblksize)) =				\
    po_memory_REGIONINIT(region, (heap), (length))

#if 0 // Requires GCC extensions and fails in initializations (GCC bug?)
#define po_memory_Region(region, heap, length, maxblksize)		\
  static po_memory_RegionLocal(region ## _po_storage, (maxblksize)) =	\
    po_memory_REGIONINIT(region ## _po_storage, (heap), (length));	\
  po_target_ALIAS_SYMBOL(po_memory_Region, region, region ## _po_storage)
#endif

#if 0 // Requires GCC extensions and fails under GDB (GCC bug?)
#define po_memory_Region(region, heap, length, maxblksize)		\
  po_memory_RegionLocal(region ## _po_storage, (maxblksize))		\
    __asm__("_" #region) =						\
    po_memory_REGIONINIT(region ## _po_storage, (heap), (length));	\
  extern po_memory_Region region
#endif

/* Default memory region when it's not specified
 */
#ifndef po_INIT_FILE
extern po_memory_Region po_memory_RegionDefault;
#endif

/*-GLOBAL-INSERT-*/

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
;

/*-GLOBAL-
 * malloc equivalent
 */
#if po_memory_TRACK_ALLOC <= 1
void *po_gmalloc(int size, po_memory_Region *region)
#else
void *po_gmalloc(int size, po_memory_Region *region, char *file, int line)
#endif
;

/*-GLOBAL-
 * free equivalent.
 */
void po_free(void *block)
;

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
;

/*-GLOBAL-
 * Init memory region. An inline function wrapper should be called.
 */
void po_memory_regioninit(po_memory_Region *region)
;

/*-GLOBAL-INSERT-END-*/

/*-GLOBAL-INLINE-
 * A faster version of malloc that should only be used when the size of
 * a memory block is a constant such that the compiler automatically converts
 * it into an index without the need to call functions.
 */
#if po_memory_TRACK_ALLOC <= 1
static inline void *po_gmalloc_const(int size, po_memory_Region *region)
#else
static inline void *po_gmalloc_const(int size, po_memory_Region *region, char *file, int line)
#endif
{
  #if po_memory_TRACK_ALLOC <= 1
  return po_gmalloc_index(po_memory_size2index(size), region);
  #else
  return po_gmalloc_index(po_memory_size2index(size), region, file, line);
  #endif
}

/*-GLOBAL-
 * Functions using the a user supplied region.
 */
#if po_memory_TRACK_ALLOC <= 1
#define po_rmalloc(size, region)         po_gmalloc((size), (region))
#define po_rmalloc_const(size, region)   po_gmalloc_const((size), (region))
#define po_rmalloc_constP(size, region)  po_gmalloc_const((size), (region))
#define po_rmalloc_forever(size, region) po_gmalloc_forever((size), (region))
#define po_rmalloc_index(index, region)  po_gmalloc_index((index), (region))
#define po_rcalloc(size, region)         po_gcalloc((size), (region))

#else
#define po_rmalloc(size, region)         po_gmalloc((size), (region), __FILE__, __LINE__)
#define po_rmalloc_const(size, region)   po_gmalloc_const((size), (region), __FILE__, __LINE__)
#define po_rmalloc_constP(size, region)  po_gmalloc_const((size), (region), %f, %l)
#define po_rmalloc_forever(size, region) po_gmalloc_forever((size), (region), __FILE__, __LINE__)
#define po_rmalloc_index(index, region)  po_gmalloc_index((index), (region), __FILE__, __LINE__)
#define po_rcalloc(size, region)         po_gcalloc((size), (region), __FILE__, __LINE__)
#endif

/*-GLOBAL-
 * Functions using the default region of 0.
 */
#define po_malloc(size)          po_rmalloc((size), &po_memory_RegionDefault)
#define po_malloc_const(size)    po_rmalloc_const((size), &po_memory_RegionDefault)
#define po_malloc_forever(size)  po_rmalloc_forever((size), &po_memory_RegionDefault)
#define po_malloc_index(index)   po_rmalloc_index((index), &po_memory_RegionDefault)
#define po_calloc(size)          po_rcalloc((size), &po_memory_RegionDefault)

/*-GLOBAL-
 * Functions used by portos (system functions) and using the system region
 * (that can be region 0 or any other region).
 */
#define po_smalloc(size)         po_rmalloc((size), &po_memory_RegionSystem)
#define po_smalloc_const(size)   po_rmalloc_const((size), &po_memory_RegionSystem)
#define po_smalloc_constP(size)  po_rmalloc_constP((size), &po_memory_RegionSystem)
#define po_smalloc_forever(size) po_rmalloc_forever((size), &po_memory_RegionSystem)
#define po_smalloc_index(index)  po_rmalloc_index((index), &po_memory_RegionSystem)
#define po_scalloc(size)         po_rcalloc((size), &po_memory_RegionSystem)


#endif // po_memory__H
