// stdlib
#include <assert.h>
#include <string.h>

#include "server_common.hpp"
#include "zset.hpp"

static ZNode *znode_new(const char *name, size_t len, double score)
{
    ZNode *node = (ZNode *)malloc(sizeof(ZNode) + len);
    if (!node)
        return NULL;

    avl_init(&node->tree);
    node->hmap.next = NULL;
    node->hmap.hcode = str_hash((uint8_t *)name, len);
    node->score = score;
    node->len = len;
    memcpy(&node->name[0], name, len);
    return node;
}

static void znode_del(ZNode *node)
{
    free(node);
}

static size_t min(size_t lhs, size_t rhs)
{
    return lhs < rhs ? lhs : rhs;
}

static int8_t avl_cmp(
    AVLNode *lhs, double score, const char *name, size_t len)
{
    ZNode *zl = container_of(lhs, ZNode, tree);

    // compare scores
    if (zl->score < score)
        return -1;
    if (zl->score > score)
        return 1;

    // compare names lexicographically
    int rv = memcmp(zl->name, name, min(zl->len, len));
    if (rv < 0)
        return -1;
    if (rv > 0)
        return 1;

    // compare lengths if name prefixes are equal
    if (zl->len < len)
        return -1;
    if (zl->len > len)
        return 1;
    return 0;
}

/* implement AVLCmpFn for ZNode: compare (score, name). */
static int8_t avl_cmp(AVLNode *lhs, AVLNode *rhs)
{
    ZNode *zr = container_of(rhs, ZNode, tree);
    return avl_cmp(lhs, zr->score, zr->name, zr->len);
}

/* update score for existing name; skip if score is the same. */
static void zset_update(ZSet *zset, ZNode *node, double score)
{
    if (node->score == score)
        return;

    // detach, mutate, and reinsert the tree node
    zset->root = avl_del(&node->tree);
    avl_init(&node->tree);
    node->score = score;
    avl_insert(&zset->root, &node->tree, avl_cmp);
}

/* add a new (score, name) tuple OR update score for existing name;
   return true if add, false if update. */
bool zset_insert(
    ZSet *zset, const char *name, size_t len, double score)
{
    ZNode *node = zset_lookup(zset, name, len);
    if (node)
    {
        zset_update(zset, node, score);
        return false;
    }
    else
    {
        node = znode_new(name, len, score);
        hm_insert(&zset->hmap, &node->hmap);
        avl_insert(&zset->root, &node->tree, avl_cmp);
        return true;
    }
}

// helper struct for hashtable lookup
struct HKey
{
    HNode node;
    const char *name = NULL;
    size_t len = 0;
};

/* implement HEqFn for 'ZNode' and 'HKey': compare names. */
static bool h_eq(HNode *node, HNode *key)
{
    ZNode *znode = container_of(node, ZNode, hmap);
    HKey *hkey = container_of(key, HKey, node);
    if (znode->len != hkey->len)
        return false;
    return memcmp(znode->name, hkey->name, znode->len) == 0;
}

/* lookup a node by name. */
ZNode *zset_lookup(ZSet *zset, const char *name, size_t len)
{
    if (!zset->root)
        return NULL;

    HKey key;
    key.node.hcode = str_hash((uint8_t *)name, len);
    key.name = name;
    key.len = len;

    HNode *found = hm_lookup(&zset->hmap, &key.node, h_eq);
    return found ? container_of(found, ZNode, hmap) : NULL;
}

/* delete a node. */
void zset_delete(ZSet *zset, ZNode *node)
{
    // remove from hashtable
    HKey key;
    key.node.hcode = node->hmap.hcode;
    key.name = node->name;
    key.len = node->len;
    HNode *found = hm_delete(&zset->hmap, &key.node, h_eq);
    assert(found != NULL);

    // remove from the tree
    zset->root = avl_del(&node->tree);

    // deallocate the node
    znode_del(node);
}

/* seek to the 1st pair >= (score, name). */
ZNode *zset_seek_ge(
    ZSet *zset, double score, const char *name, size_t len)
{
    AVLNode *found = NULL;
    for (AVLNode *node = zset->root; node;)
    {
        if (avl_cmp(node, score, name, len) < 0)
            node = node->right;
        else
        {
            found = node; // candidate
            node = node->left;
        }
    }
    return found ? container_of(found, ZNode, tree) : NULL;
}

/* return the successor/predecessor at offset from node. */
ZNode *znode_offset(ZNode *node, int64_t offset)
{
    AVLNode *target = node ? avl_offset(&node->tree, offset) : NULL;
    return target ? container_of(target, ZNode, tree) : NULL;
}

static void tree_dispose(AVLNode *node)
{
    if (!node)
        return;

    // must deallocate parent after children
    tree_dispose(node->left);
    tree_dispose(node->right);
    znode_del(container_of(node, ZNode, tree));
}

void zset_clear(ZSet *zset)
{
    hm_clear(&zset->hmap);
    tree_dispose(zset->root);
    zset->root = NULL;
}
