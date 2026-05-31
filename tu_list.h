/*
File:       tu_list.h
Purpose:    doubly-linked list
Author:     Seree R.
Date:       27-APR-2026
*/
#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* User-defined item handling callbacks */
typedef int   (*cmp_f) ( const void *p1, const void *p2 );
typedef void *(*dup_f) ( void *p );
typedef void  (*rel_f) ( void *p );

/*
 * Visitor callback for tu_list_foreach.
 * Return 0 to continue iteration, non-zero to stop early.
 * It is safe to remove the *current* node inside the visitor.
 * Do not remove other nodes during iteration.
 */
typedef int (*tu_list_visitor_f)(void *data, void *userdata);

typedef struct tu_linklist  tu_linklist_t;
typedef struct tu_listnode  tu_listnode_t;

/* Heap-allocated list — use tu_list_new/tu_list_delete */
tu_linklist_t*  tu_list_new    ( cmp_f cmp, dup_f dup, rel_f rel );
void            tu_list_delete ( tu_linklist_t *list );

/* Stack/embedded list — use tu_list_init/tu_list_release */
int             tu_list_init    ( tu_linklist_t *list, cmp_f cmp, dup_f dup, rel_f rel );
void            tu_list_release ( tu_linklist_t *list, int clone );

/* Insert */
int  tu_list_pushfront      ( tu_linklist_t *list, void *data, int clone );
int  tu_list_pushback       ( tu_linklist_t *list, void *data, int clone );
int  tu_list_insert_before  ( tu_linklist_t *list, tu_listnode_t *node, void *data, int clone );
int  tu_list_insert_after   ( tu_linklist_t *list, tu_listnode_t *node, void *data, int clone );

/* Remove */
void tu_list_popfront ( tu_linklist_t *list, int clone );
void tu_list_popback  ( tu_linklist_t *list, int clone );
int  tu_list_remove   ( tu_linklist_t *list, tu_listnode_t *node, int clone );
int  tu_list_erase    ( tu_linklist_t *list, void *data, int clone );

/* Query */
size_t          tu_list_size  ( tu_linklist_t *list );
tu_listnode_t*  tu_list_at    ( tu_linklist_t *list, size_t index );

/* Navigation */
tu_listnode_t*  tu_list_first ( tu_linklist_t *list );
tu_listnode_t*  tu_list_last  ( tu_linklist_t *list );
tu_listnode_t*  tu_list_next  ( tu_listnode_t *node );
tu_listnode_t*  tu_list_prev  ( tu_listnode_t *node );
void*           tu_list_data  ( tu_listnode_t *node );

/* Search — require cmp != NULL; return NULL if not found */
tu_listnode_t*  tu_list_find      ( tu_linklist_t *list, void *data );
tu_listnode_t*  tu_list_findnext  ( tu_linklist_t *list, tu_listnode_t *nodep, void *data );
tu_listnode_t*  tu_list_rfind     ( tu_linklist_t *list, void *data );
tu_listnode_t*  tu_list_rfindprev ( tu_linklist_t *list, tu_listnode_t *nodep, void *data );

/* Iteration — safe to remove the current node inside visitor */
void tu_list_foreach ( tu_linklist_t *list, tu_list_visitor_f visitor, void *userdata );

#ifdef __cplusplus
}
#endif

#endif /*__LINKEDLIST_H__*/
