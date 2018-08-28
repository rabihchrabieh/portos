/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * We define 2 types of signals. The simple signal is a structure pointing
 * to a linked list of pfuncs to be executed. The more complex signal is
 * defined as a group of signals and an integer value per signal. In this
 * case, we use the hash table library to implement efficient signal handling.
 * Each hash element corresponds to one signal value and points at the list
 * of pfuncs pending on this signal.
 * This module defines the complex signals.
 */

#ifndef po_signal_h
#define po_signal_h

#include <po_list.h>
#include <po_hash.h>
#include <po_function.h>

/* Forward reference */
struct _po_signal_Handle;
struct _po_signal_Group;

/* Internal signal handle */
typedef struct _po_signal_HandleInt {
  union {
    po_function_ServiceHandle service;  /* MUST BE FIRST */
    struct _po_signal_Group *group;    /* Make sure it overlaps po_function_Service */
  } u;
  int signal;                     /* signal value */
  struct _po_signal_Handle *cookie;  /* cookie for verifying the user handle */
  po_list_Node node;              /* linked list */
} po_signal_HandleInt;

/* User signal handle */
typedef struct _po_signal_Handle {
  po_signal_HandleInt *sighandle;
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

  // Group priority
  int groupPriority;

  /* Group's dynamic memory region */
  short memRegion;

  /* Type of database, 0 for list, 1 for hash table */
  short isHashTable;

  // Hash table or simple vector of lists
  union {
    po_hash_Table *hashTable;
    po_list_List list[1];   // SHOULD BE LAST, it will increase in length
  } u;
} po_signal_Group;

/*-GLOBAL-INSERT-*/

/*-GLOBAL-
 * Post signal. Any pfunc pending on this signal is executed now.
 */
void po_signal_post_(po_priority(group->groupPriority),
		  po_signal_Group *group, int signal)
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
po_signal_HandleInt *po_signal_setsrv_h(int signal, po_signal_Group *group, po_signal_Handle *usighandle)
;

/*-GLOBAL-
 * Create signal group. The groupPriority is inherited only by the functions
 * that handle the signals for this group. It does not affect the priority
 * of the pfuncs scheduled in this group. It is recommended that all pfuncs
 * that can be scheduled in this group have a priority level below the
 * group level.
 * Either numSignals or hashSize is used. The other one should be set to
 * -1. When numSignals is used, a vector of lists is created and the code
 * is more efficient. But the signal must be limited to the interval
 * [0, numSignals-1].
 * When hashSize is used, a hash table is created and signal can take any
 * integer value.
 * hashSize is the size of the hash table. The larger the size the
 * more efficient the processing of the signals. The hashSize must be a
 * power of 2.
 * memRegion specifies in which memory region the group will be allocated,
 * as well as all internal dynamic handles used by this group.
 */
po_signal_Group *po_signal_create(
    int groupPriority,      // group priority
    int numSignals,         // signal varies in [0, numSignals-1]
    int hashSize,           // hash table size
    int memRegion           // memory region in which the group is allocated
    )
;

/*-GLOBAL-
 * Destroy a signal group.
 */
void po_signal_destroy_(po_priority(group->groupPriority),
		     po_signal_Group *group)
;

/*-GLOBAL-
 * Display the content of the signal group. It returns the total number of
 * attached pfuncs.
 */
int po_signal_display(po_signal_Group *group)
;

/*-GLOBAL-INSERT-END-*/

/*-GLOBAL-
 * Post signal. Any pfunc pending on this signal is executed now.
 */
static inline void po_signal_post(po_signal_Group *group, int signal)
{
  po_signal_post_(po_priority, group, signal);
}

/*-GLOBAL-
 * Destroy signal group.
 */
static inline void po_signal_delete(po_signal_Group *group)
{
  po_signal_delete_(po_priority, group);
}

/*-GLOBAL-
 * Signal directives
 */
extern po_signal_Group *po_signal_DefaultGroup;
static inline po_function_ServiceHandle *po_signal(int signal)
{
  return (po_function_ServiceHandle*)po_signal_setsrv(signal, po_signal_DefaultGroup);
}
static inline po_function_ServiceHandle *po_signal_g(int signal, po_signal_Group *group)
{
  return (po_function_ServiceHandle*)po_signal_setsrv(signal, group);
}
static inline po_function_ServiceHandle *po_signal_h(int signal, po_signal_Handle *handle)
{
  return (po_function_ServiceHandle*)po_signal_setsrv_h(signal, po_signal_DefaultGroup, handle);
}
static inline po_function_ServiceHandle *po_signal_gh(int signal, po_signal_Group *group, po_signal_Handle *handle)
{
  return (po_function_ServiceHandle*)po_signal_setsrv_h(signal, group, handle);
}
static inline po_function_ServiceHandle *po_signal_hg(int signal, po_signal_Handle *handle, po_signal_Group *group) 
{
  return (po_function_ServiceHandle*)po_signal_setsrv_h(signal, group, handle);
}

#endif // po_signal_h
