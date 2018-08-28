/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Miscellaneous library
 */

#include <po_sys.h>
#include <po_memory.h>

#if _TI_
#include <std.h>
#include <mem.h>
#define malloc(x)  MEM_alloc(0, (x), 8)

#else
#include <stdlib.h>
#endif

static int random1(void)
{
  /* Use 2 simple generators and change the phase
   * of their loops after each loop.
   */
  static int x = 12345;
  static int y = 23456;
  static int count = 0;

  x = 69069 * x + 1;
  y = 69069 * y + 1;

  if ( ++count == 0 )
    y++; // = 69069 * y + 1;

  return (x>>5) ^ y;
}

/*-GLOBAL-
 * Random function
 */
int mlRandomInt(void)
{
  return random1();
}

/*-GLOBAL-
 * Random number uniformly distributed in the interval [x, y[.
 */
int mlRandomUniform(int x, int y)
{
  if ( x == y ) return x;
  else {
    unsigned r = mlRandomInt();
    r = (r << 1) >> 1;  // get rid of sign bit
    return x + r % (y-x);
  }
}

/*-GLOBAL-
 * Random number with pseudo log base 2 distribution. The output is in
 * [2^x, 2^y[. The distribution is uniform in each interval i given by
 * [2^(i-1), 2^i[. The distribution of the intervals i is uniform.
 */
int mlRandomPseudoLog(int x, int y)
{
  int interval = 1 << mlRandomUniform(x,y);
  int r = mlRandomUniform(interval, interval*2);
  return r;
}

