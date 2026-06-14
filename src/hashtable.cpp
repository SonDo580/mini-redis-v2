#include <assert.h>
#include <stdlib.h>

#include "hashtable.hpp"

static void htab_init(HTab *htab, size_t n)
{
    assert(n > 0 && (n & (n - 1)) == 0); // n must be power of 2
    htab->tab = (HNode **)calloc(n, sizeof(HNode *));
    htab->mask = n - 1;
    htab->size = 0;
}

/* decide bucket based on hash code */
static inline size_t htab_idx(HTab *htab, HNode *node)
{
    // (see note in HTab struct)
    return node->hcode & htab->mask;
}

/* insert at front of chain */
static void htab_insert(HTab *htab, HNode *node)
{
    size_t pos = htab_idx(htab, node);
    HNode *next = htab->tab[pos];
    node->next = next;
    htab->tab[pos] = node;
    htab->size++;
}

/* return incoming pointer to target HNode*. */
static HNode **htab_lookup(HTab *htab, HNode *key, DataEqFn eq)
{
    if (!htab->tab)
        return NULL;

    size_t pos = htab_idx(htab, key);
    HNode **from = &htab->tab[pos];
    HNode *curr;
    while ((curr = *from) != NULL)
    {
        if (curr->hcode == key->hcode && eq(curr, key))
            return from;
        from = &curr->next;
    }

    return NULL; // target not found
}

/* remove target HNode* from chain, given incoming pointer to it;
   return the removed HNode* */
static HNode *htab_detach(HTab *htab, HNode **from)
{
    HNode *target = *from;
    *from = target->next;
    htab->size--;
    return target;
}

// - load_factor = num_keys / num_slots
// - for separate chaining, load_factor > 1
//   since each slot is intended to hold multiple items
const size_t K_MAX_LOAD_FACTOR = 8;

/* trigger when load factor is too high
   - (older, newer) -> (newer, new_table) */
static void hm_trigger_rehash(HMap *hmap)
{
    assert(hmap->older.tab == NULL);
    hmap->older = hmap->newer;
    htab_init(&hmap->newer, (hmap->newer.mask + 1) * 2); // double size
    hmap->migrate_pos = 0;
}

// (constant work) max number of keys to migrate each time
const size_t K_REHASH_WORK = 128;

/* migrate some keys from 'older' to 'newer' hashtable. */
static void hm_progressive_rehash(HMap *hmap)
{
    size_t count = 0; // number of keys migrated in this batch
    while (count < K_REHASH_WORK && hmap->older.size > 0)
    {
        HNode **from = &hmap->older.tab[hmap->migrate_pos];
        if (!*from)
        { // empty slot
            hmap->migrate_pos++;
            continue;
        }

        // move head of chain to 'newer'
        htab_insert(&hmap->newer, htab_detach(&hmap->older, from));
        count++;
    }

    // discard 'older' if fully migrated
    if (hmap->older.size == 0 && hmap->older.tab)
    {
        free(hmap->older.tab);
        hmap->older = HTab{};
    }
}

void hm_insert(HMap *hmap, HNode *node)
{
    // init 'newer' if empty
    if (!hmap->newer.tab)
        htab_init(&hmap->newer, 4);

    // always insert to 'newer'
    htab_insert(&hmap->newer, node);

    // check if need to rehash
    if (!hmap->older.tab) // 'older' has been fully migrated
    {
        size_t threshold = (hmap->newer.mask + 1) * K_MAX_LOAD_FACTOR;
        if (hmap->newer.size >= threshold)
            hm_trigger_rehash(hmap);
    }

    // migrate some keys
    hm_progressive_rehash(hmap);
}

HNode *hm_lookup(HMap *hmap, HNode *key, DataEqFn eq)
{
    hm_progressive_rehash(hmap);

    // query both hashtables, prioritize 'newer'
    HNode **from = htab_lookup(&hmap->newer, key, eq);
    if (!from)
        from = htab_lookup(&hmap->older, key, eq);
    return from ? *from : NULL;
}

HNode *hm_delete(HMap *hmap, HNode *key, DataEqFn eq)
{
    hm_progressive_rehash(hmap);

    // remove from both hashtables
    // - a new key is inserted in 'newer'.
    // - the same key may exist in 'older' (hasn't been migrated).
    // - if we only delete from 'newer', the stale key in 'older'
    //   will later be moved to 'newer'.
    if (HNode **from = htab_lookup(&hmap->newer, key, eq))
        return htab_detach(&hmap->newer, from);
    if (HNode **from = htab_lookup(&hmap->older, key, eq))
        return htab_detach(&hmap->older, from);
    return NULL;
}
