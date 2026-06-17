#pragma once

#include <stddef.h>
#include <stdint.h>

// intrusive linked-list node
struct HNode
{
    HNode *next = NULL;
    uint64_t hcode = 0; // hash value
};

// hashtable (separate chaining)
struct HTab
{
    HNode **tab = NULL; // array of linked-list of HNode's
    size_t mask = 0;    // array_size = 2^n -> mask = 2^n - 1
    size_t size = 0;    // number of keys

    // use power of 2 for array size
    // -> map hash code to array index faster:
    //    . idx = hcode % 2^n = hcode & (2^n - 1)
};

// hashtable interface
// (use 2 hashtables for progressive rehashing)
struct HMap
{
    HTab newer;
    HTab older;
    size_t migrate_pos = 0; // current slot in 'older' to migrate
};

// function to decide data equality
typedef bool HEqFn(HNode *, HNode *);

void hm_insert(HMap *hmap, HNode *node);
HNode *hm_lookup(HMap *hmap, HNode *key, HEqFn eq);
HNode *hm_delete(HMap *hmap, HNode *key, HEqFn eq);
size_t hm_size(HMap *hmap);
void hm_clear(HMap *hmap);

typedef bool ForEachCb(HNode *, void *);
void hm_foreach(HMap *hmap, ForEachCb cb, void *args);