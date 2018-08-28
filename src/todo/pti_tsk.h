/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * This module is the interface to TI's BIOS.
 */

#ifndef po_ti_H
#define po_ti_H

#include <std.h>
#include <hwi.h>
#include <sem.h>
#include <tsk.h>
#include <po_log.h>

/* Priority levels boundaries
 */
enum {
  po_ti_TSK_MINPRI = 0,
  po_ti_TSK_MAXPRI = TSK_MAXPRI,
  po_ti_SWI_MINPRI = po_ti_TSK_MAXPRI + 1, // SWI level 0 (could be reserved)
  po_ti_SWI_MAXPRI = po_ti_SWI_MINPRI + SWI_MAXPRI // SWI level 14 
};

typedef struct {
  /* Main controller */
  struct {
    struct _po_function_Environ *environ;   // SWI supertask
    int waiting;  // SWI waiting to start
    int level;    // Current SWI level
    SWI_Handle handle[po_ti_SWI_MAXPRI-po_ti_SWI_MINPRI+1]; // SWIs
  } swi;
  /* Controller for spawning tasks */
  struct {
    struct _po_function_Environ *environ;   // TSK supertask
    TSK_Handle controller;   // Controller task
    SEM_Handle sem;          // Semaphore to awaken controller task
    int controllerRunning;   // Non-zero if controller task is running
    int index;               // Index of current task (highest one)
    volatile int waiting;    // Task waiting to start
    volatile int terminated; // Task has terminated
    TSK_Handle handle[po_ti_TSK_MAXPRI+1]; // Tasks
  } task;
} po_target_DataType;

extern po_target_DataType po_target_Data;

/*-GLOBAL-INLINE-
 * React to a priority restore start from priority function scheduler.
 */
static inline void po_function_reportRestore(void)
{
  if ( po_target_Data.swi.waiting )
    po_target_Data.swi.waiting = 0;
  else
    po_target_Data.task.waiting = 0;
}

/*-GLOBAL-INLINE-
 * React to a priority change report from priority function scheduler.
 */
static inline void po_function_reportLower(int priority)
{
  if ( priority <= po_ti_TSK_MAXPRI && priority >= 0 ) {
    /* Task level */
    TSK_setpri(TSK_self(), priority);
    po_ti_check();
  }
}

/*-GLOBAL-INLINE-
 * React to a priority change report from priority function scheduler.
 */
static inline void po_function_reportRaise(int priority)
{
  if ( priority <= po_ti_TSK_MAXPRI ) {
    /* Task level */
    TSK_setpri(TSK_self(), priority);
    po_ti_check();
  }
}

/*-GLOBAL-INSERT-*/

/*-GLOBAL-
 * Sets the priority functions context (in SWI ot TSK)
 */
void po_ti_context(void)
;

/*-GLOBAL-
 * One time init
 */
void po_ti_swi_init(void)
;

/*-GLOBAL-
 * Main controller for tasks
 */
void po_ti_controller_swi(void)
;

/*-GLOBAL-
 * One time init of task version
 */
void po_ti_task_init(void)
;

/*-GLOBAL-INSERT-END-*/

/*-GLOBAL-INLINE-
 * Post controller SWI after HWI.
 */
static inline void po_function_context(void)
{
  po_ti_context();
}

#endif // po_ti_H
