/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Cf. po_target_ti.h for a module description.
 */

#include <std.h>
#include <sem.h>
#include <swi.h>
#include <po_sys.h>
#include <po_memory.h>
#include <po_target_dspbios.h>
#include <po_function.h>

/* Used by TI GEL code
 */
const int po_gel_INFO        = ((po_memory_TRACK_ALLOC << 3)
				| ((!!po_DEBUG) << 5)
				| (po_function_TRACK_NAME << 6)
				| (po_function_NUM_PRI_LEVELS << 7));

const int po_gel_MEM_ADD     = po_memory_ADD;

/* Main data
 */
po_NEARFAR po_target_DataType po_target_Data;

/*-GLOBAL-
 * Error handling
 */
void po_target_error(int error)
{
  LOG_error("Portos ERROR %d", error);
  //LOG_error("  in file %s", filename);
  //LOG_error("  at line %d", line);
  po_abort();
}

/* SWI start function
 */
static void po_target_dspbios_swi(void)
{
  // Lower priority level
  SWI_restorepri(po_target_SWI_PRI_LOW_MASK);

  /* Resume priority functions scheduling */
  po_function_resume();
}

/*-GLOBAL-
 * Target init
 */
void po_target_init(void)
{
  SWI_Attrs attrS = {0};

  /* Create SWI */
  attrS.fxn = (SWI_Fxn)po_target_dspbios_swi;
  attrS.priority = po_target_SWI_PRI_HIGH;
  po_target_Data.swi_handle = SWI_create(&attrS);
  if ( !po_target_Data.swi_handle )
    po_error(po_error_CANNOT_CREATE_SWI);
}
