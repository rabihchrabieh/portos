/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Compatibility header.
 * Currently supports GCC, MSVC++, Python, and TI.
 */

#ifndef PCOMPAT_H
#define PCOMPAT_H

#ifndef __cplusplus  // Cross compilation in C mode    
#   define inline  __inline  // VC++ does not recognize inline as C keyword

#else                // cpp mode
#  ifdef _MSC_VER
#    undef staticforward
#    undef statichere
#    define staticforward extern
#    define statichere static
#  endif
#endif

#ifdef PYTHON_TARGET
#include <python.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#endif

#endif // PCOMPAT_H
