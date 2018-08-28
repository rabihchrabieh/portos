/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * This module is the pfunc scheduler.
 * A bitmap is used as a database of scheduled priority functions.
 */

#ifndef po_function_H
#define po_function_H

#include <po_cfg.h>
#include <po_sys.h>

/*-GLOBAL-
 */
#define po_priority_MAX      (po_function_NUM_PRI_LEVELS - 1)
#define po_priority_get      po_function_getpri
#define po_priority_raise    po_function_raisepri
#define po_priority_restore  po_function_restorepri

/*-GLOBAL-
 * Real time directives
 */
struct _po_function_ServiceHandle;
static struct _po_function_ServiceHandle* const po_priority =
  (struct _po_function_ServiceHandle*)0;

/* Priority function handle.
 */
typedef struct _po_function_Handle {
  struct _po_function_Handle *next;  /* MUST BE FIRST. Simple linked list */
  void (*func)(struct _po_function_Handle *pfhandle); /* entry scheduler */
  #if po_function_TRACK_NAME // DEBUG_MODE
  char *name;
  #endif // DEBUG_MODE
} po_function_Handle;

#if 0  // It used to be a priority function
typedef void (*po_function_ServiceFunc)(
    struct _po_function_ServiceHandle *po__srvhandle,
    struct _po_function_ServiceHandle *handle);
#else
typedef void (*po_function_ServiceFunc)(
    struct _po_function_ServiceHandle *handle);
#endif

/* Service information: this structure can be inherited and augmented by
 * other modules.
 */
typedef struct {
  /* Service function: called with a handle to the requested service */
  po_function_ServiceFunc func;
} po_function_Service;

/* Additional debugging info to track running priority functions.
 * Eventually this can be replaced with stack unwinding.
 */
typedef struct _po_function_TrackRunType {
  struct _po_function_TrackRunType *prev;
  void *funcname;
  int priority;
} po_function_TrackRunType;

/* Service handle: service provided to other modules.
 * This structure can be inherited and augmented by other modules.
 */
typedef struct _po_function_ServiceHandle {
  po_function_Service *service; /* MUST BE FIRST (overloaded). service info */
  po_function_Handle *pfhandle; /* handle of priority function call */
  int priority;                 /* priority level of priority function */
} po_function_ServiceHandle;

/* Data structure per environment
 */
typedef struct _po_function_Environ {
  // Current priority level
  volatile int currpri;

  // Maximum priority level in the database.
  // The highest priority in the bitmap is remembered for improved
  // performance.
  volatile int maxpri;
  volatile po_function_Handle *maxnode;

  // This is to track running functions
  #if po_DEBUG // DEBUG_MODE
  po_function_TrackRunType *trackrun;
  #endif // DEBUG_MODE

  // Priority database: 1 bit and one linked list per priority level.
  volatile unsigned bitmap;
  po_function_Handle * volatile bitmapP[po_function_NUM_PRI_LEVELS];

} po_function_Environ;

extern po_function_Environ po_function_Env;

/*-GLOBAL-
 * Returns current priority level
 */
static inline int po_function_getpri(void)
{
  return po_function_Env.currpri;
}

/* Set current priority level: should only be used to restore
 * priority level after preemption
 */
static inline int po_function_setpri(int priority)
{
  return po_function_Env.currpri = priority;
}

/* Returns the max priority in the bitmap
 */
static inline int po_function_getmaxpri(void)
{
  return po_function_Env.maxpri;
}

/*-GLOBAL-INSERT-*/

/*-GLOBAL-
 * A version of restorepri that is called from a SWI or equivalent.
 */
void po_function_resume(void)
;

/*-GLOBAL-
 * Execute all priority functions above prevpri level.
 * Or restore priority level after a priority raise.
 */
void po_function_restorepri(int prevpri)
;

/*-GLOBAL-
 * Schedule a priority function call that we are sure it will not be
 * immediately called (for its priority is not higher than the current
 * priority).
 */
void po_function_later(po_function_Handle *pfhandle, int priority)
;

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
;

/*-GLOBAL-
 * Schedules priority functions or calls the requested service.
 */
void po_function_service(
    po_function_Handle *pfhandle,
    po_function_ServiceHandle *srvhandle,
    void (*func_entry_scheduler)(po_function_Handle*),
    int priority)
;

/*-GLOBAL-INSERT-END-*/

/* Enter immediate function call. It returns a state that should be supplied
 * to the exit routine.
 */
static inline int po_function_enternow_(int priority)
{
  int prevpri = po_function_Env.currpri;
  #if po_DEBUG > 1 // DEBUG_MODE
  if ( priority < 0 || priority >= po_function_NUM_PRI_LEVELS )
    po_error(po_error_FUNC_BAD_PRIORITY);
  #endif // DEBUG_MODE
  po_function_Env.currpri = priority; 
  po_function_reportsched(priority);
  return prevpri;
}

/* Enter immediate function call. It returns a state that should be supplied
 * to the exit routine.
 */
static inline void po_function_exitnow_(int prevpri)
{
  po_function_restorepri(prevpri);
}

#if po_DEBUG // DEBUG_MODE

#define po_function_trackrunenter(priority_, funcname_) \
  po_function_TrackRunType po__trackrun;                \
  po__trackrun.funcname = (void*)(funcname_);		\
  po__trackrun.priority = (priority_);			\
  po__trackrun.prev = po_function_Env.trackrun;		\
  po_function_Env.trackrun = &po__trackrun;

#define po_function_trackrunexit()                      \
  po_function_Env.trackrun = po__trackrun.prev;

#define po_function_enternow(priority, funcname)	\
  int prevpri = po_function_enternow_(priority);	\
  po_function_trackrunenter(priority, funcname);

#define po_function_exitnow()                           \
  po_function_trackrunexit();                           \
  po_function_exitnow_(prevpri);

#else // NON_DEBUG_MODE_START

#define po_function_enternow(priority, funcname)	\
  int prevpri = po_function_enternow_(priority);

#define po_function_exitnow()			        \
  po_function_exitnow_(prevpri);

#endif // NON_DEBUG_MODE_END

/* Calls immediately the scheduler of the priority function, which means
 * the priority function must have equal priority level to current priority
 * level. No change in priority level takes place.
 */
static inline void po_function_callschedulerentry(po_function_Handle *pfhandle)
{
  #if po_DEBUG // DEBUG_MODE
  po_function_Environ *env = &po_function_Env;
  #if !po_function_TRACK_NAME
  po_function_trackrunenter(env->currpri, pfhandle->func);
  #else
  po_function_trackrunenter(env->currpri, pfhandle->name);
  #endif
  #endif // DEBUG_MODE

  pfhandle->func(pfhandle);
  po_free(pfhandle);

  #if po_DEBUG // DEBUG_MODE
  po_function_trackrunexit();
  #endif // DEBUG_MODE
}

/* Calls the priority function if priority level is above current one.
 * Otherwise, it schedules it for later call.
 */
static inline void po_function_call(po_function_Handle *pfhandle, int priority)
{
  if ( priority > po_function_getpri() ) {
    #if !po_function_TRACK_NAME
    po_function_enternow(priority, pfhandle->func);
    #else
    po_function_enternow(priority, pfhandle->name);
    #endif
    
    pfhandle->func(pfhandle);
    po_free(pfhandle);
    po_function_exitnow();
  } else {
    po_function_later(pfhandle, priority);
  }
}

/* Postpones all priority function calls inside a HWI. Any calls to
 * priority functions inside HWI should be wrapped with HWI_enter
 * and HWI_exit. Designed to allow for nested calls.
 */
static inline void po_function_enterhwi(void)
{
  // No need to lock interrupts: state will be returned to original
  // by any preempting HWI
  po_function_Env.currpri += (po_function_NUM_PRI_LEVELS+1);
}

/* Restores priority functions context (unless in a nested level). Any
 * calls priority functions inside HWI should be wrapped with HWI_enter
 * and HWI_exit.
 */
static inline void po_function_exithwi(void)
{
  // No need to lock interrupts: state will return to original
  // by any preempting HWI
  po_function_Env.currpri -= (po_function_NUM_PRI_LEVELS+1);
  if ( po_function_Env.maxpri > po_function_Env.currpri )
    po_function_context();
}

/*-GLOBAL-
 * Entering and exiting interrupts:
 * po_interrupt_enter() and po_interrupt_exit() should wrap any
 * calls to priority functions inside HWI.
 */
#define po_interrupt_enter()   (po_function_enterhwi())
#define po_interrupt_exit()    (po_function_exithwi())

/*-GLOBAL-
 * Sets the argument used to carry the service handle to a priority function.
 */
static inline void po_function_setsrv(po_function_ServiceHandle *srvhandle, po_function_Service *service)
{
  srvhandle->service = service;
}

#endif // po_function_H
