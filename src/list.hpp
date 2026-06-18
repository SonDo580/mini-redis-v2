#pragma once

// stdlib
#include <stddef.h>

// doubly-linked-list node
struct DList
{
    DList *prev = NULL;
    DList *next = NULL;
};

void dlist_init(DList *node);
bool dlist_empty(DList *node);
void dlist_detach(DList *node);
void dlist_insert_before(DList *target, DList *rookie);
