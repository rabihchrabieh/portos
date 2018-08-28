/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Includes the main Portos headers and initialization routines.
 */

#ifndef portos__H
#define portos__H

#include <po_sys.h>
#include <po_lib.h>
#include <po_list.h>
#include <po_memory.h>
#include <po_function.h>
#include <po_prep.h>
#include <po_signal.h>
#include <po_time.h>
#include <po_queue.h>
#include <po_log.h>

/* One time initialization of Portos.
 */
static inline void po_init_(void)
{
  po_function_init();
  po_target_init();
}

/*-GLOBAL-
 * One time initialization of Portos.
 */
void po_init(void);

#endif // portos__H
