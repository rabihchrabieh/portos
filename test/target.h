/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Target specific functions.
 */

void hardware_interrupt(void);

#if _TI_  // For TI BIOS

#include <std.h>
#include <sys.h>
#include <log.h>
#include <hwi.h>

#ifdef far
extern far LOG_Obj trace;
#else
extern LOG_Obj trace;
#endif
#define Output &trace
#define myprintf LOG_printf

static inline void start_hwi(void) {
  #if _6x_
  HWI_dispatchPlug(14, (Fxn)hardware_interrupt, -1, NULL);
  #elif _55_
  HWI_dispatchPlug(4, (Fxn)hardware_interrupt, NULL);
  #else
  #error "Dispatcher missing"
  #endif
}

#define target_abort()  SYS_exit(0)

#else // gcc for example

//#include <stdio.h>
//#define Output stdout
//#define myprintf fprintf

static inline void start_hwi(void) { while ( 1 ) hardware_interrupt(); }

#define target_abort()  exit(0);

#endif
