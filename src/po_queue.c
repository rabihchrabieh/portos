/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Cf. po_queue.h for a module description
 */

#include <portos.h>

/*-GLOBAL-
 * Insert in queue if no server is available.
 */
void po_queue_push(po_queue_HandleInt *quehandle)
{
  po_queue_Queue *queue = quehandle->u.queue;
  int protectState = po_interrupt_disable();

  if ( queue->count > 0 ) {
    po_function_Handle *pfhandle;
    int priority;
    queue->count--;
    po_interrupt_restore(protectState);

    pfhandle = quehandle->u.service.pfhandle;
    priority = quehandle->u.service.priority;
    po_free(quehandle);

    /* Call priority function */
    po_function_call(pfhandle, priority);

  } else {
    // Insert in linked list
    po_list_pushtail(&queue->list, &quehandle->node);
    po_interrupt_restore(protectState);
  }
}

/*-GLOBAL-
 * Release token. If queue is not empty, process next priority function in
 * queue.
 */
void po_queue_next(po_queue_Queue *queue)
{
  int protectState = po_interrupt_disable();

  if ( !po_list_isempty(&queue->list) ) {
    po_function_Handle *pfhandle;
    int priority;
    po_queue_HandleInt *quehandle;
    po_list_Node *node = po_list_pophead(&queue->list);
    po_interrupt_restore(protectState);

    quehandle = po_list_getobj(node, po_queue_HandleInt, node);
    pfhandle = quehandle->u.service.pfhandle;
    priority = quehandle->u.service.priority;
    po_free(quehandle);

    /* Call priority function */
    po_function_call(pfhandle, priority);

  } else {
    queue->count++;
    po_interrupt_restore(protectState);
  }
}

/*-GLOBAL-
 * Sets the argument used to carry the queue info to a priority function.
 */
po_queue_HandleInt *po_queue_setsrv(po_queue_Queue *queue)
{
  /* Create internal queue handle */
  po_queue_HandleInt *quehandle =
    po_rmalloc_const(sizeof(*quehandle), queue->memRegion);
  if ( !quehandle ) po_memory_error();

  po_function_setsrv(&quehandle->u.service, (po_function_Service*)queue);
  return quehandle;
}
