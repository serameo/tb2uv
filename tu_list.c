/*
File:       tu_list.c
Purpose:    create a linked list
Author:     Seree R.
Date:       27-APR-2026
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tu_list.h"

struct tu_listnode
{
    struct tu_listnode*     link[2]; /*link[0] = prev, link[1] = next*/
    void*                   data;
};
typedef struct tu_listnode tu_listnode_t;


struct tu_linklist
{
    struct tu_listnode*     head;
    struct tu_listnode*     tail;
    cmp_f                   cmp;  /* Compare two items */
    dup_f                   dup;  /* Clone an item (user-defined) */
    rel_f                   rel;  /* Destroy an item (user-defined) */
    size_t                  size;
};

static struct tu_listnode* tu_list__newnode(tu_linklist_t* list, void* data, int clone)
{
    struct tu_listnode* newnode = (struct tu_listnode*)calloc(1, sizeof(struct tu_listnode));
    if (newnode)
    {
        if (clone)
        {
            newnode->data = list->dup(data);
        }
        else
        {
            newnode->data = data;
        }
        newnode->link[0] = newnode->link[1] = NULL;
    }
    return newnode;
}

int tu_list_init (tu_linklist_t* list, cmp_f cmp, dup_f dup, rel_f rel )
{
    list->head = list->tail = NULL;
    list->cmp = cmp;
    list->dup = dup;
    list->rel = rel;
    list->size = 0;
}

void tu_list_release ( tu_linklist_t *list, int clone )
{
    struct tu_listnode* iter = list->head;
    struct tu_listnode* delnode = NULL;
    while (iter)
    {
        delnode = iter;
        iter = iter->link[1];
        if (clone)
        {
            list->rel(delnode->data);
        }
        free(delnode);
    }
    list->head = list->tail = NULL;
    list->size = 0;
}

tu_linklist_t *tu_list_new ( cmp_f cmp, dup_f dup, rel_f rel )
{
    struct tu_linklist* list = (struct tu_linklist*)calloc(1, sizeof(struct tu_linklist));
    if (NULL == list)
    {
        return NULL;
    }
    tu_list_init(list, cmp, dup, rel);
    return list;
}

void tu_list_delete ( tu_linklist_t *list )
{
    tu_list_release(list, 0);
    free(list);
}

int tu_list_pushfront ( tu_linklist_t *list, void *data, int clone )
{
    struct tu_listnode* newnode = tu_list__newnode(list, data, clone);
    if (NULL == newnode)
    {
        return -1; /*not enough memory*/
    }
    if (list->head)
    {
        newnode->link[1] = list->head;
        list->head->link[0] = newnode;
    }
    else
    {
        list->tail = newnode;
    }
    list->head = newnode;
    list->size += 1;
    return 0;
}

int tu_list_pushback ( tu_linklist_t *list, void *data, int clone )
{
    struct tu_listnode* newnode = tu_list__newnode(list, data, clone);
    if (NULL == newnode)
    {
        return -1; /*not enough memory*/
    }
    if (list->tail)
    {
        newnode->link[0] = list->tail;
        list->tail->link[1] = newnode;
    }
    else
    {
        list->head = newnode;
    }
    list->tail = newnode;
    list->size += 1;
    return 0;
}

void tu_list_popfront ( tu_linklist_t *list, int clone )
{
    if (NULL == list->head)
    {
        return;
    }
    if (list->head != list->tail)
    {
        struct tu_listnode* delnode = list->head;
        list->head = list->head->link[1];
        list->head->link[0] = NULL;
        if (clone)
        {
            list->rel(delnode->data);
        }
        free(delnode);
    }
    else
    {
        if (clone)
        {
            list->rel(list->head->data);
        }
        free(list->head);
        list->head = list->tail = NULL;
    }
    list->size -= 1;
}

void tu_list_popback ( tu_linklist_t *list, int clone )
{
    if (NULL == list->tail)
    {
        return;
    }
    if (list->head != list->tail)
    {
        struct tu_listnode* delnode = list->tail;
        list->tail = list->tail->link[0];
        list->tail->link[1] = NULL;
        if (clone)
        {
            list->rel(delnode->data);
        }
        free(delnode);
    }
    else
    {
        if (clone)
        {
            list->rel(list->tail->data);
        }
        free(list->tail);
        list->head = list->tail = NULL;
    }
    list->size -= 1;
}

int tu_list_remove( tu_linklist_t *list, tu_listnode_t* node, int clone)
{
    if (node == list->head)
    {
        tu_list_popfront(list, clone);
    }
    else if (node == list->tail)
    {
        tu_list_popback(list, clone);
    }
    else
    {
        struct tu_listnode* prev = node->link[0];
        struct tu_listnode* next = node->link[1];
        
        prev->link[1] = next;
        next->link[0] = prev;
        node->link[0] = node->link[1] = NULL;
        if (clone)
        {
            list->rel(node->data);
        }
        free(node);
        list->size -= 1;
    }
    return 0;
}

int tu_list_erase ( tu_linklist_t *list, void *data, int clone )
{
    struct tu_listnode* iter = list->head;
    while (iter)
    {
        if (list->cmp(iter->data, data) == 0)
        {
            tu_list_remove(list, iter, clone);
            break;
        }
        iter = iter->link[1];
    }
    return 1;
}

size_t tu_list_size ( tu_linklist_t *list )
{
    return list->size;
}


tu_listnode_t*  tu_list_first(tu_linklist_t *list)
{
    return (list->head);
}
tu_listnode_t*  tu_list_last(tu_linklist_t *list)
{
    return (list->tail);
}
tu_listnode_t*  tu_list_next(tu_listnode_t *node)
{
    return (node ? node->link[1] : NULL);
}
tu_listnode_t*  tu_list_prev(tu_listnode_t *node)
{
    return (node ? node->link[0] : NULL);
}
void* tu_list_data(tu_listnode_t *node)
{
    return (node ? node->data : NULL);
}


struct tu_listtrav
{
    struct tu_listnode*    iter;
};

tu_listtrav_t*  tu_listtrav_new()
{
    struct tu_listtrav* trav = (struct tu_listtrav*)calloc(1, sizeof(struct tu_listtrav));
    return trav;
}

void tu_listtrav_delete(tu_listtrav_t* trav)
{
    free(trav);
}

void            tu_listtrav_clone(tu_listtrav_t* dest, tu_listtrav_t* src)
{
    dest->iter = src->iter;
}

void *tu_listrav_first( tu_listtrav_t *trav, tu_linklist_t *list )
{
    trav->iter = list->head;
    return (trav->iter ? trav->iter->data : NULL);
}

void *tu_listrav_last ( tu_listtrav_t *trav, tu_linklist_t *list )
{
    trav->iter = list->tail;
    return (trav->iter ? trav->iter->data : NULL);
}

void *tu_listrav_next ( tu_listtrav_t *trav )
{
    if (trav && trav->iter)
    {
        trav->iter = trav->iter->link[1];
        return (trav->iter ? trav->iter->data : NULL);
    }
    return NULL;
}

void *tu_listrav_prev ( tu_listtrav_t *trav )
{
    if (trav && trav->iter)
    {
        trav->iter = trav->iter->link[0];
        return (trav->iter ? trav->iter->data : NULL);
    }
    return NULL;
}

