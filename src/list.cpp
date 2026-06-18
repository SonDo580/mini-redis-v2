#include "list.hpp"

/* initialize list head (dummy node, not member). */
void dlist_init(DList *node)
{
    // form a circle (to avoid handling NULL)
    node->prev = node->next = node;
}

/* empty list only has list head (dummy node). */
bool dlist_empty(DList *node)
{
    return node->next == node;
}

void dlist_detach(DList *node)
{
    DList *prev = node->prev;
    DList *next = node->next;
    prev->next = next;
    next->prev = prev;
}

/* special case: insert at end of list <-> insert before list head
   (form a circle). */
void dlist_insert_before(DList *target, DList *rookie)
{
    DList *prev = target->prev;
    prev->next = rookie;
    rookie->prev = prev;
    rookie->next = target;
    target->prev = rookie;
}