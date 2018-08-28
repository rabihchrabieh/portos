/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Logging data module. Data on target is quickly logged into cicular buffer,
 * at preselected priority level. Formatting is performed later on host
 * machine. Data is overwritten if not read quickly enough.
 */

#ifndef po_log__H
#define po_log__H

#include <portos.h>

/* Circular buffer handle.
 */
typedef struct {
  int *buffer;
  volatile int wrptr;
  int mask;
  int priority;
} po_log_Handle;

/* Default handle.
 */
extern po_log_Handle po_log_HandleDefault;

/* Static initialization of circular buffer.
 * Initialize a logging area with user-supplied buffer.
 * buffer has to be int aligned (e.g. int buffer[128]).
 * nelements is number of int elements in buffer and must be a power of 2.
 * Logging for this handle occurs at specified priority level.
 * Example: po_log_Handle myhandle = po_log_HANDLEINIT(buffer, 512, 7);
 */
#define po_log_INIT(buffer, nelements, priority) \
  {(buffer), 0, (nelements) - 1, (priority)}

/* Priority function that performs logging into a circular buffer. It
 * prints 1 format string and 2 int values.
 * For a given buffer, the priority level should be fixed in order to
 * prevent data corruption.
 * The formatting is not performed on the target platform. The host machine
 * obtains the formatting string's address and performs the formatting on
 * host.
 */
void po_log_pf(
    #ifndef po_log_NO_PRIORITY
    po_priority(handle->priority),
    #endif
    po_log_Handle *handle, const char *format, int a0, int a1);

/* Shorthand version for po_log_pf and that hides the priority function
 * and handle details: it uses the default handle.
 */
void po_log(const char *format, int a0, int a1);

#endif // po_log__H
