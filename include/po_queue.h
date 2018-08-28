/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Queues of functions allow synchronizing priority functions.
 */

#ifndef po_queue__H
#define po_queue__H

#include <stddef.h>
#include <po_sys.h>
#include <po_list.h>
#include <po_function.h>
#include <po_prep.h>

/* Queue.
 */
typedef struct {
  /* Methods for this service */
  po_function_Service service;   /* MUST BE FIRST */

  /* Number of servers for this queue (e.g. 1 server) */
  int nServers;

  /* Token or semaphore count. If count > 0 then "count" servers are
     available */
  volatile int count;
  
  /* Queue's dynamic memory region */
  po_memory_Region *memRegion;

  /* Linked list of priority functions */
  po_list_List list;
} po_queue_Queue;

/* Internal queue node handle.
 */
typedef struct _po_queue_HandleInt {
  union {
    po_function_ServiceHandle service;  /* MUST BE FIRST */
    po_queue_Queue *queue;              /* Make sure it overlaps po_function_Service */
  } u;
  po_list_Node node;                    /* linked list */
} po_queue_HandleInt;

/*-GLOBAL-
 * Initialization macro for statically allocated queues.
 */
#define po_queue_INIT(queuePtr, nServers, memRegion)			\
  { {(po_function_ServiceFunc)po_queue_push},				\
      (nServers), (nServers), (memRegion), po_list_INIT(&(queuePtr)->list) }

/*-GLOBAL-INSERT-*/

/*-GLOBAL-
 * Insert in queue if no server is available.
 */
void po_queue_push(po_queue_HandleInt *quehandle)
;

/*-GLOBAL-
 * Release token. If queue is not empty, process next priority function in
 * queue.
 */
void po_queue_next(po_queue_Queue *queue)
;

/*-GLOBAL-
 * Sets the argument used to carry the queue info to a priority function.
 */
po_queue_HandleInt *po_queue_setsrv(po_queue_Queue *queue)
;

/*-GLOBAL-INSERT-END-*/

/*-GLOBAL-
 * Initialization routine for dynamically allocated queues.
 */
static inline void po_queue_init(po_queue_Queue *queue, int nServers,
				 po_memory_Region *memRegion)
{
  queue->service.func = (po_function_ServiceFunc)po_queue_push;
  queue->nServers = nServers;
  queue->count = nServers;
  queue->memRegion = memRegion;
  po_list_init(&queue->list);
}

/*-GLOBAL-
 * Queue directives
 */
static inline po_function_ServiceHandle* po_queue(po_queue_Queue *queue)
{
  return (po_function_ServiceHandle*)po_queue_setsrv(queue);
}

#endif // po_queue__H
