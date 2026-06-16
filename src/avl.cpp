#include <assert.h>

#include "avl.hpp"

void avl_init(AVLNode *node)
{
    node->left = node->right = node->parent = NULL;
    node->height = 1;
    node->cnt = 1;
}

/* return precomputed height of subtree (0 for empty tree) */
uint32_t avl_height(AVLNode *node)
{
    return node ? node->height : 0;
}

/* return precomputed size of subtree (0 for empty tree) */
uint32_t avl_cnt(AVLNode *node)
{
    return node ? node->cnt : 0;
}

static uint32_t max(uint32_t lhs, uint32_t rhs)
{
    return lhs < rhs ? rhs : lhs;
}

/* update auxiliary data (propagate from children). */
static void avl_update(AVLNode *node)
{
    node->height = 1 + max(avl_height(node->left), avl_height(node->right));
    node->cnt = 1 + avl_cnt(node->left) + avl_cnt(node->right);
}

/* Rotations: keep binary search tree invariant
    B                                      D
 ___|___         rotate left            ___|___
a       D       ------------->         B       e
     ___|___    <-------------      ___|___
    c       e    rotate right      a       c
*/

/* rotate left & return new subtree root. */
static AVLNode *rot_left(AVLNode *node)
{
    AVLNode *parent = node->parent;
    AVLNode *right = node->right;
    assert(right != NULL);
    AVLNode *right_left = right->left;

    node->right = right_left;
    if (right_left)
        right_left->parent = node;

    right->parent = parent;
    // caller updates parent link to new subtree root (if parent exists)

    right->left = node;
    node->parent = right;

    // update auxiliary data
    avl_update(node);
    avl_update(right);

    return right; // new subtree root
}

/* rotate right & return new subtree root. */
static AVLNode *rot_right(AVLNode *node)
{
    AVLNode *parent = node->parent;
    AVLNode *left = node->left;
    assert(left != NULL);
    AVLNode *left_right = left->right;

    node->left = left_right;
    if (left_right)
        left_right->parent = node;

    left->parent = parent;
    // caller updates parent link to new subtree root (if parent exists)

    left->right = node;
    node->parent = left;

    // update auxiliary data
    avl_update(node);
    avl_update(left);

    return left; // new subtree root
}

/* Fix imbalances after insert/delete:

(1) left child is too tall after insert:

(1.0) Before insert:
. A,B,C,D,E are all valid AVL trees.
. height(D.left) - height(D.right) = 1

            D(h+2)
         ___|___
    B(h+1)     E(h)
   ___|___
A(h)     C(h)

(1.1) insert cause A subtree's height to grow
. height(D.left) - height(D.right) = 2 -> need fix!
. height(D.left.left) > height(D.left.right)

              D(h+3)
           ___|___
      B(h+2)     E(h)
     ___|___
A(h+1)     C(h)

=> FIX: rotate_right(D)

            B(h+2)
           ___|___
      A(h+1)     D(h+1)
                ___|___
             C(h)     E(h)

(1.2) insert cause C subtree's height to grow
. height(D.left) - height(D.right) = 2 -> need fix!
. height(D.left.left) < height(D.left.right)

            D(h+3)
           ___|___
      B(h+2)     E(h)
   ___|___
A(h)     C(h+1)

=> TRY (doesn't work): rotate_right(D)

            B(h+3)
           ___|___
        A(h)     D(h+2)     <- still imbalance
                ___|___
           C(h+1)     E(h)

=> FIX: rotate_left(B) first to transformed to case (1.1)

                D(h+3)
               ___|___
          C(h+2)     E(h)
         ___|___
    B(h+1)     ...(h)
   ___|___
A(h)     ...(h)

=========================

(2) right child is too tall after insert:
- similar, just rotate in different direction.

(3 / 4) imbalances after delete:
- fix the same way as (1), (2).
*/

/* fix when left child is taller by 2; return new subtree root. */
static AVLNode *avl_fix_left(AVLNode *node)
{
    assert(avl_height(node->left) - avl_height(node->right) == 2);
    if (avl_height(node->left->left) < avl_height(node->left->right))
        node->left = rot_left(node->left);
    return rot_right(node);
}

/* fix when right child is taller by 2; return new subtree root. */
static AVLNode *avl_fix_right(AVLNode *node)
{
    assert(avl_height(node->right) - avl_height(node->left) == 2);
    if (avl_height(node->right->right) < avl_height(node->right->left))
        node->right = rot_right(node->right);
    return rot_left(node);
}

/* keep fixing imbalances until the whole tree is fixed
   (maintain invariant "for any subtree, heights of 2 children differ at most by 1");
   return the final global tree root. */
static AVLNode *avl_fix(AVLNode *node)
{
    while (true)
    {
        // incoming pointer to fixed subtree
        AVLNode **from = &node;
        AVLNode *parent = node->parent;
        if (parent)
            from = parent->left == node ? &parent->left : &parent->right;
        // else: from = &root (node is root)

        // update auxiliary data
        avl_update(node);

        // fix imbalances if needed
        uint32_t l = avl_height(node->left);
        uint32_t r = avl_height(node->right);
        if (l == r + 2)
            *from = avl_fix_left(node);
        else if (r == l + 2)
            *from = avl_fix_right(node);

        if (!parent) // reached global tree root -> stop
            return *from;

        // continue to parent (its height may have changed)
        node = parent;
    }
}

/* detach a node with at least 1 empty child;
   return global tree root after rebalance. */
static AVLNode *avl_del_easy(AVLNode *node)
{
    assert(!node->left || !node->right);

    AVLNode *child = node->left ? node->left : node->right;
    AVLNode *parent = node->parent;

    // attach child to grandparent
    if (child)
        child->parent = parent;
    if (!parent) // removing global root
        return child;
    AVLNode **from = parent->left == node ? &parent->left : &parent->right;
    *from = child;

    // rebalance the updated tree
    return avl_fix(parent);
}

/* detach a node; return global tree root after rebalance. */
AVLNode *avl_del(AVLNode *node)
{
    // at least 1 empty child
    if (!node->left || !node->right)
        return avl_del_easy(node);

    // find successor (leftmost node of right child)
    // (can also use predecessor (rightmost node of left child))
    AVLNode *successor = node->right;
    while (successor->left)
        successor = successor->left;

    // detach the successor
    AVLNode *root = avl_del_easy(successor);

    // replace node with successor
    *successor = *node; // shallow-copy all fields

    // attach children to successor
    if (successor->left)
        successor->left->parent = successor;
    if (successor->right)
        successor->right->parent = successor;

    // attach successor to parent OR update global root
    AVLNode **from = &root;
    AVLNode *parent = node->parent;
    if (parent)
        from = parent->left == node ? &parent->left : &parent->right;
    *from = successor;

    return root;
}

/* insert a node; return global tree root after rebalance;
   ignore if duplicate (decided by cmp_fn). */
AVLNode *avl_insert(AVLNode **root, AVLNode *node, AVLCmpFn cmp_fn)
{
    // find place to insert (maintain binary search tree invariant)
    AVLNode *parent = NULL; // insert under this node
    AVLNode **from = root;  // incoming pointer to insert place
    while (*from)
    {
        parent = *from;

        int8_t cmp_result = cmp_fn(node, parent);
        if (cmp_result == 0) // duplicate -> no-op
            return *root;

        from = cmp_result < 0 ? &parent->left : &parent->right;
    }

    // attach the new node
    *from = node;
    node->parent = parent;

    // rebalance the updated tree
    return avl_fix(node);
}