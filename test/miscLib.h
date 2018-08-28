/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Miscellaneous library
 */

#ifndef MISCLIB_H
#define MISCLIB_H

/*-GLOBAL-
 * Random function
 */
int mlRandomInt(void)
;

/*-GLOBAL-
 * Random number uniformly distributed in the interval [x, y[.
 */
int mlRandomUniform(int x, int y)
;

/*-GLOBAL-
 * Random number with pseudo log base 2 distribution. The output is in
 * [2^x, 2^y[. The distribution is uniform in each interval i given by
 * [2^(i-1), 2^i[. The distribution of the intervals i is uniform.
 */
int mlRandomPseudoLog(int x, int y)
;

/*-GLOBAL-
 * Create a memory region
 */
void mlCreateMemRegion(int region, int heapSize, int maxBlockSize)
;

/*-GLOBAL-
 * Init the portos library. If defaultMemRegion flag is non-zero, a
 * default memory region is created.
 */
void mlPortosInit(int defaultMemRegion)
;

#endif // MISCLIB_H
