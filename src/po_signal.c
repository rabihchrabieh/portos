/*
 * Portos v1.2.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Cf. po_signal.h for a module description
 * A table of linked lists, each linked list contains all functions
 * that are attached to the signal modulo the table size.
 */

#include <portos.h>

po_NEARFAR po_signal_DataType po_signal_Data = {
  {NULL, NULL},

  // Dummy pointer to dummy sighandle
  po_signal_DUMMY_SIGHANDLE,

  // Dummy pointer to dummy usighandle
  po_signal_DUMMY_USIGHANDLE
};

/* Schedule a pfunc to be called when some signal is received. The pfunc waits
 * until the signal is posted. If a user handle is supplied by the caller, the
 * handle can be used later to detach the pfunc (cancel), or to check the
 * status of the pfunc (waiting, etc). If a handle is not supplied, the
 * pfunc cannot be detached.
 */
static void po_signal_attach_(po_priority(sighandle->u.group->groupPriority),
			      po_signal_HandleInt *sighandle)
{
  po_signal_Group *group = sighandle->u.group;

  // Insert in signal vector list
  po_list_pushtail(&group->list[sighandle->signal & group->modulo],
		   &sighandle->node);
}

/*-GLOBAL-
 * Wrapper function.
 */
void po_signal_attach(po_signal_HandleInt *sighandle)
{
  po_signal_attach_(po_priority, sighandle);
}

/* The signal module calls this function as a pfunc which redirects the
 * call to the real priority function. This indirect calling method is
 * used to minimize the effect of the latency between the signal post and
 * actual function call. If in the meantime the function is detached, we
 * won't call it.
 */
static void po_signal_func(po_priority(sighandle->u.service.priority),
			   po_signal_HandleInt *sighandle)
{
  po_function_Handle *pfhandle;

  /* Disconnect from user handle if any. We use a trick to avoid locking
   * interrupts: point to dummy handles when there are none.
   */
  sighandle->usighandle->sighandle = po_signal_DUMMY_SIGHANDLE;

  /* At this point, the handle can no longer be detached.
   * Moreover, if detach is called at higher or equal priority than this
   * function (po_signal_func), and if a test isactive(handle) is performed,
   * there should be no race conditions.
   * E.g. state machine runs at unique priority level + ISR or SWI.
   * Note that post() won't be an issue if detach() runs at higher or equal
   * priority than this function, po_signal_func. Without this function,
   * there can be race condition issues when isactive() and detach() are
   * called below post() level.
   * A similar solution can be applied to task model via mailbox. */

  /* Get priority function handle: it may have been changed in a volatile
   * way */
  pfhandle = sighandle->u.service.pfhandle;

  /* Free handle */
  po_free(sighandle);

  /* If the function has not been detached so far, call it (too late to
   * detach; a detach will be ignored) */
  if ( pfhandle ) po_function_callschedulerentry(pfhandle);
}

/*-GLOBAL-
 * Post signal. Any pfunc pending on this signal is executed now.
 */
void po_signal_post_(po_priority(group->groupPriority),
		     int signal, po_signal_Group *group)
{
  // Remove this signal from data base and obtain list of attached handles
  po_list_List *list;
  po_list_Node *node;

  #if po_DEBUG > 1 // DEBUG_MODE
  if ( group->modulo == -1 && (signal < 0 || signal >= group->hashSize) )
    po_error(po_error_SIG_POST_OUT_OF_RANGE);
  #endif // DEBUG_MODE

  list = &group->list[signal & group->modulo];

  // Call the pfuncs in the list. Since the nodes won't be freed at this
  // point, we could have slightly faster version that does not need to remove
  // the nodes from the list.
  node = po_list_head(list);
  while ( node != list ) {
    po_list_Node *nextnode = po_list_next(node);
    po_signal_HandleInt *sighandle =
      po_list_getobj(node, po_signal_HandleInt, node);
    if ( sighandle->signal == signal ) {
      po_list_pop(node);
      node->prev = NULL;
      po_signal_func(po_priority, sighandle);
    }
    node = nextnode;
  }
}

/* Detach (cancel) pfunc call specified by the signal handle.
 */
static void po_signal_detachInt(po_priority(sighandle->u.group->groupPriority),
				po_signal_HandleInt *sighandle)
{
  /* If function nearly called (null pfhandle), don't do anything */
  if ( sighandle->u.service.pfhandle ) {
    /* We got here before po_signal_func, so we succeeded in detaching */
    po_free(sighandle->u.service.pfhandle);
    if ( sighandle->node.prev ) {
      /* The handle was still in database, po_signal_post not yet called */
      po_list_pop(&sighandle->node);
      po_free(sighandle);
    } else {
      /* The handle was no longer in the database, po_signal_post already
       * called. Simply inform po_signal_func not to process this call */
      sighandle->u.service.pfhandle = NULL;
    }
  }
}

/*-GLOBAL-
 * Try to detach (cancel) pfunc call specified by the signal handle.
 * This function calls a pfunc which does the real job.
 */
void po_signal_detach(po_signal_Handle *usighandle)
{
  po_signal_HandleInt *sighandle;

  /* Disconnect from signal handle (after this point no one can modify it
   * in a volatile way) */
  usighandle->sighandle->usighandle = po_signal_DUMMY_USIGHANDLE;
  po_emulateirupt();

  /* Signal handle could have been changed in a volatile way before or during
   * the previous statement, so read it again */
  sighandle = (po_signal_HandleInt*)usighandle->sighandle;
  po_emulateirupt();

  /* Declare the user handle as detached so it can be reused */
  usighandle->sighandle = po_signal_DUMMY_SIGHANDLE;
  po_emulateirupt();

  /* Detach from database */
  if ( sighandle != po_signal_DUMMY_SIGHANDLE ) {
    po_emulateirupt();
    po_signal_detachInt(po_priority, sighandle);
  }
}

/*-GLOBAL-
 * Sets the argument used to carry the signal info to a priority function.
 */
po_signal_HandleInt *po_signal_setsrv(int signal, po_signal_Group *group)
{
  po_signal_HandleInt *sighandle;

  #if po_DEBUG > 1 // DEBUG_MODE
  if ( group->modulo == -1 && (signal < 0 || signal >= group->hashSize) )
    po_error(po_error_SIG_POST_OUT_OF_RANGE);
  #endif // DEBUG_MODE

  /* Create internal signal handle */
  sighandle = po_rmalloc_const(sizeof(*sighandle), group->memRegion);
  if ( !sighandle ) po_memory_error();

  sighandle->signal = signal;
  sighandle->usighandle = po_signal_DUMMY_USIGHANDLE;
  po_function_setsrv(&sighandle->u.service, (po_function_Service*)group);
  return sighandle;
}

/*-GLOBAL-
 * Idem but with a user handle in addition.
 */
po_signal_HandleInt *po_signal_setsrv_h(int signal, po_signal_Group *group,
					po_signal_Handle *usighandle)
{
  po_signal_HandleInt *sighandle;
  #if po_DEBUG
  if ( usighandle->sighandle != po_signal_DUMMY_SIGHANDLE ) {
    if ( usighandle->sighandle->usighandle != usighandle )
      po_error(po_error_SIG_CORRUPT_HANDLE);
    else
      po_error(po_error_SIG_MULTIPLE_ATTACH);
  }
  #endif
  sighandle = po_signal_setsrv(signal, group);
  sighandle->usighandle = usighandle;
  usighandle->sighandle = sighandle;
  return sighandle;
}

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
{
  int hashSize = group->hashSize;
  int i;

  /* Init vector list */
  #if _TI_
  #pragma MUST_ITERATE(1);
  #endif
  for ( i = 0 ; i < hashSize; i++ ) po_list_init(&group->list[i]);
}
