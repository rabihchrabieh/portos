/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Exercise all tests.
 */

#if __TI_COMPILER_VERSION__
#include <std.h>
#include <tsk.h>
#endif

#include <portos.h>

int test_lib(void);
int test_size2index(void);
int test_randomMalloc(void);
int test_randomPfunc(void);
int test_randomHash(void);
int test_randomSignals(void);
int test_queue(void);

// Called in task context on real systems
void mainfunc(void)
{
  int failure = 0;

  // po_lib_test
  failure |= test_lib();
  // po_memory_test
  failure |= test_size2index();
  failure |= test_randomMalloc();
  // po_function_test
  failure |= test_randomPfunc();
  // po_hash_test
  failure |= test_randomHash();
  // po_signal_test
  failure |= test_randomSignals();
  //po_queue test
  failure |= test_queue();

  if ( failure )
    po_log("\nFAILURE: some tests have failed\n", 0, 0);
  else
    po_log("\nSUCCESS: all tests have succeeded\n", 0, 0);
}

extern po_memory_Region Region1, Region2;

// Define a vector for accessing regions
po_memory_Region *RegionVector[3] =
  {&po_memory_RegionDefault, &Region1, &Region2};

int main()
{
  po_init();

  RegionVector[0] = &po_memory_RegionDefault;
  RegionVector[1] = &Region1;
  RegionVector[2] = &Region2;

  #if _TI_
    #if !_55_  // Bug for _55_ dynamic task creation
    {
      TSK_Attrs attr = {0};
      TSK_Handle MainTask;

      // Create tasks
      attr.name = "MainTask";
      attr.priority = 2;
      attr.stacksize = 1024;
      attr.stack = NULL;
      attr.initstackflag = 1;
      MainTask = TSK_create((Fxn)mainfunc, &attr);
      if ( !MainTask ) SYS_abort("Could not create profiler task");
    }
    #endif
  #else
    mainfunc();
  #endif

  return 0;
}
