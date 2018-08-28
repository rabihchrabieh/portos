/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Test for po_function module.
 */

#include <po_sys.h>
#include <po_memory.h>
#include <po_function.h>
#include <po_prep.h>
#include <po_display.h>
#include <miscLib.h>

/* Enums
 */
enum {
  ePRIORITY_MIN = 0,
  ePRIORITY_MAX = po_function_NUM_PRI_LEVELS - 2, // Don't highest level
  eMAX_SCHEDULED = po_function_NUM_PRI_LEVELS * 5
};

/* Stack start (approximate start position)
 */
char *StackStart;

/* Total number of priority functions to call
 */
static int TotalCalls = 20000;
static int NCalls = 0;
static int Done = 0;
static int EnableHwi = 0;

/* Total number of errors encountered
 */
static int Errors = 0;

/* Number of scheduled or active priority functions per priority level
 */
static int Scheduled[ePRIORITY_MAX+1] = {0};
static int TotalScheduled = 0;
static int EmptyMode = 0;

/* Number of active priority functions per priority level
 */
static int Active[ePRIORITY_MAX+1] = {0};

/* Currently active ID in each priority level
 */
static int ActiveID[ePRIORITY_MAX+1] = {0};

/* Labels within each priority level. The IDs go from 'a' to 'z'
 * and repeat. In other words, they go from 0 to 25 and repeat.
 */
static int Label[ePRIORITY_MAX+1] = {0};

/* Caller ID, increments for each new call
 */
static int ID_caller = 0;

#if _TI_
static SWI_Handle Swi;
#endif

/* This function is called from po_function_call() to signal that a priority
 * function has effectively been scheduled */
void po_function_reportsched_(int priority)
{
  int protectState = po_interrupt_disable();
  Scheduled[priority]++;
  TotalScheduled++;
  po_interrupt_restore(protectState);
}

/* Display currently scheduled or active pfuncs. For each priority level
 * a label is added to denote the number of pfuncs at this level. A label
 * of 'a' means 1 pfunc. 'b' means 2 pfuncs, etc. 'z' means 26 pfuncs. '.'
 * means more than 26 pfuncs.
 * last character denotes, for example, when we are entering a
 * pfunc with a '+' sign, and when we are exiting a pfunc with a '-' sign.
 * This last char corresponds to the highest priority level, ie, the last
 * number displayed.
 * If we are only scheduling a new pfunc, it can be denoted with ' ' or
 * '*'. In this case, the scheduling may not apply to the highest level.
 * id identifies the highest priority level pfunc within its level. A label
 * from 'a' to 'z' is attached to each (just like their total number).
 */
static void scheduledDisplay(char last, int id)
{
  #if 0
  int i;
  char label;
  int protectState = po_interrupt_disable();
  for ( i = ePRIORITY_MIN ; i <= ePRIORITY_MAX ; i++ ) {
    if ( Scheduled[i] > 0 ) {
      label = '.';
      if ( Scheduled[i] <= 26 ) label = Scheduled[i] - 1 + 'a';
      po_log(" %2d%c", i, label);
    }
  }
  po_log(" %c%c", id + 'a', last);
  po_log("0x%x\n", (int)po_function_Env.bitmap, 0);
  po_interrupt_restore(protectState);
  #endif
}

/* Messages exchanged by the pfuncs
 */
typedef struct {
  int callerpri;
  int desiredpri;
  // the id of this pfunc. The id takes into account the number
  // of scheduled pfuncs at each level. This id permits to distinguish
  // between them.
  int id;
  int line;  // line number from which the call occured
  int id_caller;
} Message;

static void myfunc(po_priority(desiredpri),
		   int callerpri, int desiredpri, int label, int id_caller, int line);

void generateHWI(int prob);

/* Emulate HWI from po_function module
 */
void po_emulateirupt_(void)
{
  generateHWI(1);
}

/* Interrupt handler
 */
static void hardwareInterrupt(void)
{
  int count;

  if ( !EnableHwi ) return;

  // Set postpone flag
  po_interrupt_enter();

  //generateHWI(2);

  // Generate a new priority function
  if ( !Done && !EmptyMode &&
       TotalScheduled < eMAX_SCHEDULED &&
       NCalls < TotalCalls ) {
    count = mlRandomUniform(0,3);
    for ( ; count > 0 ; count-- ) {
      int priority = mlRandomUniform(ePRIORITY_MIN, ePRIORITY_MAX+1);
      int callerpri = po_function_getpri();
      int desiredpri = priority;
      int label = Label[priority]++ % 26;
      int id_caller = -1;
      myfunc(po_priority, callerpri, desiredpri, label, id_caller, __LINE__);
    }
  } else if ( TotalScheduled >= eMAX_SCHEDULED ) {
    EmptyMode = 1;
  }

  // Re-establish priority functions context
  po_interrupt_exit();
}

/* Generate hardware interrupt with some probability
 */
void generateHWI(int prob)
{
  // Generate the interrupt with probability
  if ( EmptyMode || TotalScheduled > eMAX_SCHEDULED ||
       mlRandomUniform(0,100) >= prob ) return;
  else {
    #if _TI_
    SWI_post(Swi);
    #else
    hardwareInterrupt();
    #endif
  }
}

#if _TI_
/* Hardware interrupt: periodically called from timer interrupt
 */
static void timerHWI(void)
{
  if ( !EnableHwi ) return;
  generateHWI(10);
}
#endif

/* The priority function that will run the test and that will call
 * itself at different priority levels as if it were several priority
 * functions.
 */
void myfunc(po_priority(desiredpri),
	    int callerpri, int desiredpri, int label, int id_caller, int line)
{
  int currpri = po_function_getpri();
  int i;
  int protectState;
  int id_mine;

  if ( Done ) return;
  
  protectState = po_interrupt_disable();
  id_mine = ID_caller++;
  NCalls++;
  po_interrupt_restore(protectState);

  if ( NCalls >= TotalCalls ) Done = 1;

  ActiveID[currpri] = id_mine;

  // Display scheduled and active pfuncs
  scheduledDisplay('+', label);

  if ( !(NCalls & 1023) )
    po_log("No errors yet, %d priority functions called (stack %d)\n", NCalls, StackStart - (char*)&line);

  // Sanity checking
  if ( currpri != desiredpri ||
       (currpri < ePRIORITY_MIN || currpri > ePRIORITY_MAX) ) {
    Errors++;
    po_log("ERROR: bad priority level %d, %d, ", callerpri, currpri);
    po_log("%d\n", desiredpri, 0);
    Done = 1;
    return;
  }

  // Increment number of active pfuncs at this level
  protectState = po_interrupt_disable();
  Active[currpri]++;
  po_interrupt_restore(protectState);

  // Verify that this is the highest scheduled priority level
  for ( i = ePRIORITY_MAX ; i > currpri ; i-- ) {
    if ( Scheduled[i] > 0 ) {
      Errors++;
      po_log("ERROR: this is not the highest priority level: %d > %d\n",
	     i, currpri);
      Done = 1;
      return;
    }
  }

  // Verify that if this priority function has higher priority level
  // than the caller, then it is executing before the caller is done.
  if ( currpri > callerpri &&
       id_caller != -1 && ActiveID[callerpri] != id_caller ) {
    Errors++;
    po_log("ERROR: higher priority level is being called after caller is done\n", 0, 0);
    Done = 1;
    return;
  }

  // Verify that if this priority function has a lower priority level than
  // the caller, it should be the only active priority function (ie, all
  // previously running ones are done).
  if ( currpri < callerpri && Active[currpri] > 1 ) {
    Errors++;
    po_log("ERROR: equal priority level is being called from higher priority level and before already active priority functions\n", 0, 0);
    Done = 1;
    return;
  }

  // Call more pfuncs. Stop after NCalls exceeds a certain limit
  if ( !Done && !EmptyMode &&
       TotalScheduled < eMAX_SCHEDULED &&
       NCalls < TotalCalls ) {
    int count = mlRandomUniform(0,3);
    for ( ; count > 0 ; count-- ) {
      int priority = mlRandomUniform(ePRIORITY_MIN, ePRIORITY_MAX+1);
      int callerpri = currpri;
      int desiredpri = priority;
      int label = Label[priority]++ % 26;
      int id_caller = id_mine;
      // Generate an interrupt with some probability
      //generateHWI(2);
      // Call pfunc
      myfunc(po_priority, callerpri, desiredpri, label, id_caller, __LINE__);
      // Generate an interrupt with some probability
      //generateHWI(2);
    }
  } else if ( TotalScheduled >= eMAX_SCHEDULED ) {
    EmptyMode = 1;
  }

  // Display scheduled pfuncs
  scheduledDisplay('-', label);

  ActiveID[currpri] = 0;

  protectState = po_interrupt_disable();
  Scheduled[currpri]--;
  TotalScheduled--;
  Active[currpri]--;
  po_interrupt_restore(protectState);
}

/* Test randomly called pfunc with random priorities. The idea is to check
 * if each priority function is called at the right point.
 */
int test_randomPfunc(void)
{
  po_log("\nTESTING random priority functions\n", 0, 0);

  #if _TI_
  {
    /* Start Timer and SWI that acts like HWI */
    SWI_Attrs attr = {0};
    attr.fxn = (SWI_Fxn)hardwareInterrupt;
    attr.priority = SWI_MAXPRI;
    Swi = SWI_create(&attr);
    
    #if _6x_
    HWI_dispatchPlug(14, (Fxn)timerHWI, -1, NULL);
    #elif _55_
    HWI_dispatchPlug(4, (Fxn)timerHWI, NULL);
    #else
    #error "Add dispatcher"
    #endif
  }
  #endif

  EnableHwi = 1;

  // Call pfunc with intermediate priority. pfunc will do the rest of the
  // test
  while ( !Done && NCalls < TotalCalls ) {
    int priority = mlRandomUniform(ePRIORITY_MIN, ePRIORITY_MAX+1);
    int callerpri = po_function_getpri();
    int desiredpri = priority;
    int label = Label[priority]++ % 26;
    int id_caller = -1;
    StackStart = (char*)&priority;
    myfunc(po_priority, callerpri, desiredpri, label, id_caller, __LINE__);
    EmptyMode = 0;
  }

  EnableHwi = 0;

  // Still allocated memory
  po_memory_display(&po_memory_RegionSystem);

  if ( Errors > 0 ) {
    po_log("FAILURE: there were %d errors\n", Errors, 0);
    return -1;
  } else {
    po_log("SUCCESS: %d priority functions called\n", NCalls, 0);
    return 0;
  }
}
