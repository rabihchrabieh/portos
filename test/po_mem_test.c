/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Test for po_memory module.
 */

#include <po_memory.h>
#include <po_display.h>
#include <po_log.h>
#include <miscLib.h>

#if TARGET_SLOW
static long Trials = 5000;
#else
static long Trials = 1000000;
#endif

#define nRegions 3

/* Test 0:
 * This is just to check how the size2index compiles
 */
void *test_compiler(void)
{
  return po_malloc_const(30);
}

/* Test 1:
 * We check if the conversion size to index is working
 */
int test_size2index(void)
{
  static char *str[] = {"BAD", "OK"};
  int ADD = po_memory_ADD;
  int power = 3;
  int size = ADD;
  int indexCurr = 0;
  int error = 0;
  po_log("\n1) TESTING size2index conversion\n"
	 "Size in hex, Index in decimal\n", 0, 0);
  po_log("    Size   Index      Size   Index      Size   Index\n", 0, 0);

  do {
    int result;
    int index[3];
    index[0] =       po_memory_size2index(size-1);
    index[1] =       po_memory_size2index(size);
    index[2] =       po_memory_size2index(size+1);
    result = (indexCurr==index[0])&&(indexCurr==index[1])&&
      (indexCurr==index[2]-1);
    if ( result == 0 ) error++;
    
    po_log("%9X%6d   ", size-1, index[0]);
    po_log("%9X%6d   ", size, index[1]);
    po_log("%9X%6d   ", size+1, index[2]);
    po_log("%s\n", (int)str[result], 0);

    size = (1<<power) + ADD;
    power++;
    indexCurr++;
  } while ( power < 16 );
  
  if ( error ) {
    po_log("There are ERRORS\n", 0, 0);
    return -1;
  } else {
    po_log("SUCCESS\n", 0, 0);
    return 0;
  }
}

// Access regions' vector table
extern po_memory_Region *RegionVector[3];

/* Test 2:
 * We allocate a large heap and we set all bytes to 1.
 * We start randomly allocating and freeing blocks, using calloc again
 * in order to produce memory corruption if something is incorrect.
 * If the memory gets full, we analyze its content to see if everything
 * is OK.
 * We create some number of memory regions.
 */
int test_randomMalloc(void)
{
  #if TARGET_SLOW && CHAR_BIT == 16
  int maxBlockShift = 6;
  #else
  int maxBlockShift = 10;
  #endif
  long trials = Trials;
  long t;
  typedef struct _blockHeaderType {
    struct _blockHeaderType *next;
    short size;
  } blockHeaderType;
  blockHeaderType allocated = {NULL, 0};
  int nAllocated = 0;
  int success = 0;
  int error = 0;
  int regionIndex;

  po_log("\n2) TESTING random malloc and free\n"
	 "(It is OK if the heap gets full. Reduce maxBlockSize\n"
	 "      or increase heapSize to avoid it)\n", 0, 0);

  // Allocate and free randomly
  for ( t = 0 ; t < trials ; t++ ) {
    // With a probability 50% allocate a block
    if ( mlRandomUniform(0,100) < 50 ) {
      blockHeaderType *block;
      // Pick a region
      int regionIndex = mlRandomUniform(0, nRegions);
      // Pick a size
      int size = mlRandomPseudoLog(2,maxBlockShift);
      // Mminimum size is 6 to store the blockHeaderType
      if ( size < sizeof(blockHeaderType) )
	size = sizeof(blockHeaderType);
      // Allocate block
      block = po_rmalloc(size, RegionVector[regionIndex]);
      if ( block != NULL ) {
	int i;
	char *_block = (char*)block;
	for ( i = 0 ; i < size ; i++ ) _block[i] = 0;
	block->size = size;
	// Link this block in a chain with the previously allocated blocks
	block->next = allocated.next;
	allocated.next = block;
	nAllocated++;
	// Set the last byte in the block to some checksum
	if ( size > sizeof(blockHeaderType) )
	  ((char*)block)[size-1] = (int)(long)block - size;
      }
    }
    
    // With a probability 50% free a block
    if ( nAllocated > 0 && mlRandomUniform(0,100) < 50 ) {
      // Pick a block to free
      int number = mlRandomUniform(0,nAllocated);
      // Locate block
      blockHeaderType *block, *prev = &allocated;
      for ( block = allocated.next ; number > 0 ; number-- ) {
	prev = block;
	block = block->next;
      }
      // Unlink this block
      prev->next = block->next;
      nAllocated--;
      // Verify the checksum at the end of the block
      if ( block->size > sizeof(blockHeaderType) &&
	   (((((char*)block)[block->size-1]) - ((int)(long)block - block->size))
	    & 0xFF) != 0 ) {
	//po_log("test_randomMalloc: ERROR: bad checksum\n", 0, 0);
	po_log("X", 0, 0);
	error++;
      } else {
	success++;
      }
      // Free block
      po_free(block);
    }
  }

  // Still allocated memory
  for ( regionIndex = 0 ; regionIndex < nRegions ; regionIndex++ ) {
    nAllocated -= po_memory_display(RegionVector[regionIndex]);
  }
  po_log("Remaining nAllocated blocks %d\n", nAllocated, 0);

  if ( error || nAllocated ) {
    po_log("FAILURE: there are ERRORS\n", 0, 0);
    return -1;
  } else {
    po_log("SUCCESS: %d blocks randomly allocated and freed\n", success, 0);
    return 0;
  }
}
