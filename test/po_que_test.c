/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Test for po_sig module.
 */

#include <portos.h>
#include <po_display.h>
#include <miscLib.h>
#include <target.h>

static int Errors = 0;
po_queue_Queue MyQueue = po_queue_INIT(&MyQueue, 1, &po_memory_RegionDefault);

#define MAX_IDs 20
static int IDs[MAX_IDs];
static int IDcounter = 0;

int Done = 0;

/* Priority function
 */
static void myfunc(po_priority(priority), int priority, int id)
{
  if ( id < MAX_IDs/2 )
    myfunc(po_queue(&MyQueue), (priority+5)%po_function_NUM_PRI_LEVELS, id+1);
  else if ( id < MAX_IDs-1 )
    myfunc(po_queue(&MyQueue), (priority+3)%po_function_NUM_PRI_LEVELS, id+1);
  else {
    Done = 1;
  }
  po_log("func %d pri %d | ", id, priority);
  IDs[IDcounter++] = id;
  
  po_queue_next(&MyQueue);
}

/* Test queue
 */
int test_queue(void)
{
  int i;

  po_log("\nTESTING priority functions inserted in queues\n", 0, 0);
  
  myfunc(po_queue(&MyQueue), 1, 0);
  po_log("\n", 0, 0);

  for ( i = 0 ; i < MAX_IDs ; i++ ) Errors += (IDs[i] != i);

  // Still allocated memory
  po_memory_display(&po_memory_RegionDefault);

  // Success?
  if ( Errors > 0 ) {
    po_log("There were %d errors\n", Errors, 0);
    return -1;
  } else {
    po_log("SUCCESS: queue test\n", 0, 0);
    return 0;
  }
}
