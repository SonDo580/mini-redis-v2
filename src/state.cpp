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
    assert(ent->heap_idx != (size_t)-1);
    heap_delete(g_data.ttl_heap, ent->heap_idx);
    ent->heap_idx = -1;
}

void entry_del(Entry *ent)
{
    if (ent->type == T_ZSET)
        zset_clear(&ent->zset);
    entry_remove_ttl(ent);
    delete ent;
}
