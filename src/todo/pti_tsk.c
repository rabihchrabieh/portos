/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Cf. po_ti.h for a module description.
 */

#include <std.h>
#include <sem.h>
#include <swi.h>
#include <tsk.h>
#include <po_sys.h>
#include <po_ti.h>
#include <po_function.h>

/* Main data
 */
po_target_DataType po_target_Data;

/*-GLOBAL-
 * Sets the priority functions context (in SWI ot TSK)
 */
void po_ti_context(void)
{
  int maxpri = po_function_getpri_max();
  int prevpri = po_function_getpri_prev();
  if ( maxpri > prevpri ) {
    if ( maxpri >= po_ti_SWI_MINPRI ) {
      // SWI region
      int protectState = po_interrupt_disable();
      if ( !po_target_Data.swi.waiting ) {
        int level = po_target_Data.swi.level;
        po_target_Data.swi.level = level + 1;
        po_target_Data.swi.waiting = 1;  // Could also be set to level + 1...
        po_interrupt_restore(protectState);
        SWI_post(po_target_Data.swi.handle[level]);
      } else {
        po_interrupt_restore(protectState);
      }
    } else {
      // TSK region
      // If not already called by another HWI. It should not be disastrous
      // if anyway we end up calling it twice.
	  // WRONG: This should call a SWI
      if ( SEM_count(po_target_Data.task.sem) < 1 )
        SEM_post(po_target_Data.task.sem);
    }
  } else {
    po_function_setpri(prevpri);
  }
}

/* SWI start function
 */
static void po_ti_swi(void)
{
  int protectState;

  /* Resume priority functions scheduling */
  po_function_resume();

  protectState = po_interrupt_disable();
  po_target_Data.swi.level--;
  po_interrupt_restore(protectState);

  #if 0
  if ( po_function_getpri() <= po_ti_TSK_MAXPRI )
    po_ti_controller_swi();
  #endif
}

/*-GLOBAL-
 * One time init
 */
void po_ti_swi_init(void)
{
  SWI_Attrs attrS = {0};
  int i;

  int heapSize = 100000;
  int region = 0;
  int maxBlockShift = 12;
  int maxBlockSize = 1<<maxBlockShift;

  po_target_Data.swi.waiting = 0;
  po_target_Data.swi.level = 1;

  // Create one memory region (mandatory for pmem to work properly)
  mlCreateMemRegion(region, heapSize, maxBlockSize);

  // Init pfunc module
  po_target_Data.swi.environ = po_function_create(NULL);
  po_function_setenv(po_target_Data.swi.environ);

  /* Assume interrupt level */
  po_function_postpone();

  /* Create SWI */
  for ( i = 0 ; i <= po_ti_SWI_MAXPRI-po_ti_SWI_MINPRI ; i++ ) {
    attrS.fxn = (SWI_Fxn)po_ti_swi;
    attrS.priority = i;
    po_target_Data.swi.handle[i] = SWI_create(&attrS);
    if ( !po_target_Data.swi.handle[i] ) {
      po_log_error(po_log_File, "Cannot create Portos SWI\n");
      po_crash();
    }
  }
}

/* Returns the task's mode */
static int inline po_ti_taskMode(TSK_Handle task)
{
  return task->kobj.mode;
}

static char po_ti_Names[][9] = {
  "portos1 ",
  "portos2 ",
  "portos3 ",
  "portos4 ",
  "portos5 ",
  "portos6 ",
  "portos7 ",
  "portos8 ",
  "portos9 ",
  "portos10",
  "portos11",
  "portos12",
  "portos13",
  "portos14",
  "portos15"
};

po_ti_check()
{
  int i = 1;
  while ( po_target_Data.task.handle[i] ) {
    TSK_Stat s;
    TSK_stat(po_target_Data.task.handle[i], &s);
    if ( s.mode == TSK_TERMINATED ) break;
    
    if ( TSK_getpri(po_target_Data.task.handle[i]) <=
	 TSK_getpri(po_target_Data.task.handle[i-1]) ) {
      // This tasks may have disappeared while we are looping. Check again,
      // or lock interrupts?
      if ( po_target_Data.task.handle[i] )
        po_log_printf(po_log_File, "XXXXXXXXXXX\n");
    }
    i++;
  }
}

/* Task start function
 */
static void po_ti_task(void)
{
  po_log_printf(po_log_File, "%x New task", TSK_self());
  po_log_printf(po_log_File, "    T %d %d", po_function_getpri(), TSK_getpri(TSK_self()));
  
  /* Resume priority functions scheduling */
  po_function_resume();
}

/* Controller task only needed to create and delete tasks, which
 * cannot be done from SWI, unfortunately.
 */
static void po_ti_controllerTask(void)
{
  TSK_Attrs attr = {0};
  int pri_max, pri_prev;

  attr.exitflag = 1;
  attr.stacksize = 4096;
  attr.stack = NULL;

  while ( 1 ) {
    /* Wait until HWI or SWI call priority functions */
    SEM_pend(po_target_Data.task.sem, SYS_FOREVER);

    /* Signal to SWI controller that the controller task is running */
    po_target_Data.task.controllerRunning = 1;

    pri_max = po_function_getpri_max();

    /* If a task is waiting to start, it can handle this. Simply raise its
     * priority, if needed */
    /* This code is duplicated in case the SWI occurs a bit late and can't
     * change priority level */
    if ( po_target_Data.task.waiting ) {
      /* The test is only meant to improve performance in case set priority
       * is slow */
      int index = po_target_Data.task.index;
      if ( TSK_getpri(po_target_Data.task.handle[index-1]) < pri_max )
        TSK_setpri(po_target_Data.task.handle[index-1], pri_max);
      
      po_ti_check();

    } else {
      /* Delete terminated tasks */
      while ( po_ti_taskMode(po_target_Data.task.handle[po_target_Data.task.index-1]) ==
	      TSK_TERMINATED ) {
        po_target_Data.task.index--;
        TSK_delete(po_target_Data.task.handle[po_target_Data.task.index]);
        po_target_Data.task.handle[po_target_Data.task.index] = NULL;
      }
      /* Delete one more if requested from SWI controller */
      if ( po_target_Data.task.terminated ) {
        po_target_Data.task.terminated = 0;
        po_target_Data.task.index--;
        TSK_delete(po_target_Data.task.handle[po_target_Data.task.index]);
        po_target_Data.task.handle[po_target_Data.task.index] = NULL;
      }

      pri_prev = po_function_getpri_prev();

      /* Spawn a new task if priority is higher */
      if ( pri_max > pri_prev ) {
        int index = po_target_Data.task.index;
        attr.name = po_ti_Names[index];
        attr.priority = pri_max;
        po_target_Data.task.handle[index] = TSK_create((Fxn)po_ti_task, &attr);
        if ( !po_target_Data.task.handle[index] ) {
          po_log_error(po_log_File, "Could not start portos task %s\n", attr.name);
          po_crash();
        }

        po_ti_check();

        po_target_Data.task.index = index + 1;
        po_target_Data.task.waiting = 1; // Set this last in case SWI is activated
      }
    }

    /* Signal to SWI controller that the controller task is no longer
     * running */
    po_target_Data.task.controllerRunning = 0;
  }
}

/*-GLOBAL-
 * Main controller for tasks
 */
void po_ti_controller_swi(void)
{
  int call_controllerTask = 0;
  int index = po_target_Data.task.index;
  int pri_max = po_function_getpri_max();

  /* If a task is waiting to start, it can handle this. Simply raise its
   * priority, if needed */
  /* Note that if a task is waiting, we are sure that the index is properly
   * pointing at this task: all terminated tasks must have been deleted */
  if ( po_target_Data.task.waiting ) {
    /* The test is only meant to improve performance in case set priority
     * is slow */
    if ( TSK_getpri(po_target_Data.task.handle[index-1]) < pri_max )
      TSK_setpri(po_target_Data.task.handle[index-1], pri_max);
      
    po_ti_check();

  } else {
    int pri_prev = po_function_getpri_prev();

    if ( !po_target_Data.task.controllerRunning ) {
      /* This code has already executed in the previous SWI if the controller
       * task is running. Otherwise, it is safe to access index */

      /* In case the current task did not have time to set its priority level
       * properly, and also in order to prevent an unnecessary context switch,
       * we set the priority level here too.
       * But we do not set it if the task is about to terminate and its
       * priority could coincide with that of the lower task. In such a case,
       * we delete the task to prevent collision over the stack in case the
       * lower priority one runs first! */
      if ( index && TSK_getpri(po_target_Data.task.handle[index-1]) > pri_prev ) {
        if ( (index == 1 && pri_prev >= 0) ||
	     TSK_getpri(po_target_Data.task.handle[index-2]) < pri_prev ) {
          po_log_printf(po_log_File, "Controller change pri %d -> %d",
	              TSK_getpri(po_target_Data.task.handle[index-1]), pri_prev);
          TSK_setpri(po_target_Data.task.handle[index-1], pri_prev);
        } else {
          /* Delete task since it is about to terminate, and to avoid
	   * collision */
          if ( po_ti_taskMode(po_target_Data.task.handle[index-1]) != TSK_TERMINATED &&
               po_target_Data.task.terminated == 0 ) {
            po_target_Data.task.terminated = 1;
	  }
	  /* Delete this task to avoid collisions over stack */
          call_controllerTask = 1;
        }
      }
      po_ti_check();
    }

    /* Spawn a new task if priority is higher */
    if ( pri_max > pri_prev ) {
      call_controllerTask = 1;
    } else {
      /* Return to previous task. We need to reset current priority */
      po_log_printf(po_log_File, "Controller curr %d prev %d", po_function_getpri(),
		  pri_prev);
      po_function_setpri(pri_prev);
    }
  }
  /* Call controller task if needed */
  if ( call_controllerTask )
    if ( SEM_count(po_target_Data.task.sem) < 1 )
      SEM_post(po_target_Data.task.sem);
}

/*-GLOBAL-
 * One time init of task version
 */
void po_ti_task_init(void)
{
  TSK_Attrs attrT = {0};

  int heapSize = 100000;
  int region = 0;
  int maxBlockShift = 12;
  int maxBlockSize = 1<<maxBlockShift;

  // Create one memory region (mandatory for pmem to work properly)
  mlCreateMemRegion(region, heapSize, maxBlockSize);

  // Create environment for task
  po_target_Data.task.environ = po_function_create(NULL);
  po_function_setenv(po_target_Data.task.environ);

  /* Assume interrupt level */
  po_function_postpone();

  /* Spawned tasks index and waiting flag */
  po_target_Data.task.controllerRunning = 0;
  po_target_Data.task.index = 0;
  po_target_Data.task.waiting = 0;
  po_target_Data.task.terminated = 0;

  /* Create semaphore */
  po_target_Data.task.sem = SEM_create(0, NULL);
  if ( !po_target_Data.task.sem ) {
    po_log_error(po_log_File, "Cannot create Portos controller semaphore\n");
    po_crash();
  }

  /* Create controller task */
  attrT.name = "portos_ctrl";
  attrT.priority = TSK_MAXPRI;
  attrT.exitflag = 1;
  attrT.stacksize = 512;
  attrT.stack = NULL;
  po_target_Data.task.controller = TSK_create((Fxn)po_ti_controllerTask, &attrT);
  if ( !po_target_Data.task.controller ) {
    po_log_error(po_log_File, "Could not start portos controller task (portos_ctrl)\n");
    po_crash();
  }
}

/* -GLOBAL-
 * One time init of module
 */
void po_ti_init(void)
{
  po_ti_swi_init();
  //po_ti_task_init();
  po_function_setenv(po_target_Data.swi.environ);
}
