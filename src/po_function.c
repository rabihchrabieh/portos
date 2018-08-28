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
 * priority p is organized as follows:
 *
 *      L(p).first-> First-->Second-->Third-->Last-->NULL
 *      L(p).last -> Last-->NULL
 *
 * Empty list
 *      L(p).first = NULL
 *      L(p).last -> (H*)&L(p).first (filling last automatically fills first)
 */

#include <po_sys.h>
#include <po_memory.h>
#include <po_lib.h>
#include <po_function.h>

/* TODO: move into target area. Issue: not all variables/functions end up in these segments. May need to issue many more of these pragmas.
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

  // trackrun
  #if po_DEBUG // DEBUG_MODE
  NULL,
  #endif // DEBUG_MODE

  // bitmap
  0
};

/* Mapping of priority level to priority bit in integer bitmap
 */
#define po_function_PRI_BIT(priority) (priority)
#define po_function_PRI_MAX(bitmap) (po_function_PRI_BIT(po_lib_msb(bitmap)))

/*-GLOBAL-
 * Execute all priority functions above prevpri level.
 * Or restore priority level after a priority raise.
 */
void po_function_restorepri(int prevpri)
{
  po_function_Environ *env = &po_function_Env;
  int protectState, maxpri;
  po_function_Handle *first, *next;
  unsigned bitmap;

  // Check if new priority functions were installed and that have priorities
  // above prevpri.

  // We play a trick to avoid locking interrupts. First we set
  // priority back to previous priority. Then we check max priority and
  // decide to reraise priority if needed.
  env->currpri = prevpri;
  po_emulateirupt();

  // Note that any preempting process that occurs here will take care of
  // everything above current priority.

  maxpri = env->maxpri;
  po_emulateirupt();

  if ( maxpri <= prevpri ) {
    // More common case
    // Resume previous priority function
    po_emulateirupt();
    return;
  }

  // If we reach this point, it means there are, or were, some higher priority
  // functions installed that we need to process. But they may have
  // vanished in the meantime.

  do {
    po_function_BitmapList *list = &env->list[maxpri];
    po_emulateirupt();

    // Reraise priority to local maxpri, which may be different from
    // global env->maxpri that could have changed in a volatile manner.
    // In rare cases, it is better to use env->maxpri, but
    // in most cases it is more optimal to use local maxpri.
    env->currpri = maxpri;
    po_emulateirupt();

    // Get first node at this current priority level to be sure it didn't
    // vanish.
    first = list->first;
    po_emulateirupt();

    while ( first ) {
      // Start a new linked list so that we can work on the current one
      // without locking interrupts (in particular, the last node cannot
      // be freed while last pointer is pointing at it).
      po_emulateirupt();
      list->first = NULL;
      po_emulateirupt();
      list->last = (po_function_Handle*)&(list->first);

      // For tracking purposes, we can have a temporary first pointer.
      // list->first_tmp = first; (???) (Goes back automatically to NULL???)

      do {
	// Remember next position before first node is freed (or reused in
	// static cases).
	po_emulateirupt();
	next = first->next;
	po_emulateirupt();

	// For tracking purposes (current function has no node in list)
	#if po_DEBUG
	list->first_tmp = next;
	#endif
	po_emulateirupt();

	// Call function.
	po_function_callschedulerentry(first);
	po_emulateirupt();
	
	first = next;
	po_emulateirupt();
      } while ( first );

      // This one is not so needed here.
      #if 1
      first = list->first;
      #else
      break;
      #endif
      po_emulateirupt();
    }
    po_emulateirupt();

    // Relower currpri before finding new maxpri (otherwise it can change
    // just after we find new value).
    env->currpri = prevpri;
    po_emulateirupt();

    // Get first node again at this lower priority level. If it's NULL then
    // we know for sure there are no nodes left at this level and we can
    // move down to lower level.
    first = list->first;
    po_emulateirupt();

    // Check if there are really no nodes left at this level.
    if ( !first ) {
      // Find new maxpri
      po_emulateirupt();

      // Bit clear instruction must be atomic
      protectState = po_interrupt_disable();
      po_lib_BIT_CLR(env->bitmap, po_function_PRI_BIT(maxpri));
      po_interrupt_restore(protectState);

      po_emulateirupt();
      bitmap = env->bitmap;
      po_emulateirupt();
      maxpri = po_function_PRI_MAX(bitmap);
      po_emulateirupt();

      if ( maxpri <= prevpri ) {
	// We can't set env->maxpri to a value < prevpri because
	// of interrupting processes (disabling interrupts may have
	// advantages in some cases).
	po_emulateirupt();
	env->maxpri = prevpri;
	po_emulateirupt();
	break;
      }
      po_emulateirupt();

      env->maxpri = maxpri;
      po_emulateirupt();
    }
    po_emulateirupt();

  } while ( 1 );
}

/*-GLOBAL-
 * Schedule a priority function call that we are sure it will not be
 * immediately called (its priority is not higher than the current
 * priority).
 */
void po_function_later(po_function_Handle *pfhandle, int priority)
{
  // Priority of this function is less than the currently running one.
  // Insert in waiting list.

  po_function_Environ *env = &po_function_Env;
  int priorityBit = po_function_PRI_BIT(priority);
  int protectState;

  pfhandle->next = NULL;
  po_emulateirupt();

  protectState = po_interrupt_disable();
  if ( priority > env->maxpri ) {
    env->maxpri = priority; // We cannot use maxpri in no-lock ISR version (?)
  }
  po_lib_BIT_SET(env->bitmap, priorityBit);  // Critical if non-atomic
  env->list[priority].last->next = pfhandle;
  env->list[priority].last = pfhandle; // Do this first if non-lock ISR

  // If no-lock ISR (RE-CHECK) (there may be issues with non-atomic ptr load)
  #if 0
  {
    po_function_Handle *tmp, *before = env->list[priority].last;
    // Preemptions here precede us
    env->list[priority].last = pfhandle;
    // Preemptions here succeed us
    while ( (tmp = before->next) != NULL )
      before = tmp; // Ignore all that preceded us
    before->next = pfhandle;
  }
  #endif

  po_interrupt_restore(protectState);

  po_emulateirupt();
  po_function_reportsched(priority);
}

/*-GLOBAL-
 * Calls the priority function if priority level is above current one.
 * Otherwise, it schedules it for later call.
 * NOTE: po_function_callschedulerentry() does not save and restore
 * prevpri (via restorepri). So we need to emulate what po_prep.h does.
 */
void po_function_call(po_function_Handle *pfhandle, int priority)
{
  int currpri = po_function_getpri();

  if ( (unsigned)priority > (unsigned)currpri ) {
    #if !po_function_TRACK_NAME
    po_function_enternow(priority, pfhandle->func);
    #else
    po_function_enternow(priority, pfhandle->name);
    #endif
    
    pfhandle->func(pfhandle);
    //po_free(pfhandle);
    po_function_exitnow();

  } else {
    po_function_later(pfhandle, priority);
    if ( currpri < 0 ) po_function_context(); // Start context
  }
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
    po_function_call(pfhandle, priority);
    return;
  }

  /* Service request */
  srvhandle->pfhandle = pfhandle;
  srvhandle->priority = priority;

  /* Call service (this used to be a priority function call) */
  srvhandle->service->func(srvhandle);
}

/*-GLOBAL-
 * Module initialization.
 */
void po_function_init(void)
{
  int i;
  for ( i = 0 ; i < po_function_NUM_PRI_LEVELS ; i++ ) {
    po_function_Env.list[i].last =
      (po_function_Handle*)&po_function_Env.list[i].first;
    po_function_Env.list[i].first = NULL;
    #if po_DEBUG
    po_function_Env.list[i].first_tmp = NULL;
    #endif
  }
}
