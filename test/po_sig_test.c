/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Test for po_signal module.
 */

#include <portos.h>
#include <po_display.h>
#include <miscLib.h>

/* Some constants
 */
#define SIGNALS          20
#define PRIORITY_MIN     1
#define PRIORITY_MAX     (po_function_NUM_PRI_LEVELS - 2)
#define PRIORITY_SIGNALS (PRIORITY_MAX + 1)

/* Second signal group
 */
extern po_signal_Group Group;

/* Number of calls and errors
 */
#if TARGET_SLOW
static long StopCalls = 5000;
#else
static long StopCalls = 100000;
#endif
static long NCalls = 0;
static int Errors = 0;
static int Detached = 0;

static int Done = 0;

/* Keep track of the number of attached pfuncs per signal
 */
int Attached[SIGNALS];

/* Message transmitted to pfunc
 */
typedef struct {
  struct {
    po_list_Node node;        // MUST BE FIRST
    po_signal_Handle handle;  // If a handle is used
    int state;                // 0: not in list, 1: in list
  } h;
  int signal;
  int priority;
} Message;

/* Linked list to maintain those attached handles
 */
static struct {
  po_list_List list;
} List;

/* Priority function
 */
static void myfunc(po_priority(msg->priority), Message *msg)
{
  int protectState;
  int count;

  if ( Done ) return;

  protectState = po_interrupt_disable();

  NCalls++;

  // If this pfunc was attached to a signal
  if ( msg->signal >= 0 ) {
    if ( --Attached[msg->signal] < 0 ) {
      // Sanity checking failed: there are more pfuncs called than attached
      po_interrupt_restore(protectState);
      Attached[msg->signal] = 0;
      Errors++;
      po_log("ERROR: more pfuncs called than attached, signal %d\n",
	     msg->signal, 0);
      Done = 1;
      return;
    }

    // Remove from data base if a handle was used and not yet removed
    if ( msg->h.state == 1 ) {
      po_list_pop(&msg->h.node);
    }
  }

  po_interrupt_restore(protectState);

  if ( !Done && NCalls < StopCalls ) {
    // Attach more pfuncs
    count = mlRandomUniform(0,4);
    for ( ; count > 0 ; count-- ) {
      int priority = mlRandomUniform(PRIORITY_MIN, PRIORITY_MAX+1);
      int signal = mlRandomUniform(0, SIGNALS);
      Message *message = po_malloc(sizeof(*message));
      message->signal = signal;
      message->priority = priority;
      message->h.state = 0;
      if ( signal < 0 ) {
	// Call as regular pfunc
	myfunc(po_priority, message);
      } else {
	// Attach to signal
	// Choose if a handler will be created that allow to detach (cancel)
	// later
	int group = mlRandomInt() & 0x1;
	if ( mlRandomInt() & 0x1 ) {
	  message->h.state = 1;
	  po_signal_init(&message->h.handle);
	  protectState = po_interrupt_disable();
	  po_list_pushtail(&List.list, &message->h.node);
	  Attached[signal]++;
	  po_interrupt_restore(protectState);
	  if ( !group )
	    myfunc(po_signal_ph(signal, &po_signal_GroupDefault, &message->h.handle),
		   message);
	  else
	    myfunc(po_signal_ph(signal, &Group, &message->h.handle),
		   message);
	} else {
	  protectState = po_interrupt_disable();
	  Attached[signal]++;
	  po_interrupt_restore(protectState);
	  if ( !group )
	    myfunc(po_signal(signal), message);
	  else
	    myfunc(po_signal_p(signal, &Group), message);
	}
      }
    }

    // Post signals
    if ( 1 ) {
      int signal = mlRandomUniform(0, SIGNALS);
      po_signal_post(signal, &po_signal_GroupDefault);
      signal = mlRandomUniform(0, SIGNALS);
      po_signal_post(signal, &Group);
    }

    // Detach some pfunc that were assigned handlers
    if ( !po_list_isempty(&List.list) && mlRandomUniform(0,100) < 75 ) {
      int tail = mlRandomInt() & 0x1;
      int protectState = po_interrupt_disable();
      po_list_Node *node = tail ?
	po_list_poptail(&List.list) : po_list_pophead(&List.list);
      if ( node ) {
	Message *message = (Message*)node;
	if ( message->h.state > 0 ) {
	  protectState = po_interrupt_disable();
	  Detached++;
	  Attached[message->signal]--;
	  message->h.state = 0;
	  po_interrupt_restore(protectState);
	  po_signal_detach(&message->h.handle);
	  po_free(message);
	}
      }
    }
  }

  po_free(msg);
}

/* Test random signal attach and post
 */
int test_randomSignals(void)
{
  int signal;

  po_log("\nTESTING priority functions attached to signals randomly\n", 0, 0);

  po_list_init(&List.list);

  // Start
  do {
    Message *message = po_malloc(sizeof(*message));
    message->signal = -1;
    message->priority = PRIORITY_MIN;
    message->h.state = 0;
    myfunc(po_priority, message);
  } while ( !Done && NCalls < StopCalls );

  // Post all signals to process all remaining attached pfuncs
  for ( signal = 0 ; signal < SIGNALS ; signal++ ) {
    po_signal_post(signal, &po_signal_GroupDefault);
    po_signal_post(signal, &Group);
  }

  // Are there left over pfuncs that were not called?
  for ( signal = 0 ; signal < SIGNALS ; signal++ ) {
    if ( Attached[signal] != 0 ) {
      Errors++;
      po_log("ERROR: signal %d: still attached %d pfuncs\n",
	     signal, Attached[signal]);
    }
  }

  // Check data base
  if ( !po_list_isempty(&List.list) ) {
    po_log("ERROR: Data base not empty\n", 0, 0);
    Errors++;
  }

  // Still allocated memory
  po_memory_display(&po_memory_RegionDefault);

  // Success?
  if ( Errors > 0 ) {
    po_log("There were %d errors (%d detached pfuncs)\n", Errors, Detached);
    return -1;
  } else {
    po_log("SUCCESS: More than %ld pfuncs attached and called, %d detached\n",
	   NCalls, Detached);
    return 0;
  }
}
