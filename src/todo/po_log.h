/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * This is a temporary implementation of the message logging.
 * This implementation is useful when we run simulations in a non
 * real time environment. We get the message as soon as it is posted.
 * A more correct implementation for real time would define a priority
 * function po_log_print() for example with a pretty low priority level.
 * This priority function sends slowly its messages to the output
 * device. In order to keep track of all the messages that were
 * posted (in case of system crash, it would be nice to recover the
 * messages), this function can also maintain a linked list of all
 * the pending messages. Upon system crash, the linked list can
 * be recovered and printed to the output. The function po_log_printf
 * can only take a constant number and well defined parameters (integers
 * and pointers for example).
 * With such a scheme, multiple output buffers can be defined with different
 * priority levels.
 */

#ifndef po_log_H
#define po_log_H

/*-GLOBAL-INSERT-*/

/*-GLOBAL-
 * Error code reporting
 */
void po_log_error(int error, char *file, int line)
;

/*-GLOBAL-INSERT-END-*/


#endif // po_log_H
