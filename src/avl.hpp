#pragma once

#include <stddef.h>
#include <stdint.h>

// intrusive AVL tree node
struct AVLNode
{
    AVLNode *parent = NULL;
    AVLNode *left = NULL;
    AVLNode *right = NULL;
    uint32_t height = 0; // subtree height (number of nodes in longest path from node to a leaf)
    uint32_t cnt = 0;    // subtree size (number of nodes in subtree)
};

// return -1 if lhs < rhs, 1 if lhs > rhs, 0 if lhs == rhs
typedef int8_t AVLCmpFn(AVLNode *, AVLNode *);

void avl_init(AVLNode *node);
uint32_t avl_height(AVLNode *node);
uint32_t avl_cnt(AVLNode *node);
AVLNode *avl_del(AVLNode *node);
AVLNode *avl_insert(AVLNode **root, AVLNode *node, AVLCmpFn cmp_fn);
AVLNode *avl_offset(AVLNode *node, int64_t offset);
