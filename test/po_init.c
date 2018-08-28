/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Initialization file. Modify it as needed.
 *
 * The reason for this file is that the C language (especially C89) does
 * not provide a clean method to define structures containing arrays of
 * "variable" size. Hence, we define these structures here, and we refer to
 * them as "fixed" size in other files.
 * The structures defined are for memory regions, signal groups and clocks.
 */

// This macro is defined in this file only.
#define po_INIT_FILE

#include <portos.h>

#if TARGET_SLOW && CHAR_BIT == 16
#  define heapSize 100000
#else
#  define heapSize 200000
#endif
static char Heap[3][heapSize];

// At least the default memory region must be defined
po_memory_Region(po_memory_RegionDefault, Heap[0], heapSize, 10000);
// Declare 2 more memory regions
po_memory_Region(Region1, Heap[1], heapSize, 10000);
po_memory_Region(Region2, Heap[2], heapSize, 10000);

// Zero or more signal groups can be defined
po_signal_Group(po_signal_GroupDefault, po_priority_MAX, 8, &po_memory_RegionDefault);
// Create a second signal group
#define SIGNALS 20
po_signal_Group(Group, po_priority_MAX, SIGNALS, /* non-power of 2 */
		&po_memory_RegionDefault);

// Zero or more clocks can be defined
po_time_Clock(po_time_ClockDefault, po_priority_MAX, 16, &po_memory_RegionDefault);

// This one does not need to be in this file
static int LogBuffer[1024];
po_log_Handle po_log_HandleDefault =
  po_log_INIT(LogBuffer, sizeof(LogBuffer)/sizeof(int), po_priority_MAX);

// One time initialization function that should be called from main()
// or elsewhere.
void po_init(void)
{
  po_memory_regioninit((po_memory_Region*)&po_memory_RegionDefault);
  po_memory_regioninit((po_memory_Region*)&Region1);
  po_memory_regioninit((po_memory_Region*)&Region2);
  po_signal_groupinit((po_signal_Group*)&po_signal_GroupDefault);
  po_signal_groupinit((po_signal_Group*)&Group);
  po_time_clockinit((po_time_Clock*)&po_time_ClockDefault);

  // Call this function at the end
  po_init_();
}
