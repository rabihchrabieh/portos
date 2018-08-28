/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Some general purpose functions
 */

#ifndef po_lib__H
#define po_lib__H

#include <po_sys.h>

/* Compare 2 values. The macro compares the 2 integers that can wrap around.
 * Eg, for a 16 bit integer, this means
 *   -1 < 0 but also (int)0x7FFF < (int)0x8000
 * The general equation is
 *   x % m < y % m            <=>    (x - y) % m < 0
 * where % is the modulo operation which produces positive numbers only.
 * If m is a power of 2, the equation simplifies to
 *   x & (m-1) < y & (m-1)    <=>    (x - y ) & (m-1) < 0
 * where & is the bitwise AND operation.
 * On most machines, the & operation is unnecessary when m is equal to the
 * size of the integer, eg, 0x10000 for a 16 bit integer. However, on
 * certain machines this is not the case.
 * The macro returns
 *   >0    if a > b
 *    0    if a = b
 *   <0    if a < b
 *
 * NOTE: a - b < 0 is not the same as a < b on typical machines. The reason
 * is that on these machines, a - b < 0 is a modulo operation, ie, it is
 * equivalent to (a - b) % m < 0, while a < b is not a modulo operation
 * and therefore is not equivalent to a % m < b % m.
 */
#define po_lib_CMP(a,b) ((a)-(b))

/* Bit manipulation
 */
#define po_lib_BIT_SET(x, bit) ((x) |= 1 << (bit))
#define po_lib_BIT_CLR(x, bit) ((x) &= ~(1 << (bit)))
#define po_lib_BIT_TOGGLE(x, bit) ((x) ^= 1 << (bit))
#define po_lib_BIT_TEST(x, bit) ((x) & (1 << (bit)))

/*-GLOBAL-INSERT-*/

/*-GLOBAL-
 * Returns MSB position, -1 if x is 0.
 * The function handles 16 and 32 bit integers.
 */
int po_lib_msb_sw(unsigned x)
;

/*-GLOBAL-INSERT-END-*/

static inline int po_lib_msb(unsigned x)
{
  #if po_lib_MSB_HW
  return po_lib_msb_hw(x);
  #else
  return po_lib_msb_sw(x);
  #endif
}

#endif // po_lib__H
