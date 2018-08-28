/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * Cf. po_function.h for a module description
 *
 * When calling a priority function (pfunc) a dynamic node is created. The
 * dynamic node is destroyed just before the pfunc's execution starts.
 * If the pfunc is to be executed right away because it has a priority
 * level higher than the current level, the node is not created.
 * The nodes are inserted in the priority database which is a binary tree
 * sorted according to priority levels. The nodes can also be inserted in
 * a sorted linear list for faster access and deletion. This list is not
 * implemented because it doesn't seem too attractive. Rather, a pointer
 * to the maximum priority node in the tree is maintained.
 * A nice feature of the tree structure is that nodes are only inserted as
 * leaves by preempting processes. A linear list does not have this nice
 * feature and requires more protection against preemption (eg, by locking
 * interrupts).
 *
 * Below is an example of the priority database: binary tree and list.
 * The nodes are pfunc calls. The number at each node is the priority level.
 * First the node 9 is executed, then 8, then 7, etc.
 *
 *          Binary tree         Priority list (not implemented)
 *                2                9->8->7->6->5->4->3->2->1
 *              /   \
 *             6     1
 *           /   \
 *          8     4
 *         / \   / \
 *        9  7  5   3
 */

#include <po_sys.h>
#include <po_memory.h>
#include <pfunc_tree.h>

#define po_function_DEBUG DEBUG

/* Memory region for dynamic allocation
 */
enum { po_function_MEM_REGION = po_sys_MEM_REGION };

/* pfunc dynamic node. This node is allocated for each pfunc call.
 * When the pfunc is executed, the node is freed.
 */
typedef struct _po_function_Node {
  po_function_Function func;                // pfunc function
  int priority;                       // pfunc priority
  void *message;                      // pfunc message

  // Priority database pointers. The pointers are not defined as
  // volatile but care should be taken when accessing these pointers.
  struct _po_function_Node *higher; // higher priority child in the tree
  struct _po_function_Node *lower;  // lower priority child in the tree

} po_function_Node;

/* Data structure
 */
static struct {
  // Current priority level
  volatile int currPriority;

  // Postpone flag. If set to non-zero, calls to po_function_call will not
  // schedule pfuncs to execute immediately. This is useful, for example,
  // to postpone calls to pfuncs when calling from interrupt level.
  volatile int postpone;

  // Mem alloc indexes
  struct {
    int node;
  } memIndex;

  // Priority database
  struct {
    volatile po_function_Node *tree; // top of binary tree
    volatile po_function_Node *max;  // max priority node in tree
  } db;

} po_function_Data;

/*-GLOBAL-
 * Call a pfunc. The priority of the pfunc is usually different from a
 * currently running pfunc. The new pfunc will start running immediately
 * if it's priority is higher. Otherwise, it will wait until higher
 * priority pfuncs are done.
 */
void po_function_call(po_function_Function func, int priority, void *message)
{
  int protectState;

  if ( priority >= po_function_Data.currPriority && !po_function_Data.postpone ) {
    // This pfunc has higher or equal priority. It will get executed
    // immediately (unless we have issued a postpone instruction, eg,
    // from interrupt level).
    // No need to create a node. Save previous priority on the stack.
    // Set current priority to the priority of the pfunc being executed.
    // Execute all nodes whose priority is above the previous priority
    // and return. Each time a pfunc is executed, modify currPriority
    // to signal to preempting calls that they should be responsible
    // for any pfunc with priority above the current one being executed.
    int prevPriority = po_function_Data.currPriority;
    po_function_Data.currPriority = priority;

    // Call the function
    func(message);

    // Execute all pfuncs whose priority is above prevPriority that could
    // have been installed since this function has been called.
    do {
      po_function_Node *node;

      protectState = po_interrupt_disable();
      // Check if a new max node was installed and that has a priority
      // above prevPriority (installed by a higher priority process or
      // a preempting interrupt).
      (volatile po_function_Node*)node = po_function_Data.db.max;
      if ( node == NULL ) {
	// Resume previous pfunc
	po_function_Data.currPriority = prevPriority;
	po_interrupt_restore(protectState);
	break;
      }

      // Note that in the test below, the ">" means everything that was
      // installed by interrupt level or a higher priority level will not
      // execute now if it has equal priority to prevPriority.
      if ( node->priority > prevPriority ) {
	po_function_Node *parent;

	// Modify the currPriority to the new pfunc priority. After
	// this statement, any preempting code will be responsible for
	// any pfunc whose priority is higher than this new priority.
	// Note that max node pointer is invalid after this operation
	// but preempting processes should not be affected.
	po_function_Data.currPriority = node->priority;
	po_interrupt_restore(protectState);
	
	// Find the node's parent.
	// Remove node from tree. The operation has to be atomic because
	// a preempting process can change the value of node->lower (if it
	// was NULL) after the code here has retrieved it into a register.
	(volatile po_function_Node*)parent = po_function_Data.db.tree;
	if ( parent != node ) {
	  while ( parent->higher != node ) parent = parent->higher;
	  protectState = po_interrupt_disable();
	  parent->higher = node->lower;
	  po_interrupt_restore(protectState);
	} else {
	  protectState = po_interrupt_disable();
	  po_function_Data.db.tree = node->lower;
	  // It is convenient to update the max pointer if the tree
	  // is NULL. Otherwise, it is a pain to update below.
	  if ( node->lower == NULL ) po_function_Data.db.max = NULL;
	  po_interrupt_restore(protectState);
	  parent = node->lower;
	}

	// Call function now after node has been removed to keep the
	// tree consistent: a running pfunc has no node in the tree.
	// However, it may be more efficient to call the function before
	// removing the node. The max pointer will benefit if we do
	// remove the node, and the tree will be more trimmed. So the
	// issue is debatable.
	// NOTE: the current max pointer is invalid. However it will not
	// be accessed by a preempting process unless a node with priority
	// function this max is inserted. In this case, the max gets
	// automatically updated by the preempting process.
	node->func(node->message);

	// Free the node
	po_free(node);

 	// Update the max pointer. If the tree is empty, the max pointer
	// has been updated in the protected area above.
	if ( parent != NULL ) {
	  do {
	    // Find new max
	    while ( parent->higher ) parent = parent->higher;
	    // Check under protected mode that this is still max node
	    protectState = po_interrupt_disable();
	    if ( (volatile po_function_Node*)parent->higher == NULL ) {
	      // Yes, this is still max node
	      po_function_Data.db.max = parent;
	      po_interrupt_restore(protectState);
	      break;
	    }
	    po_interrupt_restore(protectState);
	  } while ( 1 );
	}

      } else {
	// Resume the previous pfunc
	po_function_Data.currPriority = prevPriority;
	po_interrupt_restore(protectState);
	break;
      }
    } while ( 1 );

  } else {
    // Priority of this pfunc is less than the currently running one.
    // Insert this pfunc in priority database.    
    po_function_Node *parent;
    po_function_Node *node;

    // Create node
    node = po_malloc_index(po_function_Data.memIndex.node, po_function_MEM_REGION);
    if ( node == NULL ) {
      po_log_error("po_function_call: cannot allocate memory for pfunc 0x%p\n", func);
      return;
    }

    // Set node info
    node->func = func;
    node->priority = priority;
    node->message = message;
    node->higher = NULL;
    node->lower = NULL;

    // Check if tree is empty in protected mode
    protectState = po_interrupt_disable();
    (volatile po_function_Node*)parent = po_function_Data.db.tree;
    
    if ( parent != NULL ) {
      po_interrupt_restore(protectState);
      // Search the tree to locate the parent node.
      // At the instant we're trying to add "node" as a leaf, more nodes
      // could have been added by preempting processes. Another check
      // should be performed in protected mode, and the search should
      // continue if more nodes are found.
      do {
	if ( priority > parent->priority ) {
	  // Go into higher priority branch
	  if ( parent->higher == NULL ) {
	    // Leaf found. Check again in protected mode.
	    protectState = po_interrupt_disable();
	    if ( (volatile po_function_Node*)parent->higher == NULL ) {
	      parent->higher = node;
	      if ( priority > po_function_Data.db.max->priority )
		po_function_Data.db.max = node;
	      po_interrupt_restore(protectState);
	      break;
	    }
	    po_interrupt_restore(protectState);
	  }
	  parent = parent->higher;
	} else {
	  // Go into lower priority branch
	  if ( parent->lower == NULL ) {
	    // Leaf found. Check again in protected mode.
	    protectState = po_interrupt_disable();
	    if ( (volatile po_function_Node*)parent->lower == NULL ) {
	      parent->lower = node;
	      po_interrupt_restore(protectState);
	      break;
	    }
	    po_interrupt_restore(protectState);
	  }
	  parent = parent->lower;
	}
      } while ( 1 );

    } else {
      // Priority database is empty
      po_function_Data.db.tree = node;
      po_function_Data.db.max = node;
      po_interrupt_restore(protectState);
    }
  }
}

/*-GLOBAL-
 * Returns the current priority level
 */
int po_function_currPriority(void)
{
  return po_function_Data.currPriority;
}

/*-GLOBAL-
 * Postpones all pfunc calls until a po_function_resume() is issued.
 * This is useful to call at interrupt level before calling po_function_call()
 * so that pfuncs don't execute at interrupt level. The pfuncs will get
 * executed later when a po_function_resume() is issued at software interrupt
 * level or equivalent.
 */
void po_function_postpone(void)
{
  po_function_Data.postpone = 1;
}

/* Dummy pfunc that is used by po_function_resume()
 */
static void po_function_resume_dummy_pf(void *message)
{
}

/*-GLOBAL-
 * Resumes the execution of pfuncs (if some were postponed).
 * Call this from the software interrupt, or equivalent, that 
 * is supposed to handle postponed pfuncs (scheduled at hardware interrupt
 * level but postponed to run here).
 */
void po_function_resume(void)
{
  // Reset postpone flag
  po_function_Data.postpone = 0;
  
  // Are there pfuncs postponed?
  if ( po_function_Data.db.max ) {
    int postponedPriority = po_function_Data.db.max->priority;
    if ( postponedPriority > po_function_Data.currPriority ) {
      // Make a dummy call to po_function_call(). The dummy call will restart the
      // engine embedded inside po_function_call() which will run the postponed
      // pfuncs.
      po_function_call(po_function_resume_dummy_pf, postponedPriority, NULL);
    }
  }
}

/*-GLOBAL- (better if inlined)
 * For optimization purposes, some pfuncs can take a message over stack
 * or can return a result inside the message they received. In this case,
 * it is mandatory that the pfunc has a higher or equal priority than the
 * current (caller) pfunc. This function checks that, and if the condition
 * is not verified, the code prints an error and crashes.
 * The postpone flag cannot be set either (ie, this particular pfunc cannot
 * be called from interrupt level).
 */
void po_function_priorityCheck(int priority)
{
  if ( priority >= po_function_Data.currPriority && !po_function_Data.postpone ) {
    // This is OK
  } else {
    po_log_error("po_function_priorityCheck: failed\n");
    po_crash();
  }
}

/*-GLOBAL- (better if inlined)
 * For optimization and simplification purposes, some functions can raise
 * the priority level without calling a pfunc. In this case, the variables
 * in the stack are accessible (no need to malloc). The priority can only
 * be raised.
 * This function returns the previous priority level to be supplied to
 * po_function_priorityRestore() when restoring the priority level.
 * This function cannot be called from interrupt level. Define normal pfuncs
 * if they need to be called from interrupt level.
 * Basically, the code that needs to be protected with a mechanism similar
 * to "priority ceiling" in traditional RTOS (something like a semaphore),
 * should be wrapped with po_function_priorityRaise() and po_function_priorityRestore().
 */
int po_function_priorityRaise(int priority)
{
  if ( priority >= po_function_Data.currPriority && !po_function_Data.postpone ) {
    int prevPriority = po_function_Data.currPriority;
    po_function_Data.currPriority = priority;
    return prevPriority;
  } else {
    if ( po_function_Data.postpone )
      po_log_error("po_function_priorityRaise: cannot be called from interrupt "
		 "level\n");
    else
      po_log_error("po_function_priorityRaise: cannot be called from a higher "
		 "priority level %d > %d\n",
		 po_function_Data.currPriority, priority);

    po_crash();
    return 0;
  }
}

/*-GLOBAL- (better if inlined)
 * Use this function after a call to po_function_priorityRaise() to restore
 * the priority level. The argument returned by priorityRaise() should be
 * supplied to this function.
 */
void po_function_priorityRestore(int prevPriority)
{
  // Restore the prevPriority level
  po_function_Data.currPriority = prevPriority;

  // This is sort of similar to po_function_resume(). Some pfuncs may have been
  // delayed due to priority raising. Execute them before returning to
  // prevPriority.
  // Are there pfuncs postponed?
  if ( po_function_Data.db.max ) {
    int delayedPriority = po_function_Data.db.max->priority;
    if ( delayedPriority > prevPriority ) {
      // Make a dummy call to po_function_call(). The dummy call will restart the
      // engine embedded inside po_function_call() which will run the delayed
      // pfuncs.
      po_function_call(po_function_resume_dummy_pf, delayedPriority, NULL);
    }
  }
}

/*-GLOBAL-
 * One time init of this module
 */
void po_function_init(void)
{
  static int isInit = 0;
  if ( isInit ) return;
  isInit = 1;

  // Init node malloc index
  po_function_Data.memIndex.node = po_memory_size2index(sizeof(po_function_Node));

  // Init curr priority level
  po_function_Data.currPriority = -1; // What is a convenient choice?

  // Init binary tree
  po_function_Data.db.tree = NULL;
  po_function_Data.db.max = NULL;
}

#if po_function_DEBUG
/* Recursive function of the tree display
 */
static void po_function_treeDisplayR(volatile po_function_Node *subtree)
{
  if ( !subtree ) return;

  po_function_treeDisplayR(subtree->higher);
  po_log_fprintf(po_logFile, "%d ", subtree->priority);
  po_function_treeDisplayR(subtree->lower);
}

/*-GLOBAL-
 * Displays sorted nodes in tree. Works recursively.
 */
void po_function_treeDisplay(void)
{
  po_log_fprintf(po_logFile, "pFUNC: TREE: ");
  po_function_treeDisplayR(po_function_Data.db.tree);
  po_log_fprintf(po_logFile, "\n");
}
#endif
