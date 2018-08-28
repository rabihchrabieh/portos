/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * YOU CANNOT MODIFY THIS FILE UNLESS YOU HAVE THE FULL PORTOS SOURCE
 * CODE IN ORDER TO RECOMPILE IT. OTHERWISE, MODIFYING THIS FILE WILL
 * RESULT IN SYSTEM CRASH.
 *
 * Configuration file containing default values that can be modified by
 * the programmer.
 */

#ifndef po_cfg__H
#define po_cfg__H


/* DEBUGGING */

/* Enable debugging phase */
#if defined(DEBUG)
#  define po_DEBUG                 DEBUG
#elif defined(_DEBUG)
#  define po_DEBUG                 _DEBUG
#else
#  define po_DEBUG                 0
#endif

/* Optimize for size or speed
 */
#define po_OPTIMIZE_SIZE           0  // 0: speed, 1: size

/* For debugging on hosts such as TI CCS using GEL commands, we currently
 * need to keep track of priority functions' names.
 */
#define po_function_TRACK_NAME     po_DEBUG

/* Track memory allocations for debugging: file name and line number.
 * The level of this value denotes more or less tracking:
 * 0 - no tracking
 * 1 - medium tracking
 * 2 - full tracking
 */
#define po_memory_TRACK_ALLOC      (po_DEBUG ? 2 : 0)

/* For faster allocation and freeing, you can disable extra sanity
 * checking. But this is only recommended for the final product after
 * it has been thoroughly debugged. You can also leave it enabled in the
 * final product.
 */
#define po_memory_SANITY_CHECK     po_DEBUG

/* Memory alignment: 1, 2, 4 or 8 byte boundary. It must be able to
 * accomodate any type of variables. A default of 8 is fine
 * in most cases. It puts a limit on the minimum block size.
 */
#define po_memory_ALIGN            (sizeof(double))


/* DYNAMIC MEMORY MANAGEMENT (Heap) */

/* Default memory region for Portos library */
#define po_memory_RegionSystem     po_memory_RegionDefault

/* Use malloc with variable size blocks (small memory improvement if
 * disabled) */
#define po_memory_VARIABLE_SIZE    1

/* Abort program if heap is full (otherwise, return NULL pointer) */
#define po_memory_HEAP_FULL_ABORT  1


/* MISCELLANEOUS */

/* Look up table width for MSB search. Irrelevant when the DSP has one fast
 * instruction to find the MSB. In the negative, the look up table can
 * be a 4 bit (16 bytes) or 8 bit (256 bytes) table. The larger the more
 * efficient and uses less code memory, but uses a lot more data memory.
 * The 4 bit version is nearly as efficient as the 8 bit version.
 */
#define po_lib_MSB_TABLE_WIDTH     4


/* Include target specific headers (only one file) */
//#include <po_target_sim.h>
#include <po_target_dspbios.h>

#endif // po_cfg__H
