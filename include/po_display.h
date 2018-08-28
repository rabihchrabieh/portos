/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Printing functions for debugging Portos.
 */

#ifndef po_display__H
#define po_display__H

#include <po_hash.h>
#include <po_hashp.h>

/*-GLOBAL-INSERT-*/

/*-GLOBAL-
 * Displays all memory allocations and the source of the allocation, file
 * name and line number.
 */
int po_memory_display(po_memory_Region *region)
;

/*-GLOBAL-
 * Display the hash table content. It returns the total number of objects
 * in the hash table.
 */
int po_hash_display(po_hash_Table *hashTable)
;

/*-GLOBAL-
 * Display the hash table content. It returns the total number of objects
 * in the hash table.
 * THIS FUNCTION CAN ONLY BE CALLED FROM A LOWER PRIORITY LEVEL in order
 * to return a value. An error is generated if called from a higher priority
 * level.
 */
int po_hashp_display(po_hashp_Table *hTablep)
;

/*-GLOBAL-INSERT-END-*/

#endif // po_display__H
