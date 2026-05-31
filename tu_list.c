/*
File:       tu_list.c
Purpose:    doubly-linked list
Author:     Seree R.
Date:       27-APR-2026
*/
#include <stdlib.h>
#include "tu_list.h"

struct tu_listnode
{
    struct tu_listnode *link[2]; /* link[0]=prev, link[1]=next */
    void               *data;
};
typedef struct tu_listnode tu_listnode_t;

struct tu_linklist
{
    struct tu_listnode *head;
    struct tu_listnode *tail;
    cmp_f               cmp; /* compare two items   */
    dup_f               dup; /* clone an item        */
    rel_f               rel; /* destroy an item      */
    size_t              size;
};

/* ---------- internal helpers ---------- */

static struct tu_listnode *tu_list__newnode(tu_linklist_t *list, void *data, int clone)
{
    struct tu_listnode *newnode =
        (struct tu_listnode *)calloc(1, sizeof(struct tu_listnode));
    if (!newnode)
        return NULL;
    if (clone)
    {
        if (!list->dup)  /* fail fast: caller requested clone but no dup callback */
        {
            free(newnode);
            return NULL;
        }
        newnode->data = list->dup(data);
    }
    else
    {
        newnode->data = data;
    }
    return newnode;
}

/* Unlink and free a node; release its data when clone != 0 and rel is set. */
static void tu_list__freenode(tu_linklist_t *list, struct tu_listnode *node, int clone)
{
    if (clone && list->rel)
        list->rel(node->data);
    free(node);
}

/* ---------- init / new / release / delete ---------- */

int tu_list_init(tu_linklist_t *list, cmp_f cmp, dup_f dup, rel_f rel)
{
    list->head = list->tail = NULL;
    list->cmp  = cmp;
    list->dup  = dup;
    list->rel  = rel;
    list->size = 0;
    return 0;
}

void tu_list_release(tu_linklist_t *list, int clone)
{
    struct tu_listnode *iter = list->head;
    while (iter)
    {
        struct tu_listnode *next = iter->link[1];
        tu_list__freenode(list, iter, clone);
        iter = next;
    }
    list->head = list->tail = NULL;
    list->size = 0;
}

tu_linklist_t *tu_list_new(cmp_f cmp, dup_f dup, rel_f rel)
{
    struct tu_linklist *list =
        (struct tu_linklist *)calloc(1, sizeof(struct tu_linklist));
    if (!list)
        return NULL;
    tu_list_init(list, cmp, dup, rel);
    return list;
}

void tu_list_delete(tu_linklist_t *list)
{
    tu_list_release(list, 0);
    free(list);
}

/* ---------- insert ---------- */

int tu_list_pushfront(tu_linklist_t *list, void *data, int clone)
{
    struct tu_listnode *newnode = tu_list__newnode(list, data, clone);
    if (!newnode)
        return -1;
    if (list->head)
    {
        newnode->link[1]    = list->head;
        list->head->link[0] = newnode;
    }
    else
    {
        list->tail = newnode;
    }
    list->head = newnode;
    list->size++;
    return 0;
}

int tu_list_pushback(tu_linklist_t *list, void *data, int clone)
{
    struct tu_listnode *newnode = tu_list__newnode(list, data, clone);
    if (!newnode)
        return -1;
    if (list->tail)
    {
        newnode->link[0]    = list->tail;
        list->tail->link[1] = newnode;
    }
    else
    {
        list->head = newnode;
    }
    list->tail = newnode;
    list->size++;
    return 0;
}

int tu_list_insert_after(tu_linklist_t *list, tu_listnode_t *node, void *data, int clone)
{
    if (!node)
        return -1;
    if (node == list->tail)
        return tu_list_pushback(list, data, clone);

    struct tu_listnode *newnode = tu_list__newnode(list, data, clone);
    if (!newnode)
        return -1;

    struct tu_listnode *next = node->link[1];
    newnode->link[0] = node;
    newnode->link[1] = next;
    node->link[1]    = newnode;
    if (next)
        next->link[0] = newnode;
    list->size++;
    return 0;
}

int tu_list_insert_before(tu_linklist_t *list, tu_listnode_t *node, void *data, int clone)
{
    if (!node)
        return -1;
    if (node == list->head)
        return tu_list_pushfront(list, data, clone);

    struct tu_listnode *newnode = tu_list__newnode(list, data, clone);
    if (!newnode)
        return -1;

    struct tu_listnode *prev = node->link[0];
    newnode->link[0] = prev;
    newnode->link[1] = node;
    node->link[0]    = newnode;
    if (prev)
        prev->link[1] = newnode;
    list->size++;
    return 0;
}

/* ---------- remove ---------- */

void tu_list_popfront(tu_linklist_t *list, int clone)
{
    if (!list->head)
        return;
    struct tu_listnode *delnode = list->head;
    list->head = delnode->link[1];
    if (list->head)
        list->head->link[0] = NULL;
    else
        list->tail = NULL;
    tu_list__freenode(list, delnode, clone);
    list->size--;
}

void tu_list_popback(tu_linklist_t *list, int clone)
{
    if (!list->tail)
        return;
    struct tu_listnode *delnode = list->tail;
    list->tail = delnode->link[0];
    if (list->tail)
        list->tail->link[1] = NULL;
    else
        list->head = NULL;
    tu_list__freenode(list, delnode, clone);
    list->size--;
}

int tu_list_remove(tu_linklist_t *list, tu_listnode_t *node, int clone)
{
    if (!node)
        return -1;
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
        struct tu_listnode *prev = node->link[0];
        struct tu_listnode *next = node->link[1];
        prev->link[1] = next;
        next->link[0] = prev;
        tu_list__freenode(list, node, clone);
        list->size--;
    }
    return 0;
}

/* Returns 0 if found and removed, -1 if not found. */
int tu_list_erase(tu_linklist_t *list, void *data, int clone)
{
    if (!list->cmp)
        return -1;
    struct tu_listnode *iter = list->head;
    while (iter)
    {
        if (list->cmp(iter->data, data) == 0)
        {
            tu_list_remove(list, iter, clone);
            return 0;
        }
        iter = iter->link[1];
    }
    return -1;
}

/* ---------- query ---------- */

size_t tu_list_size(tu_linklist_t *list)
{
    return list->size;
}

tu_listnode_t *tu_list_at(tu_linklist_t *list, size_t index)
{
    if (index >= list->size)
        return NULL;
    struct tu_listnode *node = list->head;
    for (size_t i = 0; i < index && node; i++)
        node = node->link[1];
    return node;
}

/* ---------- navigation ---------- */

tu_listnode_t *tu_list_first(tu_linklist_t *list) { return list->head; }
tu_listnode_t *tu_list_last (tu_linklist_t *list) { return list->tail; }

tu_listnode_t *tu_list_next(tu_listnode_t *node)
{
    return node ? node->link[1] : NULL;
}
tu_listnode_t *tu_list_prev(tu_listnode_t *node)
{
    return node ? node->link[0] : NULL;
}
void *tu_list_data(tu_listnode_t *node)
{
    return node ? node->data : NULL;
}

/* ---------- search ---------- */

tu_listnode_t *tu_list_find(tu_linklist_t *list, void *data)
{
    if (!list->cmp)
        return NULL;
    tu_listnode_t *nodep = list->head;
    while (nodep)
    {
        if (list->cmp(nodep->data, data) == 0)
            return nodep;
        nodep = nodep->link[1];
    }
    return NULL;
}

/* Search forward starting *after* nodep. */
tu_listnode_t *tu_list_findnext(tu_linklist_t *list, tu_listnode_t *nodep, void *data)
{
    if (!list->cmp || !nodep)
        return NULL;
    nodep = nodep->link[1];
    while (nodep)
    {
        if (list->cmp(nodep->data, data) == 0)
            return nodep;
        nodep = nodep->link[1];
    }
    return NULL;
}

tu_listnode_t *tu_list_rfind(tu_linklist_t *list, void *data)
{
    if (!list->cmp)
        return NULL;
    tu_listnode_t *nodep = list->tail;
    while (nodep)
    {
        if (list->cmp(nodep->data, data) == 0)
            return nodep;
        nodep = nodep->link[0];
    }
    return NULL;
}

/* Search backward starting *before* nodep. */
tu_listnode_t *tu_list_rfindprev(tu_linklist_t *list, tu_listnode_t *nodep, void *data)
{
    if (!list->cmp || !nodep)
        return NULL;
    nodep = nodep->link[0];
    while (nodep)
    {
        if (list->cmp(nodep->data, data) == 0)
            return nodep;
        nodep = nodep->link[0];
    }
    return NULL;
}

/* ---------- iteration ---------- */

void tu_list_foreach(tu_linklist_t *list, tu_list_visitor_f visitor, void *userdata)
{
    tu_listnode_t *nodep = list->head;
    while (nodep)
    {
        /* Snapshot next before visitor runs — safe to remove current node inside visitor */
        tu_listnode_t *nextp = nodep->link[1];
        if (visitor(nodep->data, userdata) != 0)
            break;
        nodep = nextp;
    }
}
