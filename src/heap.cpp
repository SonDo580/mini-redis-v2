/* This file implements a min heap:

Heap:
- implement a binary tree with an array
  (the array represents the level-order traversal of the tree).
- the tree must be complete
  (all levels except the last must be fully filled,
   and the last level is filled from left to right)
- a node at i has left child at 2i + 1 and right child at 2i + 2.
- a node at i has parent at (i - 1) // 2.
- all nodes from n // 2 to n - 1 are leaves.

Min Heap:
- parent.val <= child.val.
- min item is at index 0 (root).
*/

// stdlib
#include <assert.h>

#include "heap.hpp"

/* index of parent. */
static size_t idx_parent(size_t i)
{
  return (i - 1) / 2;
  // return (i + 1) / 2 - 1; // alternative
}

/* index of left child. */
size_t idx_left(size_t i)
{
  return i * 2 + 1;
}

/* index of right child. */
size_t idx_right(size_t i)
{
  return i * 2 + 2;
}

/* move an item up the tree to the correct position. */
static void sift_up(HeapItem *arr, size_t pos)
{
  HeapItem item = arr[pos];
  while (pos > 0 && arr[idx_parent(pos)].val > item.val)
  {
    arr[pos] = arr[idx_parent(pos)];
    *arr[pos].ref = pos;
    pos = idx_parent(pos);
  }
  arr[pos] = item;
  *arr[pos].ref = pos;
}

/* move an item down the tree to the correct position. */
static void sift_down(HeapItem *arr, size_t pos, size_t len)
{
  HeapItem item = arr[pos];
  while (true)
  {
    // find smallest value among current node and its children
    size_t l = idx_left(pos);
    size_t r = idx_right(pos);
    size_t min_pos = pos;
    uint64_t min_val = item.val;
    if (l < len && arr[l].val < min_val)
    {
      min_pos = l;
      min_val = arr[l].val;
    }
    if (r < len && arr[r].val < min_val)
    {
      min_pos = r;
      min_val = arr[r].val;
    }

    if (min_pos == pos) // found correct position
      break;

    // swap with the smaller kid
    arr[pos] = arr[min_pos];
    *arr[pos].ref = pos;
    pos = min_pos;
  }

  arr[pos] = item;
  *arr[pos].ref = pos;
}

/* move an item to the correct position. */
static void sift(HeapItem *arr, size_t pos, size_t len)
{
  if (pos > 0 && arr[idx_parent(pos)].val > arr[pos].val)
    sift_up(arr, pos);
  else
    sift_down(arr, pos, len);
}

/* remove a heap item. */
void heap_delete(Heap &heap, size_t pos)
{
  assert(pos < heap.size());

  // swap with last item and pop
  heap[pos] = heap.back();
  heap.pop_back();

  if (pos == heap.size()) // removed the last item itself
    return;
  // move the swapped item to the correct place
  sift(heap.data(), pos, heap.size());
}

/* update or insert a heap item. */
void heap_upsert(Heap &heap, size_t pos, HeapItem item)
{
  if (pos < heap.size())
  { // update existing item
    heap[pos] = item;
  }
  else
  { // add new item to the end
    assert(pos == (size_t)-1);
    pos = heap.size();
    heap.push_back(item);
  }

  // move the updated/inserted item to the correct place
  sift(heap.data(), pos, heap.size());
}

/* (Extra) Proofs:

1. a node at i has left child at 2i + 1 and right child at 2i + 2:
- let say i is at level k, its children are at level k + 1.
  -> level k and k - 1 must be full.
- if level k is full, it has 2^k nodes.
- let m be number of nodes before node i at level k;
      n be number of nodes before left child at level k + 1;
      lci be index of left child in array;
      rci be index of right child in array.

m = i - number_of_nodes_before_level(k)
  = i - (1 + 2 + ... + 2^(k-1))
n = 2m = 2*i - (2 + 2^2 + ... + 2^k)
lci = number_of_nodes_before_level(k+1) + n
    = (1 + 2 + ... + 2^k) + 2*i - (2 + 2^2 + ... + 2^k)
    = 2*i + 1
rci = lci + 1 = 2*i + 2

====================

2. a node at i has parent at p = (i - 1) // 2
- if i is left child:
  . i = 2*p + 1 (odd number)
    -> p = (i - 1) / 2
  . i is odd -> i - 1 is even -> (i - 1) / 2 is an integer
    -> p = (i - 1) / 2 = (i - 1) // 2

- if i is right child:
  . i = 2*p + 2 (even number)
    -> p = (i - 2) / 2 = i / 2 - 1
  . i is even -> i / 2 is an integer
  . (i - 1) / 2 = i / 2 - 1 / 2
    -> (i - 1) // 2 = i / 2 - 1
    -> p = (i - 1) // 2

Alternative: p = (i + 1) // 2 - 1
- if i is left child:
  . i is odd -> i + 1 is even -> (i + 1) // 2 = (i + 1) / 2
  . (i + 1) / 2 - 1 = (i - 1) / 2 = (i - 1) // 2
- if i is right child:
  . i is even -> (i + 1) // 2 = i / 2 + 1 // 2 = i / 2
  . (i + 1) // 2 - 1 = i / 2 - 1 = (i - 1) // 2
*/