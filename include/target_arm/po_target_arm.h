/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Interface to ARM platform.
 */

#ifndef po_target_arm__H
#define po_target_arm__H

#include <stdio.h>

#include <po_cfg_arm.h>

/* Empty */
#define po_NEARFAR

/* Tell Portos how to abort when an error occurs. In release
 * versions this may force the processor to reboot. In debug versions you
 * may want to catch the bug and analyze the last functions on the stack.
 */
static inline void po_abort(void)
{
  *(volatile int*)0 = 0;
}

/* Logging (cf. po_log)
 */
static inline void po_target_log(int *buffer)
{
  printf((char*)buffer[1], buffer[2], buffer[3]);
}

/* Error handling
 */
static inline void po_target_error(int error)
{
  //printf("Portos ERROR %d (%s:%d)\n", error, filename, line);
  //po_log("Portos ERROR %d\n", error, 0);
  po_abort();
}

/* Use software MSB detection
 */
#define po_lib_MSB_HW 0

#define po_arm_DISABLE_FIQ 0

#if po_arm_DISABLE_FIQ
#  define po_arm_DISABLE_IRUPT "#0xC0"
#else
#  define po_arm_DISABLE_IRUPT "#0x80"
#endif

#ifdef __thumb__

#else

/* Target specific
 */
static inline int po_interrupt_disable(void)
{
  int oldState;
  asm volatile(
	       "mrs    %0, cpsr"                      "\n"
	       "orr    r1, %0, " po_arm_DISABLE_IRUPT "\n"
	       "msr    cpsr_cxsf, r1"                 "\n"
	       : "=r" (oldState)
	       :
	       : "r1", "memory"
	       );
  return oldState;
}

/* Target specific
 */
static inline void po_interrupt_restore(int oldState)
{
  asm volatile(
	       "and    r1, %0, " po_arm_DISABLE_IRUPT "\n"
	       "mrs    r0, cpsr"                      "\n"
	       "bic    r0, r0, " po_arm_DISABLE_IRUPT "\n"
	       "orr    r0, r0, r1"                    "\n"
	       "msr    cpsr_cxsf, r0"                 "\n"
	       :
	       : "r" (oldState)
	       : "r0", "r1", "memory"
	       );
}
#endif

/* A replacement, on some targets, for __attribute__((alias("...")))
 */
#define po_target_ALIAS_SYMBOL(type, sym1, sym2)	\
  extern type sym1 __attribute__((alias(#sym2)))

//asm(".globl\t_" #sym1 "\n\t.set _" #sym1 ", _" #sym2)

/* Set context
 */
#define po_function_context po_function_resume

/* On some targets this function can be optimized.
 */
#define po_function_iscontext() (po_function_getpri() >= 0)

/* Target init
 */
static inline void po_target_init(void)
{
}

/*-GLOBAL-INSERT-*/

/*-GLOBAL-INSERT-END-*/


#endif // po_target_arm__H
