/*
 * Portos v1.2.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Implementation of signals.
 */

#ifndef po_signal__H
#define po_signal__H

#include <stddef.h>
#include <po_sys.h>
#include <po_list.h>
#include <po_function.h>
#include <po_prep.h>

/* Forward reference */
struct _po_signal_Handle;
struct _po_signal_Group;

/* Internal signal handle */
typedef struct _po_signal_HandleInt {
  union {
    po_function_ServiceHandle service;  /* MUST BE FIRST */
    struct _po_signal_Group *group;     /* Make sure it overlaps po_function_Service */
  } u;
  volatile struct _po_signal_Handle *usighandle; /* user handle if available */
  po_list_Node node;                    /* linked list */
  int signal;                           /* signal value */
} po_signal_HandleInt;

/* User signal handle */
typedef struct _po_signal_Handle {
  volatile po_signal_HandleInt *sighandle;
} po_signal_Handle;

/* Signal group. For additional flexibility, the user can define
 * different groups of signals. For instance, a timer module can define
 * its own group of signals, each signal being the time (eg, frame number
 * or clock in milliseconds).
 * Each group of signals has its own priority level. This is the priority
 * level at which the signal handling functions will execute. This is
 * not the priority level at which the pfuncs will execute (pfuncs will
 * execute at their own priority level).
 * It is highly recommended that all pfuncs that can be scheduled in some
 * group have a priority level less than or equal to the group priority
 * level. If some pfuncs have a priority level above the priority of the
 * group, the behavior is not well defined. The priority levels can be
 * mixed up for those pfuncs above the group level.
 */
typedef struct _po_signal_Group {
  /* Methods for this service */
  po_function_Service service;   /* MUST BE FIRST */

  // Group priority used by internal signal routines
  int groupPriority;

  // Group's dynamic memory region for allocating signal handles
  po_memory_Region *memRegion;

  // Hash table consisting of a vector of lists.
  // Hash key is signal modulo (2^n - 1) if hashSize is a power of 2
  int hashSize;
  int modulo;
  po_list_List list[1];   // SHOULD BE LAST, it will increase in length
} po_signal_Group;

/* Macro for group init. po_signal_groupinit is also needed for complete
 * initialization.
 */
#define po_signal_GROUPINIT(groupPriority, hashSize, memRegion) \
  {{								\
    {(po_function_ServiceFunc)po_signal_attach},		\
    (groupPriority),						\
    (po_memory_Region*)(memRegion),				\
    (hashSize), 						\
    ((hashSize) & ((hashSize) - 1)) ? -1 : (hashSize)-1 	\
  }}

/* Macro to define the signal group structure with variable number of
 * lists. A compounded structure is created.
 */
#define po_signal_GroupVar(group_, hashSize)				\
  struct {								\
    po_signal_Group group;						\
    po_list_List storage[(hashSize)-1];					\
  } group_

#define po_signal_Group(group, groupPriority, hashSize, memRegion)	\
  po_signal_GroupVar(group, (hashSize)) =				\
    po_signal_GROUPINIT((groupPriority), (hashSize), (memRegion))

#if 0
#define po_signal_Group(group, groupPriority, hashSize, memRegion)	\
  static po_signal_GroupVar(group ## _po_storage, (hashSize)) =	\
    po_signal_GROUPINIT((groupPriority), (hashSize), (memRegion));	\
  po_target_ALIAS_SYMBOL(po_signal_Group, group, group ## _po_storage)
#endif

/* Default signal group when it's not specified
 */
#ifndef po_INIT_FILE
extern po_signal_Group po_signal_GroupDefault;
#endif

/* Signal state
 */
typedef struct {
  // Some unused pointers needed to ensure dummy_usighandle points inside
  // structure. Not all architectures need this so we could #if it.
  // Is there a mechanism to say "at least N words from start of segment?"
  // Actually, it might work even though TI compiler returns a warning, as
  // pointer arithmetic could properly wrap around.
  void *unused[2];
  // Emulate 
  po_signal_HandleInt *dummy_sighandle;
  po_signal_Handle *dummy_usighandle;
} po_signal_DataType;

extern po_signal_DataType po_signal_Data;

/* Dummy handles
 */
#define po_signal_DUMMY_USIGHANDLE \
  ((po_signal_Handle*)((char*)&po_signal_Data.dummy_sighandle - (int)offsetof(po_signal_Handle, sighandle)))

#define po_signal_DUMMY_SIGHANDLE \
  ((po_signal_HandleInt*)((char*)&po_signal_Data.dummy_usighandle - (int)offsetof(po_signal_HandleInt, usighandle)))


/*-GLOBAL-INSERT-*/

/*-GLOBAL-
 * Wrapper function.
 */
void po_signal_attach(po_signal_HandleInt *sighandle)
;

/*-GLOBAL-
 * Post signal. Any pfunc pending on this signal is executed now.
 */
void po_signal_post_(po_priority(group->groupPriority),
		     int signal, po_signal_Group *group)
;

/*-GLOBAL-
 * Try to detach (cancel) pfunc call specified by the signal handle.
 * This function calls a pfunc which does the real job.
 */
void po_signal_detach(po_signal_Handle *usighandle)
;

/*-GLOBAL-
 * Sets the argument used to carry the signal info to a priority function.
 */
po_signal_HandleInt *po_signal_setsrv(int signal, po_signal_Group *group)
;

/*-GLOBAL-
 * Idem but with a user handle in addition.
 */
po_signal_HandleInt *po_signal_setsrv_h(int signal, po_signal_Group *group,
					po_signal_Handle *usighandle)
;

/*-GLOBAL-
 * Create signal group. The groupPriority is inherited only by the functions
 * that handle the signals for this group. It does not affect the priority
 * of the pfuncs scheduled in this group. It is recommended that all pfuncs
 * that can be scheduled in this group have a priority level below or equal
 * to the group level.
 * hashSize is the size of the hash table. The larger the size the
 * more efficient the processing of the signals. The hashSize must be a
 * power of 2 in order to allow for any signal value. If hashSize is not
 * a power of 2 then signals can only vary between 0 and hashSize-1.
 * memRegion specifies in which memory region the group will be allocated,
 * as well as all internal dynamic handles used by this group.
 */
void po_signal_groupinit(po_signal_Group *group)
;

/*-GLOBAL-INSERT-END-*/

/*-GLOBAL-
 * Post signal. Any pfunc pending on this signal is executed now.
 */
static inline void po_signal_post(int signal, po_signal_Group *group)
{
  po_signal_post_(po_priority, signal, group);
}

/*-GLOBAL-
 * Signal directives
 */
#define po_signal(signal)						\
  ((po_function_ServiceHandle*)po_signal_setsrv((signal), &po_signal_GroupDefault))

/*-GLOBAL-
 * Signal directives
 */
#define po_signal_p(signal, group) \
  ((po_function_ServiceHandle*)po_signal_setsrv((signal), (group)))

/*-GLOBAL-
 * Signal directives
 */
static inline po_function_ServiceHandle* po_signal_ph(
    int signal, po_signal_Group *group, po_signal_Handle *handle
)
{
  return (po_function_ServiceHandle*)
    po_signal_setsrv_h(signal, group, handle);
}

/*-GLOBAL-
 * Return non-zero if the handle is in-use.
 */
static inline int po_signal_isactive(po_signal_Handle *handle)
{
  return handle->sighandle != po_signal_DUMMY_SIGHANDLE;
}

/*-GLOBAL-
 * Init a user handle (sometimes useful at start up if the handle
 * can be tested before it has been attached).
 */
static inline void po_signal_init(po_signal_Handle *handle)
{
  handle->sighandle = po_signal_DUMMY_SIGHANDLE;
}

/*-GLOBAL-
 * Static initialization for user handles
 */
#define po_signal_INIT  {po_signal_DUMMY_SIGHANDLE}

#endif // po_signal__H
