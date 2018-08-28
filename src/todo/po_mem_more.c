/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Cf. po_memory.h for a module description
 *
 * This module adds calloc and realloc
 */

#include <po_memory.h>

/*-GLOBAL-
 * calloc equivalent
 */
#if po_memory_TRACK_ALLOC <= 1
void *po_gcalloc(int size, int region)
#else
void *po_gcalloc(int size, int region, char *file, int line)
#endif
{
  #if po_memory_TRACK_ALLOC <= 1
  int *block = (int*)po_gmalloc(size, region);
  #else
  int *block = (int*)po_gmalloc(size, region, file, line);
  #endif


  if ( block != NULL ) {
    int i, remain;

    // Set block array to zero, except for the last partial integer
    for ( i = 0 ; i < size / (int)sizeof(int) ; i++ )
      block[i] = 0;

    // Set the last partial integer if any
    remain = size - i * sizeof(int);
    if ( remain > 0 ) {
      char *blockEnd = (char*)(block + i);
      do { *blockEnd++ = 0; } while ( --remain > 0 );
    }
  }

  return block;
}

/*-GLOBAL-
 * realloc equivalent
 */
#if po_memory_TRACK_ALLOC <= 1
void *po_memory_realloc(void *block, int size)
#else
void *po_memory_realloc(void *block, int size, char *file, int line)
#endif
{
  po_memory_HeaderType *hdr;
  int info, index, region;
  void *blockNew;
  int indexNew;

  #if po_memory_SANITY_CHECK
  if ( block == NULL ) {
    po_error(po_error_MEM_NULL_BLOCK);
    return NULL;
  }
  #endif

  hdr = po_memory_headerAddr(block);
  info = hdr->info ^ po_memory_HEADER_XOR_ALLOC;
  index = po_memory_headerIndex(info);

  // New free list index
  indexNew = po_memory_size2indexInt(size);

  // If no index change, return same block
  if ( indexNew == index ) return block;

  region = po_memory_headerRegion(info);
  #if po_memory_TRACK_ALLOC <= 1
  blockNew = po_gmalloc_index(indexNew, region);
  #else
  blockNew = po_gmalloc_index(indexNew, region, file, line);
  #endif

  // If memory full, we can't copy to a new block
  if ( !blockNew ) return block;

  // Copy from block to blockNew
  {
    int i, remain;
    int *b_old = (int*)block;
    int *b_new = (int*)blockNew;

    // Copy all except for the last partial integer
    for ( i = 0 ; i < size / (int)sizeof(int) ; i++ )
      b_new[i] = b_old[i];

    // Set the last partial integer if any
    remain = size - i * sizeof(int);
    if ( remain > 0 ) {
      char *b_old = (char*)((int*)block + i);
      char *b_new = (char*)((int*)blockNew + i);
      do { *b_new++ = *b_old++; } while ( --remain > 0 );
    }
  }

  po_free(block);
  return blockNew;
}
