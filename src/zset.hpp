#pragma once

#include "avl.hpp"
#include "hashtable.hpp"

// sorted set
struct ZSet
{
    AVLNode *root; // index by (score, name)
    HMap hmap;     // index by name
};

// sorted-set node
struct ZNode
{
    // === intrusive data structure node ===

    AVLNode tree;
    HNode hmap;

    // === data ===
    double score = 0;
    size_t len = 0; // name's size
    char name[];    // flexible array member
    // flexible array member:
    // - allows the last member of a struct to be an array of unspecified size.
    // - in our case: used to embed the string 'name' into the node.
};

bool zset_insert(
    ZSet *zset, const char *name, size_t len, double score);
ZNode *zset_lookup(ZSet *zset, const char *name, size_t len);
void zset_delete(ZSet *zset, ZNode *node);
void zset_clear(ZSet *zset);
ZNode *zset_seek_ge(
    ZSet *zset, double score, const char *name, size_t len);
ZNode *znode_offset(ZNode *node, int64_t offset);
int64_t znode_rank(ZNode *node);