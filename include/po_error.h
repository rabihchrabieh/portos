/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Error codes.
 */

#ifndef po_error__H
#define po_error__H

/* System error codes
 */
enum {
  po_error_MEM_HEAP_FULL = 100,      /* Memory heap is full */
  po_error_MEM_CORRUPT_MEMORY,       /* Dynamic memory corrupted */
  po_error_MEM_INVALID_REGION,       /* Invalid memory region number */
  po_error_MEM_MULTIPLE_FREE,        /* Freeing a block more than once */
  po_error_MEM_NULL_BLOCK,           /* Freeing or reallocating a NULL block */
  po_error_MEM_BLOCK_TOO_LARGE,      /* Block too large for free lists */
  po_error_MEM_FREEING_FOREVER_BLOCK,/* Cannot free forever allocated block */

  po_error_LIST_CORRUPT = 200,       /* Linked list is corrupt (po_list) */

  po_error_HASH_NOT_POWER_OF_2 = 300,/* Hash size not a positive power of 2 */
  po_error_HASH_NODE_NOT_IN_TABLE,   /* Node not in table */

  po_error_FUNC_BAD_PRIORITY = 400, /* Priority level out of range */
  po_error_FUNC_INVALID_RAISE_PRI,  /* raisepri() called with lower priority */

  po_error_SIG_POST_OUT_OF_RANGE = 500, /* hashSize!=2^n, post out of range */
  po_error_SIG_ATTACH_OUT_OF_RANGE,     /* Same but attach sig out of range */
  po_error_SIG_GROUP_OUT_OF_RANGE,      /* Group index out of range */
  po_error_SIG_CORRUPT_HANDLE,          /* Corrupt or uninitialized handle */
  po_error_SIG_MULTIPLE_ATTACH,         /* Attaching handle already active */

  po_error_TIM_CLOCK_OUT_OF_RANGE = 600, /* Clock index out of range */

  po_error_LOG_SIZE_NOT_POWER_OF_2 = 700,/* Log buffer not power of 2 */

  po_error_CANNOT_CREATE_SWI = 1100      /* Failed to create SWI */
};

/*-GLOBAL-
 * Error handling
 */
static inline void po_error(int error)
{
  po_target_error(error);
}

#endif // po_error__H
