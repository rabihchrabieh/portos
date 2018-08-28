/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Test for po_lib module.
 */

#include <po_lib.h>
#include <po_log.h>

int test_lib()
{
  unsigned i, j;
  int error = 0;
  
  po_log("TESTING po_lib_msb\n", 0, 0);

  printf("AAA %d  %d\n", po_lib_msb(0), po_lib_msb(3));

  if ( po_lib_msb(0) != -1 ) {
    error = -1;
  } else {
    j = 1;
    for ( i = 2 ; i != 0 ; i <<= 1 ) {
      if ( po_lib_msb(i-1) != j-1 ||
	   po_lib_msb(i) != j ||
	   po_lib_msb(i+1) != j ) {
	error = -1;
	break;
      }
      j++;
    }
  }

  if ( error ) {
    po_log("There are ERRORS\n", 0, 0);
    return -1;
  } else {
    po_log("SUCCESS\n", 0, 0);
    return 0;
  }
}
