/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Test for phash module.
 */

#include <po_memory.h>
#include <po_list.h>
#include <po_hash.h>
#include <po_display.h>
#include <miscLib.h>

#if TARGET_SLOW
static long NTries = 10000;
#else
static long NTries = 100000;
#endif

/* Hash table size
 */
#define HashSize 16

/* Number of objects
 */
#define NObjects 1000

/* Hash table pointer
 */
po_hash_Table *HashTable;

/* Objects type to store in hash table
 */
typedef struct {
  po_list_Node listNode; /* MUST BE FIRTS */
  int value;
  int inserted;
  int valid;
} ObjectType;

/* Allocate some objects
 */
ObjectType Objects[NObjects];

/* Test random hash insert and delete.
 */
int test_randomHash(void)
{
  long t, count;
  int errors = 0;

  po_log("\nTESTING hash table module\n", 0, 0);
  
  HashTable = po_hash_create(HashSize, &po_memory_RegionDefault);
  // Init the values of the objects
  for ( t = 0 ; t < NObjects ; t++ ) {
    ObjectType *o = &Objects[t];
    o->value = mlRandomUniform(0, HashSize*2);
    o->valid = o->value;
    o->inserted = 0;
  }

  // Randomly insert and delete objects
  for ( t = 0 ; t < NTries ; t++ ) {
    // Pick an object
    int i = mlRandomUniform(0,NObjects);
    ObjectType *o = &Objects[i];
    int operation;

    // Choose an operation: 0 = insert, 1 = delete, 2 = delete object
    if ( !o->inserted ) operation = 0;
    else operation = (mlRandomInt() & 0x1) + 1;

    switch ( operation ) {
    case 0: // insert
      po_hash_insert(HashTable, o->value, &o->listNode);
      o->inserted = 1;
      break;
    case 1: // delete by value (ie, all objects with the same value)
      {
	po_list_List *list = po_hash_remove(HashTable, o->value);
	po_list_Node *node = po_list_head(list);
	while ( node != list ) {
	  ObjectType *object = (ObjectType*)node;
	  if ( object->value != object->valid ) {
	    // Invalid object!
	    po_log("ERROR: invalid object\n", 0, 0);
	    errors++;
	  }
	  if ( !object->inserted ) {
	    po_log("ERROR: object was not inserted\n", 0, 0);
	    errors++;
	  }
	  object->inserted = 0;
	  node = po_list_next(node);
	}
	po_free(list);
      }
      break;
    case 2: // delete object
      po_hash_removeobj(HashTable, &o->listNode);
      o->inserted = 0;
      break;
    default:
      errors++;
      po_log("ERROR: bad operation\n", 0, 0);
      break;
    }
  }

  // Delete some remaining objects
  if ( 1 ) {
    count = 0;
    for ( t = 0 ; t < NObjects ; t++ ) {
      ObjectType *o = &Objects[t];
      if ( o->inserted ) {
	if ( mlRandomInt() & 0x1 ) po_hash_removeobj(HashTable, &o->listNode);
	else count++;
      }
    }
  }

  // Still in hash table?
  if ( po_hash_display(HashTable) != count ) {
    po_log("ERROR: hash table count is incorrect\n", 0, 0);
    errors++;
  }

  po_hash_delete(HashTable);
  
  // Still allocated memory
  po_memory_display(&po_memory_RegionDefault);

  // Success?
  if ( errors > 0 ) {
    po_log("There were %d errors\n", errors, 0);
    return -1;
  } else {
    po_log("SUCCESS: %ld objects inserted and deleted\n", NTries, 0);
    return 0;
  }
}
