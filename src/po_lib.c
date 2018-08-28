/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Cf. po_lib.h for a module description.
 */

#include <po_sys.h>
#include <po_lib.h>

#if 0

/* LSB and MSB positions from http://aggregate.org/MAGIC/
 * We don't use these versions but define more efficient ones.
 */

/* Returns number of ones in a 32 bit integer
 */
static inline int po_lib_onesCount(unsigned x)
{
  /* 32-bit recursive reduction using SWAR...
   * but first step is mapping 2-bit values
   * into sum of 2 1-bit values in sneaky way
   */
  x -= ((x >> 1) & 0x55555555);
  x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
  x = (((x >> 4) + x) & 0x0f0f0f0f);
  x += (x >> 8);
  x += (x >> 16);
  return x & 0x0000003f;
}

/* Returns LSB bit position
 */
int po_lib_lsb_(unsigned x)
{
  return po_lib_onesCount((x & -x) - 1);
}

/* Returns MSB bit position. This is one instruction on certain machines
 */
int po_lib_msb_(unsigned x)
{
  x |= (x >> 1);
  x |= (x >> 2);
  x |= (x >> 4);
  x |= (x >> 8);
  x |= (x >> 16);
  return po_lib_onesCount(x) - 1;
}

#endif

/* Versions defined and used by Portos.
 */

#if !po_lib_MSB_HW    /* If not hardware MSB detection code available */

/* Lookup table of width 4 or 8 bits. 8 bits is more efficient but uses
 * 256 chars to store the table. 4 bits is nearly as efficient while only
 * using 16 chars to store the table. But the 4 bits version uses more
 * code memory.
 */
static const signed char table_msb[1 << po_lib_MSB_TABLE_WIDTH] = {
  -1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3
  #if po_lib_MSB_TABLE_WIDTH == 8
  ,
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
  #endif
};

/*-GLOBAL-
 * Returns MSB position, -1 if x is 0.
 * The function handles 16 and 32 bit integers.
 */
int po_lib_msb_sw(unsigned x)
{
  #if po_lib_MSB_TABLE_WIDTH == 8

  /* Look up table of 256 chars */
  #if po_INT_SIZE == 32
  if ( x < (1<<16) ) {
  #endif
    if ( x < (1<<8) ) {
      return table_msb32_8[x];
    } else {
      return 8 + table_msb32_8[x >> 8];
    }
  #if po_INT_SIZE == 32
  } else {
    if ( x < (1<<24) ) {
      return 16 + table_msb32_8[x >> 16];
    } else {
      return 24 + table_msb32_8[x >> 24];
    }
  }
  #endif

  #else

  /* Look up table of 16 chars */
  #if po_INT_SIZE == 32
  if ( x < (1<<16) ) {
  #endif
    if ( x < (1<<8) ) {
      if ( x < (1<<4) ) return table_msb[x];
      else return 4 + table_msb[x >> 4];
    } else {
      if ( x < (1<<12) ) return 8 + table_msb[x >> 8];
      else return 12 + table_msb[x >> 12];
    }
  #if po_INT_SIZE == 32
  } else {
    if ( x < (1<<24) ) {
      if ( x < (1<<20) ) return 16 + table_msb[x >> 16];
      else return 20 + table_msb[x >> 20];
    } else {
      if ( x < (1<<28) ) return 24 + table_msb[x >> 24];
      else return 28 + table_msb[x >> 28];
    }
  }
  #endif

  #endif
}

#endif
