/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * System functions.
 */

#ifndef po_sys__H
#define po_sys__H

#include <stddef.h>
#include <limits.h>
#include <po_cfg.h>
#include <po_error.h>

#if INT_MAX == (1<<15)-1
  #define po_INT_SIZE        16
#elif INT_MAX == (1<<23)-1
  #define po_INT_SIZE        24
#elif INT_MAX == (1<<31)-1
  #define po_INT_SIZE        32
//#elif INT_MAX == (1LL<<63)-1     TODO: DOES NOT COMPILE on some machines
//  #define po_INT_SIZE        64
#else
  #error("Unknown integer size");
#endif

/* Emulate interrupts
 */
void po_emulateirupt_(void);
static inline void po_emulateirupt(void)
{
  #if po_TEST
  po_emulateirupt_();
  #endif
}

/*-GLOBAL-INSERT-*/
/*-GLOBAL-INSERT-END-*/

#endif // po_sys__H
