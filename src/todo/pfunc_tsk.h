/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * This module is the pfunc scheduler.
 * A bitmap is used as a database of scheduled priority functions.
 */

#ifndef po_function_H
#define po_function_H

#include <po_sys.h>
#include <po_log.h>
#include <po_ti.h>

/* Number of priority levels. Reducing this number could save a small
 * amount of resources, and could speed up the bitmap search on some
 * platforms.
 */
enum {
  #if po_TARGET_TI_BIOS
  po_function_PRIORITY_LEVELS = po_ti_SWI_LEVELS
  #else
  po_function_PRIORITY_LEVELS = 32
  #endif
};

/* pfunc type
 */
typedef void (*po_function_Function)(void *message);

/* pfunc dynamic node. This node is allocated for each pfunc call.
 * When the pfunc is executed, the node is freed.
 */
typedef struct _po_function_Node {
  // MUST BE FIRST
  struct _po_function_Node *next;           // simple linked list

  po_function_Function func;                // pfunc function
  void *message;                      // pfunc message
} po_function_Node;

/* Data structure per environment
 */
typedef struct _po_function_Environ {
  // Current priority level
  volatile int currpri;

  // Maximum priority level in the database.
  // The highest priority in the bitmap is remembered for improved
  // performance.
  volatile int maxpri;
  
  // Mem alloc indexes
  struct {
    int node;
  } memIndex;

  // Priority database: 1 bit and one linked list per priority level.
  volatile unsigned bitmap;
  po_function_Node * volatile bitmapP[po_function_PRIORITY_LEVELS];

} po_function_Environ;

extern po_function_Environ *po_function_Env;

/* Reports that a priority function has been effectively scheduled.
 */
#ifndef po_function_reportScheduled
void po_function_reportScheduled(int priority);
#endif

/* Restore priority function context after HWI
 */
#ifndef po_function_context
static inline void po_function_context(void);
#endif

/* Report that priority restore phase has started
 */
#ifndef po_function_reportRestore
static inline void po_function_reportRestore(void);
#endif

/* Report that the priority level has been raised
 */
#ifndef po_function_reportRaise
static inline void po_function_reportRaise(int priority);
#endif

/* Report that the priority level has been lowered
 */
#ifndef po_function_reportLower
static inline void po_function_reportLower(int priority);
#endif

/*-GLOBAL-INTERNAL-
 * Set current priority level: should only be used to restore
 * priority level after preemption
 */
static inline int po_function_setpri(int priority)
{
  return po_function_Env->currpri = priority;
}

/*-GLOBAL-INTERNAL-
 * Returns the max priority in the bitmap
 */
static inline int po_function_getmaxpri(void)
{
  return po_function_Env->maxpri;
}

/*-GLOBAL-INSERT-*/

/*-GLOBAL-
 * Execute all priority functions above prevpri level.
 * Or restore priority level after a priority raise.
 */
void po_function_priorityRestore(int prevpri)
;

/*-GLOBAL-
 * Call a pfunc. The priority of the pfunc is usually different from a
 * currently running pfunc. The new pfunc will start running immediately
 * if it's priority is higher. Otherwise, it will wait until higher
 * priority pfuncs are done.
 */
void po_function_call(po_function_Function func, int priority, void *message)
;

/*-GLOBAL-
 * For optimization and simplification purposes, some functions can raise
 * the priority level without calling a pfunc. In this case, the variables
 * in the stack are accessible (no need to malloc). The priority can only
 * be raised.
 * This function returns the previous priority level to be supplied to
 * po_function_priorityRestore() when restoring the priority level.
 * Basically, the code that needs to be protected with a mechanism similar
 * to "priority ceiling" in traditional RTOS (something like a semaphore),
 * should be wrapped with po_function_priorityRaise() and po_function_priorityRestore().
 */
int po_function_priorityRaise(int priority)
;

/*-GLOBAL-
 * Create environment. Returns a pointer to the environment.
 * If environ is NULL, the function allocates an environ in dynamic
 * memory. Otherwise, the supplied environ is initialized.
 */
po_function_Environ *po_function_create(po_function_Environ *environ)
;

/*-GLOBAL-
 * Displays the content of the bitmap database
 */
void po_function_bitmapDisplay(void)
;

/*-GLOBAL-INSERT-END-*/

/*-GLOBAL-
 * Returns the current priority level
 */
static inline int po_function_getpri(void)
{
  return po_function_Env->currpri;
}

/*-GLOBAL-
 * Resumes the execution of pfuncs (if some were postponed).
 * Call this from the software interrupt, or equivalent, that 
 * is supposed to handle postponed pfuncs (scheduled at hardware interrupt
 * level but postponed to run here).
 */
static inline void po_function_resume(void)
{
  po_function_priorityRestore(po_function_Env->currpri);
}

/*-GLOBAL-
 * For optimization purposes, some pfuncs can take a message over stack
 * or can return a result inside the message they received. In this case,
 * it is mandatory that the pfunc has a higher or equal priority than the
 * current (caller) pfunc. This function checks this, and if the condition
 * is not verified, the code prints an error and crashes.
 * The postpone state cannot be set either (ie, this pfunc cannot
 * be called from interrupt level).
 */
static inline void po_function_priorityCheck(int priority)
{
  if ( priority < po_function_Env->currpri ) {
    po_log_error(po_log_File, "po_function_priorityCheck: failed\n");
    po_crash();
  }
}

/*-GLOBAL-
 * Set current environment (e.g., task)
 */
static inline void po_function_setenv(po_function_Environ *environ)
{
  po_function_Env = environ;
}

/*-GLOBAL-
 * Get current environment (e.g., task)
 */
static inline po_function_Environ *po_function_getenv(po_function_Environ *environ)
{
  return po_function_Env;
}

#endif // po_function_H
