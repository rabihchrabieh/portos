/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Cf. po_function.h for a module description.
 *
 * In this version of the scheduler the database is a bitmap. One bit
 * per priority level indicating if there are awaiting priority functions
 * at this level. If there are, then the priority functions are stored in
 * a simple linked list. In order to insert at the end of the list and
 * retrieve from the beginning of the list, the simple linked list for
 * priority P(i) is organized as follows:
 *
 *      P(i)-->Last-->First-->Second-->Third-->Last (circular)
 *
 * In other words, it is a circular linked list and we maintain a pointer
 * at the Last element. We insert at the position of Last. We retrieve
 * from the position of First. The goal is to have a highly efficient
 * design both in terms of memory and speed. A double linked list may
 * be more efficient in some cases. To be studied.
 */

#include <po_sys.h>
#include <po_memory.h>
#include <po_lib.h>
#include <po_function.h>
#include <po_target_ti.h> // replace this with a general .h that points to specific, for example, add this in po_cfg.h

#define po_function_DEBUG DEBUG

/* Memory region for dynamic allocation
 */
enum { po_function_MEM_REGION = po_sys_MEM_REGION };

/* Currently active environment
 */
po_function_Environ *po_function_Env;

/* Use MSB or LSB as max priority in bitmap. This is architecture dependent
 * for best performance.
 */
#define USE_LSB 0

#if USE_LSB
/* Bitmap priority conversion: returns the bit in bitmap that corresponds
 * to priority, and vice versa. It should return -1 when the bitmap is
 * empty, but this doesn't work usually unless po_function_PRIORITY_LEVELS is
 * equal to the number of bits in an integer. Perhaps we could change
 * this function to (NUM_BITS_IN_INTEGER-1) - (priority), however this
 * requires knowledge of the number of bits in an integer.
 * Use the MSB version for the time being.
 */
#  define po_function_PRI_BIT(priority) ((po_function_PRIORITY_LEVELS-1) - (priority))

/* Returns max priority in bitmap.
 */
#  define po_function_PRI_MAX(bitmap) (po_function_PRI_BIT(po_lib_lsb_position(bitmap)))

#else // Use MSB
#  define po_function_PRI_BIT(priority) (priority)
#  define po_function_PRI_MAX(bitmap) (po_function_PRI_BIT(po_lib_msb_position(bitmap)))
#endif

/*-GLOBAL-
 * Execute all priority functions above prevpri level.
 * Or restore priority level after a priority raise.
 */
void po_function_priorityRestore(int prevpri)
{
  po_function_Environ *env = po_function_Env;

  do {
    int protectState;
    int priority;
    po_function_Node *first, *last;

    protectState = po_interrupt_disable();

    // Check if a new pfunc was installed and that has a priority
    // above prevpri (installed by a higher priority process or
    // a preempting interrupt).
    // Note that in the test below, the "<=" means everything that was
    // installed by interrupt level or a higher priority level will not
    // execute now if it has equal priority to prevpri.
    if ( env->maxpri <= prevpri ) {
      // Most common case
      // Resume previous pfunc
      env->currpri = prevpri;
      po_interrupt_restore(protectState);
      break;
    }

    po_function_reportRestore();

    // Modify the currpri to the new pfunc priority. After
    // this statement, any preempting code will be responsible for
    // any pfunc whose priority is higher than this new priority.
    priority = env->maxpri;
    env->currpri = priority;

    // Remove first node from linked list
    last = env->bitmapP[priority];
    first = last->next;

    if ( first == last ) {
      unsigned bitmap;
      if ( po_function_DEBUG ) env->bitmapP[priority] = NULL;
      // Empty list after this node
      po_lib_BIT_CLR(env->bitmap, po_function_PRI_BIT(priority));
      // Find next max priority (could be done in unprotected mode
      // using a loop).
      bitmap = env->bitmap;
      env->maxpri = po_function_PRI_MAX(bitmap);
      po_interrupt_restore(protectState);
    } else {
      last->next = first->next;
      po_interrupt_restore(protectState);
    }

    po_function_reportLower(env->currpri);

    // Call function now after node has been removed to keep the
    // database consistent: a running pfunc has no node in database.
    first->func(first->message);
    
    // Free the node
    po_free(first);
    
  } while ( 1 );  
}

/*-GLOBAL-
 * Call a pfunc. The priority of the pfunc is usually different from a
 * currently running pfunc. The new pfunc will start running immediately
 * if it's priority is higher. Otherwise, it will wait until higher
 * priority pfuncs are done.
 */
void po_function_call(po_function_Function func, int priority, void *message)
{
  po_function_Environ *env = po_function_Env;
  int protectState;

  if ( priority >= env->currpri ) {
    // This pfunc has higher or equal priority. It will get executed
    // immediately.
    // No need to create a node. Save previous priority on the stack.
    // Set current priority to the priority of the pfunc being executed.
    // Execute all nodes whose priority is above the previous priority
    // and return. Each time a pfunc is executed, modify currpri
    // to signal to preempting calls that they should be responsible
    // for any pfunc with priority above the current one being executed.
    int prevpri = env->currpri;

    env->currpri = priority;

    po_function_reportRaise(priority);
    po_function_reportScheduled(priority);

    // Call the function
    func(message);

    // Execute all pfuncs whose priority is above prevpri that could
    // have been installed since this function has been called.
    po_function_priorityRestore(prevpri);

    po_function_reportLower(prevpri);

  } else {
    // Priority of this pfunc is less than the currently running one.
    // Insert this pfunc in priority database.    
    po_function_Node *last;
    int priorityBit = po_function_PRI_BIT(priority);

    // Create node
    last = (po_function_Node*)po_malloc_index(env->memIndex.node,
					 po_function_MEM_REGION);
    if ( last == NULL ) {
      po_log_error(po_log_File, "po_function_call: cannot allocate memory for pfunc 0x%p\n", func);
      return;
    }

    // Set node info
    last->func = func;
    last->message = message;

    protectState = po_interrupt_disable();
    if ( !po_lib_BIT_TEST(env->bitmap, priorityBit) ) {
      // List is empty
      if ( priority > env->maxpri )
	env->maxpri = priority;
      env->bitmapP[priority] = last;
      last->next = last;
      po_lib_BIT_SET(env->bitmap, priorityBit);
      po_interrupt_restore(protectState);
    } else {
      // List is not empty
      po_function_Node *prev_last = (po_function_Node*)env->bitmapP[priority];
      last->next = prev_last->next;
      prev_last->next = last;
      env->bitmapP[priority] = last;
      po_interrupt_restore(protectState);
    }

    po_function_reportScheduled(priority);
  }
}

/*-GLOBAL-
 * For optimization and simplification purposes, some functions can raise
 * the priority level without calling a pfunc. In this case, the variables
 * in the stack are accessible (no need to malloc). The priority can only
 * be raised.
 * This function returns the previous priority level to be supplied to
 * po_function_priorityRestore() when restoring the priority level.
 * Basically, the code that needs to be protected with a mechanism similar
 * to "priority ceiling" in traditional RTOS (something like a semaphore),
 * should be wrapped with po_function_priorityRaise() and po_function_priorityRestore().
 */
int po_function_priorityRaise(int priority)
{
  po_function_Environ *env = po_function_Env;

  if ( priority >= env->currpri ) {
    int prevpri = env->currpri;
    env->currpri = priority;
    po_function_reportRaise(priority);
    return prevpri;
  } else {
    return env->currpri;
  }
}

/*-GLOBAL-
 * Create environment. Returns a pointer to the environment.
 * If environ is NULL, the function allocates an environ in dynamic
 * memory. Otherwise, the supplied environ is initialized.
 */
po_function_Environ *po_function_create(po_function_Environ *environ)
{
  if ( !environ )
    environ = po_malloc(sizeof(*environ), po_sys_MEM_REGION);

  // Init node malloc index
  environ->memIndex.node = po_memory_size2index(sizeof(po_function_Node));

  // Init curr priority level
  environ->currpri = -1;

  // Init bitmap
  environ->maxpri = -1;
  environ->bitmap = 0;

  if ( po_function_DEBUG ) {
    int i;
    for ( i = 0 ; i < po_function_PRIORITY_LEVELS ; i++ )
      environ->bitmapP[i] = NULL;
  }
  return environ;
}

#if po_function_DEBUG
/*-GLOBAL-
 * Displays the content of the bitmap database
 */
void po_function_bitmapDisplay(void)
{
  po_function_Environ *env = po_function_Env;
  int i;
  if ( !env->bitmap ) {
    po_log_printf(po_log_File, "pFUNC: BITMAP EMPTY\n");
    return;
  } 
  po_log_printf(po_log_File, "pFUNC: BITMAP CONTENT:\n");
  for ( i = 0 ; i < po_function_PRIORITY_LEVELS ; i++ ) {
    if ( po_lib_BIT_TEST(env->bitmap, i) ) {
      po_function_Node *last = env->bitmapP[i]->next;
      po_function_Node *node = last;
      po_log_printf(po_log_File, "Pri %2d: ", i);
      do {
	node = node->next;
	po_log_printf(po_log_File, "0x%p ", node->func);
      } while ( node != last );
      po_log_printf(po_log_File, "\n");
    }
  }
}
#endif
