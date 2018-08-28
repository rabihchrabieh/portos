/*
 * Portos v1.7.0
 * Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
 *
 * This module implements a circular double linked list of nodes.
 * Each node normally refers to an object. It is up to the programmer
 * to connect the node and the object, for example, by placing the
 * node as the first field in the object's structure (or by using
 * the offset of the node inside the object). po_list_getobj() helps
 * obtaining the object from the node, given the structure and field.
 *
 * The list has the following structure where "List" is a dummy node:
 *
 *   List <-> First <-> Second <->...<-> Last <-> List     (circular)
 *
 * Insertion and deletion can be done at any position in the list.
 * The list can also be parsed object by object.
 * Hence a fifo, a lifo, and other types of lists can be implemented
 * using this module.
 *
 * The dummy node is just like any other node, and parsing the list
 * can stop when we reach the dummy node (i.e., the list node).
 */

#ifndef po_list__H
#define po_list__H

#include <po_sys.h>

/* List Node.
 * This node can be the first field of an object's structure, or it can
 * be a field at a fixed offset from the start.
 */
typedef struct _po_list_Node {
  struct _po_list_Node *next;
  struct _po_list_Node *prev;
} po_list_Node;

/* List.
 * This also acts as a dummy node.
 */
typedef po_list_Node po_list_List;

/*-GLOBAL-
 * Initialization macro for statically allocated lists. For example,
 *   po_list_List mylist = po_list_INIT(&mylist);
 */
#define po_list_INIT(listPtr) {(listPtr), (listPtr)}

/*-GLOBAL-
 * Initialization routine for dynamically allocated lists.
 */
static inline void po_list_init(po_list_List *list)
{
  list->next = list->prev = list;
}

/* For debugging purposes, after removing a node from the list, this
 * function sets the pointers to NULL so that invalid accesses can be
 * detected.
 */
static inline void po_list_reset(po_list_Node *node)
{
  #if po_DEBUG // DEBUG_MODE
  if ( node->next != node )  // Do not reset the "dummy" node
    node->next = node->prev = NULL;
  #endif // DEBUG_MODE
}

/* Sanity checking in debug mode.
 */
static inline void po_list_verify(po_list_Node *node)
{
  #if po_DEBUG // DEBUG_MODE
  if ( node->next->prev != node || node->prev->next != node )
    po_error(po_error_LIST_CORRUPT);
  #endif // DEBUG_MODE
}

/*-GLOBAL-
 * Push object after reference object.
 */
static inline void po_list_pushafter(po_list_Node *ref, po_list_Node *node)
{
  po_list_verify(ref);     /* Debug mode only */
  node->next = ref->next;
  node->prev = ref;
  ref->next->prev = node;
  ref->next = node;
}

/*-GLOBAL-
 * Push object before reference object.
 */
static inline void po_list_pushbefore(po_list_Node *ref, po_list_Node *node)
{
  po_list_verify(ref);     /* Debug mode only */
  node->prev = ref->prev;
  node->next = ref;
  ref->prev->next = node;
  ref->prev = node;
}

/*-GLOBAL-
 * Push object at head of list.
 */
static inline void po_list_pushhead(po_list_List *list, po_list_Node *node)
{
  po_list_pushafter(list, node);
}

/*-GLOBAL-
 * Push object at tail of list.
 */
static inline void po_list_pushtail(po_list_List *list, po_list_Node *node)
{
  po_list_pushbefore(list, node);
}

/*-GLOBAL-
 * Pop object from head of list.
 * The node is removed from head of list and returned.
 */
static inline po_list_Node *po_list_pophead(po_list_List *list)
{
  po_list_Node *node = list->next;
  po_list_verify(list);    /* Debug mode only */
  node->next->prev = list;
  list->next = node->next;
  po_list_reset(node);     /* Debug mode only */
  return node;
}

/*-GLOBAL-
 * Pop object from tail of list.
 * The node is removed from tail of list and returned.
 */
static inline po_list_Node *po_list_poptail(po_list_List *list)
{
  po_list_Node *node = list->prev;
  po_list_verify(list);    /* Debug mode only */
  node->prev->next = list;
  list->prev = node->prev;
  po_list_reset(node);     /* Debug mode only */
  return node;
}

/*-GLOBAL-
 * Pop object from list.
 * The node is removed from list.
 */
static inline void po_list_pop(po_list_Node *node)
{
  po_list_verify(node);    /* Debug mode only */
  node->prev->next = node->next;
  node->next->prev = node->prev;
  po_list_reset(node);     /* Debug mode only */
}

/*-GLOBAL-
 * Return a pointer to head of list.
 * No nodes are removed from list.
 */
static inline po_list_Node *po_list_head(po_list_List *list)
{
  po_list_verify(list);
  return list->next;
}

/*-GLOBAL-
 * Return a pointer to tail of list.
 * No nodes are removed from list.
 */
static inline po_list_Node *po_list_tail(po_list_Node *list)
{
  po_list_verify(list);
  return list->prev;
}

/*-GLOBAL-
 * Return a pointer to next node in list, the one that comes after ref.
 * No nodes are removed from list.
 */
static inline po_list_Node *po_list_next(po_list_Node *ref)
{
  po_list_verify(ref);
  return ref->next;
}

/*-GLOBAL-
 * Return a pointer to previous node in list, the one that comes before ref.
 * No nodes are removed from list.
 */
static inline po_list_Node *po_list_prev(po_list_Node *ref)
{
  po_list_verify(ref);
  return ref->prev;
}

/*-GLOBAL-
 * Returns non-zero if the list is empty (it contains only the dummy node).
 */
static inline int po_list_isempty(po_list_List *list)
{
  po_list_verify(list);
  return list->next == list;
}

/*-GLOBAL-
 * Returns non-zero if the list contains a unique object (in addition to the
 * dummy node).
 */
static inline int po_list_iscount1(po_list_Node *node)
{
  po_list_verify(node);
  return node->next->next == node;
}

/*-GLOBAL-
 * Used by loops: we start from one side and loop until we reach the dummy
 * node. This function returns non-zero when we're done. Use po_list_next,
 * for exampe, to loop.
 */
static inline int po_list_isdone(po_list_List *list, po_list_Node *node)
{
  return list == node;
}

/*-GLOBAL-
 * Returns the object in which is embedded the po_list_Node.
 */
#define po_list_getobj(node, struct_name, field) \
  ((struct_name*)((char*)node - offsetof(struct_name, field)))

#endif // po_list__H
