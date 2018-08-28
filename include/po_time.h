/*
 * Portos v1.2.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * This module is a layer on top of the signal module.
 * Several clocks can be defined. One clock can handle for example the
 * frame interrupts while another can handle a more fine time such as a
 * slot interrupt. pfuncs can be executed at the specified time of some
 * clock.
 */

#ifndef po_time__H
#define po_time__H

#include <po_sys.h>
#include <po_lib.h>
#include <po_memory.h>
#include <po_function.h>
#include <po_signal.h>

/* Clock structure.
 * If modified, update po_time_ClockVar.
 */
typedef struct {
  int time;
  po_signal_Group signalGroup;
} po_time_Clock;

/* Macro for clock init. po_time_clockinit is also needed for complete
 * initialization.
 */
#define po_time_CLOCKINIT(clockPriority, hashSize, memRegion)		\
  {{									\
    0,  								\
    po_signal_GROUPINIT((clockPriority), (hashSize), (memRegion))	\
  }}

/* Macro to define the clock structure with variable number of
 * lists. A compounded structure is created.
 * Unfortunately, we have variable of variable size (clock and group),
 * which sort of requires duplicating the content of po_time_Clock.
 */
#define po_time_ClockVar(clock_, hashSize)				\
  struct {								\
    struct {								\
      int time;								\
      po_signal_GroupVar(signalGroup, (hashSize));			\
    } storage;								\
  } clock_

#define po_time_Clock(clock, clockPriority, hashSize, memRegion)	\
  po_time_ClockVar(clock, (hashSize)) =				\
    po_time_CLOCKINIT((clockPriority), (hashSize), (memRegion))

#if 0
#define po_time_Clock(clock, clockPriority, hashSize, memRegion)	\
  static po_time_ClockVar(clock ## _po_storage, (hashSize)) =		\
    po_time_CLOCKINIT((clockPriority), (hashSize), (memRegion));	\
  po_target_ALIAS_SYMBOL(po_time_Clock, clock, clock ## _po_storage)
#endif

/* Default clock when it's not specified
 */
#ifndef po_INIT_FILE
extern po_time_Clock po_time_ClockDefault;
#endif

/* Timer handle. It can be user supplied or automatically created by
 * the timer routines. If it is user supplied, it can be used to check
 * on the status of the timer and to cancel the timer.
 */
typedef po_signal_Handle po_time_Handle;

/* This is to complete clock init.
 */
static inline void po_time_clockinit(po_time_Clock *clock)
{
  po_signal_groupinit(&clock->signalGroup);
}

/*-GLOBAL-
 * Cancel a timer, ie, a pfunc scheduled at some time. The handle must
 * be known (ie, it must have been supplied to po_time_start).
 */
static inline void po_time_cancel(po_time_Handle *handle)
{
  po_signal_detach(handle);
}

/*-GLOBAL-
 * Increment clock time and execute all timers that have just expired.
 */
static inline void po_time_tick(po_time_Clock *clock)
{
  clock->time++;
  po_signal_post(clock->time, &clock->signalGroup);
}

/*-GLOBAL-
 * Return clock time
 */
static inline int po_time_get(po_time_Clock *clock)
{
  return clock->time;
}

/*-GLOBAL-
 * Set clock time. This is usually used to wrap the clock time back to
 * zero.
 * Note: you should not use this function to increment by 2 the clock
 * value. One clock tick will be missed in this case. Rather call
 * po_time_tick twice.
 */
static inline void po_time_set(int time, po_time_Clock *clock)
{
  clock->time = time;
  po_signal_post(clock->time, &clock->signalGroup);
}

/*-GLOBAL-
 * Time directives
 */
static inline po_function_ServiceHandle* po_time_p(
    int time, po_time_Clock *clock
)
{
  return po_signal_p(time, &clock->signalGroup);
}

/*-GLOBAL-
 * Time directives
 */
static inline po_function_ServiceHandle* po_time_ph(
    int time, po_time_Clock *clock, po_time_Handle *handle
)
{
  return po_signal_ph(time, &clock->signalGroup, handle);
}

/*-GLOBAL-
 * Time directives
 */
#define po_time(time)				\
  po_time_p((time), &(po_time_ClockDefault))

/*-GLOBAL-INSERT-*/

/*-GLOBAL-INSERT-END-*/

/*-GLOBAL-
 * Return non-zero if the timer is running, i.e., not yet done.
 */
static inline int po_time_isactive(po_time_Handle *handle)
{
  return po_signal_isactive(handle);
}

/*-GLOBAL-
 * Init a user handle (sometimes useful at start up if the handle
 * can be tested before it has been attached).
 */
static inline void po_time_init(po_time_Handle *handle)
{
  po_signal_init(handle);
}

/*-GLOBAL-
 * Static initialization for user handles
 */
#define po_time_INIT  po_signal_INIT

#endif // po_time__H
