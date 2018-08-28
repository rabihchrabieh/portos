/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Cf. po_log.h for a module description.
 */

#include <po_log.h>

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
    po_log_Handle *handle, const char *format, int a0, int a1)
{
  int *buffer = handle->buffer;
  int mask = handle->mask;
  int wr = handle->wrptr;
  buffer[wr & mask] = wr; // Message counter
  wr++;
  buffer[(wr++) & mask] = (int)format; // What if char* > int ? (const section)
  buffer[(wr++) & mask] = a0;
  buffer[(wr++) & mask] = a1;
  handle->wrptr = wr;

  po_target_log(&buffer[wr-4]);
}

/* Shorthand version for po_log_pf and that hides the priority function
 * and handle details: it uses the default handle.
 */
void po_log(const char *format, int a0, int a1)
{
  po_log_pf(
    #ifndef po_log_NO_PRIORITY
    po_priority,
    #endif
    &po_log_HandleDefault, format, a0, a1);
}
