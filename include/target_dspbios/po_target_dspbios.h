/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * This module is the interface to TI's BIOS.
 *
 * NOTES:
 * TI compiler can be auto-identified via __TI_COMPILER_VERSION__
 * TI BIOS can be auto-identified via _TI_, after std.h is included
 */

#ifndef po_target_dspbios__H
#define po_target_dspbios__H

#include <std.h>
#include <sys.h>
#include <hwi.h>
#include <log.h>
#include <sem.h>
#include <mem.h>
#include <po_cfg_dspbios.h>

/* Preprocessor definition
 */
#define po_prep_template_start po_prep_template_start_TI

/* Near or far data, used by certain Portos structures so they can be
 * located in the BSS section for faster access */
#ifdef near
#  define po_NEARFAR near
#else
#  define po_NEARFAR
#endif

/* Tell Portos how to abort when an error occurs. In release
 * versions this may force the processor to reboot. In debug versions you
 * may want to catch the bug and analyze the last functions on the stack.
 */
static inline void po_abort(void)
{
  SYS_abort("");
}

typedef struct {
  SWI_Handle swi_handle;
} po_target_DataType;

extern po_target_DataType po_target_Data;

/* If LMBD is defined use hardware MSB detection.
 * NORM instruction can also be used on C5x.
 */
#if _6x_
#  define po_lib_MSB_HW 1
static inline int po_lib_msb_hw(unsigned x)
{
  return 31 - _lmbd(1, x);
}
#elif _54_ || _55_
#  define po_lib_MSB_HW 1
static inline int po_lib_msb_hw(unsigned x)
{
  if ( x != 0 ) {
    if ( x < 0x8000 ) {
      return 14 - _norm(x);
    } else return 15;
  } else return -1;
}
#else
#  define po_lib_MSB_HW 0
#endif

/* Disable HWI
 */
#define po_interrupt_disable   HWI_disableI

/* Restore HWI
 */
#define po_interrupt_restore   HWI_restoreI

#if 0
/* A replacement, on some targets, for __attribute__((alias("...")))
 */
#define po_target_ALIAS_SYMBOL(type, sym1, sym2)		\
  extern type sym1 __attribute__((alias(#sym2)));		\
  __asm("\t.global\t_" #sym1 "\n_" #sym1 "\t.set\t_" #sym2)
#endif

/* Error handling
 */
void po_target_error(int error)
;

/* Target init
 */
void po_target_init(void)
;

/* Set context
 */
static inline void po_function_context(void)
{
  SWI_post(po_target_Data.swi_handle);
}

/* On some targets this function can be optimized.
 */
#define po_function_iscontext() (po_function_getpri() >= 0)

#endif // po_target_dspbios__H
