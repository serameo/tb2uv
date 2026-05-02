/*
File:       tu_list.h
Purpose:    create a linked list
Author:     Seree R.
Date:       27-APR-2026
*/
#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__

#ifdef __cplusplus
extern "C" {
#endif

/* User-defined item handling */
typedef int   (*cmp_f) ( const void *p1, const void *p2 );
typedef void *(*dup_f) ( void *p );
typedef void  (*rel_f) ( void *p );

typedef struct tu_linklist  tu_linklist_t;
typedef struct tu_listnode  tu_listnode_t;

tu_linklist_t*  tu_list_new ( cmp_f cmp, dup_f dup, rel_f rel );
void            tu_list_delete ( tu_linklist_t *list );

int             tu_list_init ( tu_linklist_t* list, cmp_f cmp, dup_f dup, rel_f rel );
void            tu_list_release ( tu_linklist_t *list, int clone );

int             tu_list_pushfront ( tu_linklist_t *list, void *data, int clone );
int             tu_list_pushback ( tu_linklist_t *list, void *data, int clone );
void            tu_list_popfront ( tu_linklist_t *list, int clone );
void            tu_list_popback ( tu_linklist_t *list, int clone );
int             tu_list_erase ( tu_linklist_t *list, void *data, int clone );
size_t          tu_list_size ( tu_linklist_t *list );

tu_listnode_t*  tu_list_first(tu_linklist_t *list);
tu_listnode_t*  tu_list_last(tu_linklist_t *list);
tu_listnode_t*  tu_list_next(tu_listnode_t *node);
tu_listnode_t*  tu_list_prev(tu_listnode_t *node);
void*           tu_list_data(tu_listnode_t *node);

/*Traversal*/
typedef struct tu_listtrav  tu_listtrav_t;
tu_listtrav_t*  tu_listtrav_new();
void            tu_listtrav_delete(tu_listtrav_t* trav);
void            tu_listtrav_clone(tu_listtrav_t* dest, tu_listtrav_t* src);

void            *tu_listrav_first( tu_listtrav_t *trav, tu_linklist_t *list );
void            *tu_listrav_last ( tu_listtrav_t *trav, tu_linklist_t *list );
void            *tu_listrav_next ( tu_listtrav_t *trav );
void            *tu_listrav_prev ( tu_listtrav_t *trav );

#ifdef __cplusplus
}
#endif


#endif /*__LINKEDLIST_H__*/
