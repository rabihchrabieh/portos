// ATTEMPT TO OPTIMIZE.
// But it's failing po_func_test.c
// Moreover, there is certainly an issue with updating maxpri and maxnode
// via the trick that sets maxpri to -1. In the meantime interrupts may
// fill up maxnode. So we need another strategy there. Or we need to copy
// everything in maxnode into database (things that interrupts may have
// inserted).

/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Cf. po_function.h for a module description.
 *
 * In this version of the scheduler the database is a bitmap. One bit
 * per priority level indicating if there are awaiting priority functions
 * at this level. If there are, then the priority functions are stored in
 * a simple linked list. In order to insert at the end of the list and
 * retrieve from the beginning of the list, the simple linked list for
 * priority P(i) is organized as follows:
 *
 *      P(i)-->Last-->First-->Second-->Third-->Last (circular)
 *
 * In other words, it is a circular linked list and we maintain a pointer
 * at the Last element. We insert at the position of Last. We retrieve
 * from the position of First. The goal is to have a highly efficient
 * design both in terms of memory and speed. A double linked list may
 * be more efficient in some cases. To be studied.
 */

#include <po_sys.h>
#include <po_memory.h>
#include <po_lib.h>
#include <po_function.h>

/*
#if _TI_
  #pragma DATA_SECTION(po_function_Env,        ".po_data")
  #pragma CODE_SECTION(po_function_resume,     ".po_code")
  #pragma CODE_SECTION(po_function_restorepri, ".po_code")
  #pragma CODE_SECTION(po_function_later,      ".po_code")
  #pragma CODE_SECTION(po_function_raisepri,   ".po_code")
  #pragma CODE_SECTION(po_function_service,    ".po_code")
#endif
*/

/* Currently active environment
 */
po_NEARFAR po_function_Environ po_function_Env = {
  // currpri
  -1,

  // maxpri
  -1,

  // maxnode
  NULL,

  // trackrun
  #if po_DEBUG // DEBUG_MODE
  NULL,
  #endif // DEBUG_MODE

  // bitmap
  0,

  // bitmapP
  {0}
};

/* Mapping of priority level to priority bit in integer bitmap
 */
#define po_function_PRI_BIT(priority) (priority)
#define po_function_PRI_MAX(bitmap) (po_function_PRI_BIT(po_lib_msb(bitmap)))

/*-GLOBAL-
 * A version of restorepri that is called from a SWI or equivalent.
 */
void po_function_resume(void)
{
  po_function_Environ *env = &po_function_Env;
  int prevpri = env->currpri;
  int protectState = po_interrupt_disable();
  po_function_reportrun();

  do {
    int maxpri = env->maxpri;
    po_function_Handle *first, *last;

    // Modify the currpri to the new pfunc priority. After
    // this statement, any preempting code will be responsible for
    // any pfunc whose priority is higher than this new priority.
    env->currpri = maxpri;

    // We can unprotect then protect again at this point

    first = (po_function_Handle*)env->maxnode;
    env->maxnode = NULL;

    if ( !first ) {
      // Max node was empty. Max is in bitmap
      // Remove first node from linked list
      last = env->bitmapP[maxpri];
      first = last->next;

      if ( first == last ) {
	unsigned bitmap;
	// Empty list after this node
	po_lib_BIT_CLR(env->bitmap, po_function_PRI_BIT(maxpri));
	// Find next max priority
	env->maxpri = -1; /* TEMPORARILY INCONSISTENT */
	po_interrupt_restore(protectState);
	bitmap = env->bitmap;
	maxpri = po_function_PRI_MAX(bitmap);
	protectState = po_interrupt_disable();
	if ( maxpri > env->maxpri ) env->maxpri = maxpri;

      } else {
	last->next = first->next;
      }
    } else {
      // Max node was not empty. Simply update maxpri value
      // Find next max priority
      unsigned bitmap;
      env->maxpri = -1; /* TEMPORARILY INCONSISTENT */
      po_interrupt_restore(protectState);
      bitmap = env->bitmap;
      maxpri = po_function_PRI_MAX(bitmap);
      protectState = po_interrupt_disable();
      if ( maxpri > env->maxpri ) env->maxpri = maxpri;
    }
    po_interrupt_restore(protectState);
	
    // Call function now after node has been removed to keep the
    // database consistent: a running pfunc has no node in database.
    po_function_callschedulerentry(first);

    protectState = po_interrupt_disable();
    
    // Check if a new pfunc was installed and that has a priority
    // above prevpri (installed by a higher priority process or
    // a preempting interrupt).
    // Note that in the test below, the "<=" means everything that was
    // installed by interrupt level or a higher priority level will not
    // execute now if it has equal priority to prevpri.
    if ( env->maxpri <= prevpri ) {
      // Most common case
      // Resume previous pfunc
      env->currpri = prevpri;
      po_function_reportdone();
      po_interrupt_restore(protectState);
      break;
    }

  } while ( 1 );  
}

/*-GLOBAL-
 * Execute all priority functions above prevpri level.
 * Or restore priority level after a priority raise.
 */
void po_function_restorepri(int prevpri)
{
  po_function_Environ *env = &po_function_Env;

  do {
    int protectState = po_interrupt_disable();
    int maxpri = env->maxpri;
    po_function_Handle *first, *last;

    // Check if a new pfunc was installed and that has a priority
    // above prevpri (installed by a higher priority process or
    // a preempting interrupt).
    // Note that in the test below, the "<=" means everything that was
    // installed by interrupt level or a higher priority level will not
    // execute now if it has equal priority to prevpri.
    if ( maxpri <= prevpri ) {
      // Most common case
      // Resume previous pfunc
      env->currpri = prevpri;
      po_interrupt_restore(protectState);
      break;
    }

    // Modify the currpri to the new max priority. After
    // this statement, any preempting code will be responsible for
    // any pfunc whose priority is higher than this new priority.
    env->currpri = maxpri;

    // We can unprotect then protect again at this point

    first = (po_function_Handle*)env->maxnode;
    env->maxnode = NULL;

    if ( !first ) {
      // Max node was empty. Max is in bitmap
      // Remove first node from linked list
      last = env->bitmapP[maxpri];
      first = last->next;

      if ( first == last ) {
	unsigned bitmap;
	// Empty list after this node
	po_lib_BIT_CLR(env->bitmap, po_function_PRI_BIT(maxpri));
	// Find next max priority
	env->maxpri = -1; /* TEMPORARILY INCONSISTENT */
	po_interrupt_restore(protectState);
	bitmap = env->bitmap;
	maxpri = po_function_PRI_MAX(bitmap);
	protectState = po_interrupt_disable();
	if ( maxpri > env->maxpri ) env->maxpri = maxpri;

      } else {
	last->next = first->next;
      }
    } else {
      // Max node was not empty. Simply update maxpri value
      // Find next max priority
      unsigned bitmap;
      env->maxpri = -1; /* TEMPORARILY INCONSISTENT */
      po_interrupt_restore(protectState);
      bitmap = env->bitmap;
      maxpri = po_function_PRI_MAX(bitmap);
      protectState = po_interrupt_disable();
      if ( maxpri > env->maxpri ) env->maxpri = maxpri;
    }
    po_interrupt_restore(protectState);

    // Call function now after node has been removed to keep the
    // database consistent: a running pfunc has no node in database.
    po_function_callschedulerentry(first);
    
  } while ( 1 );  
}

/*-GLOBAL-
 * Schedule a priority function call that we are sure it will not be
 * immediately called (for its priority is not higher than the current
 * priority).
 */
void po_function_later(po_function_Handle *pfhandle, int priority)
{
  po_function_Environ *env = &po_function_Env;
  int protectState;
  int priorityBit;

  // Priority of this pfunc is less than the currently running one.
  // Insert this pfunc in priority database.
  protectState = po_interrupt_disable();
  if ( priority > env->maxpri ) {
    // Replace max node
    po_function_Handle *tmp1;
    int tmp = env->maxpri;
    env->maxpri = priority;
    priority = tmp;
    tmp1 = (po_function_Handle*)env->maxnode;
    env->maxnode = pfhandle;
    pfhandle = tmp1;
    po_interrupt_restore(protectState);
    if ( !pfhandle ) {
      // We're done
      po_function_reportsched(priority);
      return;
    }
  } else {
    po_interrupt_restore(protectState);
  }

  // At this point, "pfhandle" and "priority" contain a non-max handle
  // Insert in bitmap
  priorityBit = po_function_PRI_BIT(priority);

  protectState = po_interrupt_disable();
  if ( !po_lib_BIT_TEST(env->bitmap, priorityBit) ) {
    // List is empty
    env->bitmapP[priority] = pfhandle;
    pfhandle->next = pfhandle;
    po_lib_BIT_SET(env->bitmap, priorityBit);
    po_interrupt_restore(protectState);
  } else {
    // List is not empty
    po_function_Handle *prev_last = (po_function_Handle*)env->bitmapP[priority];
    pfhandle->next = prev_last->next;
    prev_last->next = pfhandle;
    env->bitmapP[priority] = pfhandle;
    po_interrupt_restore(protectState);
  }

  po_function_reportsched(priority);
}

/*-GLOBAL-
 * For optimization and simplification purposes, a priority function can
 * raise its priority level without calling another priority function.
 * Memory requirements are decreased and speed is increased.
 * This function returns the previous priority level to be supplied to
 * po_function_restorepri when restoring the priority level.
 * Basically, the code that needs to be protected with a mechanism similar
 * to "priority ceiling" in traditional RTOS (something like a semaphore),
 * should be wrapped with po_function_raisepri and po_function_restorepri.
 * This method can replace the priority function method only if the previous
 * priority level is ALWAYS below than the new priority level.
 * E.g., if called from HWI, this function will fail and return an error.
 */
int po_function_raisepri(int priority)
{
  po_function_Environ *env = &po_function_Env;
  int prevpri = env->currpri;

  if ( priority >= env->currpri ) {
    env->currpri = priority;
  } else {
    po_error(po_error_FUNC_INVALID_RAISE_PRI);
  }
  return prevpri;
}

/*-GLOBAL-
 * Schedules priority functions or calls the requested service.
 */
void po_function_service(
    po_function_Handle *pfhandle,
    po_function_ServiceHandle *srvhandle,
    void (*func_entry_scheduler)(po_function_Handle*),
    int priority)
{
  pfhandle->func = func_entry_scheduler;

  if ( !srvhandle ) {
    /* Priority function call */
    po_function_later(pfhandle, priority);
    if ( !po_function_iscontext() ) { // or po_function_getpri() < 0
      /* No current priority function context running, start one */
      po_function_context();
    }
    return;
  }

  /* Service request */
  srvhandle->pfhandle = pfhandle;
  srvhandle->priority = priority;

  /* Call service (this used to be a priority function call) */
  srvhandle->service->func(srvhandle);
}

#if 0
/* Create environment.
 */
void po_function_create(void)
{
  po_function_Environ *environ = &po_function_Env;

  // Init curr priority level
  environ->currpri = -1;

  // Init bitmap
  environ->maxpri = -1;
  environ->bitmap = 0;

  // No running functions yet
  #if po_DEBUG // DEBUG_MODE
  environ->trackrun = NULL;
  #endif // DEBUG_MODE
}
#endif
