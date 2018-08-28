/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * YOU CANNOT MODIFY THIS FILE UNLESS YOU HAVE THE FULL PORTOS SOURCE
 * CODE IN ORDER TO RECOMPILE IT. OTHERWISE, MODIFYING THIS FILE WILL
 * RESULT IN SYSTEM CRASH.
 *
 * Configuration file containing default values that can be modified by
 * the programmer.
 */

#ifndef po_cfg_dspbios__H
#define po_cfg_dspbios__H


/* Total number of priority levels: 0 to po_priority_MAX */
#define po_function_NUM_PRI_LEVELS    14   //po_INT_SIZE

/* SWI levels used by Portos. These levels should not be used by any
 * SWI that could call priority functions */
#define po_target_SWI_PRI_HIGH         3
#define po_target_SWI_PRI_LOW          2
#define po_target_SWI_PRI_LOW_MASK     (1<<(po_target_SWI_PRI_LOW+1))


#endif // po_cfg_dspbios__H
