/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Preprocessor code generation.
 */

#ifndef po_prep__H
#define po_prep__H

#include <po_sys.h>
#include <po_function.h>

po_prep_template_start;
/* Old entry */
%D__po__priorityfunction%dpo_function_ServiceHandle *po__srvhandle%A;

/* Priority function handle type. Includes function arguments */
typedef struct {
  po_function_Handle pfhandle; /* MUST BE FIRST */
%B  struct {%b
%B%n    %T;%b
%B  } args;%b
} %F__po__Handle;

/* Priority function handle when not dynamically allocated */
//static %F__po__Handle %F__po__handle;

/* Entry point from scheduler */
%D__po__schedulerentry(po_function_Handle *pfhandle)
{
  %B%F__po__Handle *handle = (%F__po__Handle*)pfhandle;%b
  %F__po__priorityfunction((po_function_ServiceHandle*)0%B, handle->args.%a%b);
  po_free(pfhandle);
}

/* New entry point */
%D%dpo_function_ServiceHandle *po__srvhandle%A
{
  int po__optspeed = %o == 0 ? !(po_OPTIMIZE_SIZE) : (%o > 0);
  int po__priority = (%P);
  %F__po__Handle *po__pfhandle;
  #if po_function_TRACK_NAME // DEBUG_MODE
  void *po__funcname = (void*)"%F";
  #elif po_DEBUG
  void *po__funcname = (void*)%F;
  #endif

  if ( po__optspeed && !po__srvhandle &&
       (unsigned)po__priority > (unsigned)po_function_getpri() ) {
    /* Immediate call */
    po_function_enternow(po__priority, po__funcname);
    %F__po__priorityfunction((po_function_ServiceHandle*)0%B, %a%b);
    po_function_exitnow();
    return;
  }

  /* Not an immediate call, pack arguments inside handle */
  po__pfhandle = po_smalloc_constP(sizeof(*po__pfhandle));
  if ( !po__pfhandle ) po_memory_error();
  //po__pfhandle = &%F__po__handle;

%B%n  po__pfhandle->args.%a = %a;%b

  #if po_function_TRACK_NAME // DEBUG_MODE
  po__pfhandle->pfhandle.name = po__funcname;
  #endif // DEBUG_MODE

  po_function_service((po_function_Handle*)po__pfhandle, po__srvhandle, %F__po__schedulerentry, po__priority);
}

/* Old entry */
%D__po__priorityfunction%dpo_function_ServiceHandle *po__srvhandle%A
po_prep_template_end

#endif // po_prep__H
