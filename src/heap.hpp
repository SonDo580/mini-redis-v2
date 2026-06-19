#pragma once

// system
#include <stddef.h>
#include <stdint.h>
// C++
#include <vector>

struct HeapItem
{
    uint64_t val = 0;   // (Entry's expires_at ms)
    size_t *ref = NULL; // pointer to embedded array index (Entry::heap_idx)
};

typedef std::vector<HeapItem> Heap;

void heap_delete(Heap &heap, size_t pos);
void heap_upsert(Heap &heap, size_t pos, HeapItem item);

// === exposed for testing ===

size_t idx_left(size_t i);
size_t idx_right(size_t i);
