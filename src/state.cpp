// stdlib
#include <assert.h>

#include "state.hpp"
#include "server_utils.hpp"

GlobalData g_data; // define (to allocate memory)

Entry *entry_new(uint32_t type)
{
    Entry *ent = new Entry();
    ent->type = type;
    return ent;
}

void entry_set_ttl(Entry *ent, uint64_t ttl_ms)
{
    uint64_t expire_at = get_monotonic_msec() + (uint64_t)ttl_ms;
    HeapItem item = {expire_at, &ent->heap_idx};
    heap_upsert(g_data.ttl_heap, ent->heap_idx, item);
}

void entry_remove_ttl(Entry *ent)
{
    if (ent->heap_idx == (size_t)-1) // entry doesn't have TTL
        return;
    heap_delete(g_data.ttl_heap, ent->heap_idx);
    ent->heap_idx = -1;
}

static void entry_del_sync(Entry *ent)
{
    if (ent->type == T_ZSET)
        zset_clear(&ent->zset);
    delete ent;
}

static void entry_del_task_fn(void *arg)
{
    entry_del_sync((Entry *)arg);
}

void entry_del(Entry *ent)
{
    // unlink from any data structures
    // (entry already removed from top-level hash table)
    entry_remove_ttl(ent); // remove from TTL heap

    // run destructor for large structures in worker thread
    const size_t k_large_zset_size = 500;
    size_t zset_size = 
        (ent->type == T_ZSET) ? hm_size(&ent->zset.hmap) : 0; // number of pairs
    if (zset_size > k_large_zset_size)
        thread_pool_queue(&g_data.thread_pool, entry_del_task_fn, ent);
    else
        entry_del_sync(ent); // small -> avoid context switch
}
