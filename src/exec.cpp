// stdlib
#include <assert.h>
// C++
#include <string>
#include <vector>

#include "exec.hpp"
#include "state.hpp"
#include "server_utils.hpp"

// dummy Entry without value for lookup
struct LookupKey
{
    struct HNode node; // intrusive hash table node
    std::string key;
};

/* implement HEqFn for 'Entry' and 'LookupKey': compare keys */
static bool entry_eq(HNode *node, HNode *key)
{
    struct Entry *ent = container_of(node, struct Entry, node);
    struct LookupKey *key_data = container_of(key, struct LookupKey, node);
    return ent->key == key_data->key;
}

/* get dummy entry for lookup. */
static LookupKey get_lookup_key(std::string &key_str)
{
    LookupKey key;
    key.key.swap(key_str);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
    return key;
}

/* get key:
   - get string value by key.
   - response with nil if not found. */
static void exec_get(std::vector<std::string> &cmd, Buffer &out)
{
    // dummy entry for lookup
    LookupKey key = get_lookup_key(cmd[1]);

    // hashtable lookup
    HNode *node = hm_lookup(&g_data.db, &key.node, entry_eq);
    if (!node) // not found
        return out_nil(out);

    // copy value
    Entry *ent = container_of(node, Entry, node);
    if (ent->type != T_STR)
        return out_err(out, ERR_BAD_TYPE, "not a string value");
    return out_str(out, ent->str.data(), ent->str.size());
}

/* set key val:
   - insert key-value pair OR update value by key.
   - response with nil on success. */
static void exec_set(std::vector<std::string> &cmd, Buffer &out)
{
    // dummy entry for lookup
    LookupKey key = get_lookup_key(cmd[1]);

    // hashtable lookup
    HNode *node = hm_lookup(&g_data.db, &key.node, entry_eq);
    if (node)
    { // found -> update value
        Entry *ent = container_of(node, Entry, node);
        if (ent->type != T_STR)
            return out_err(out, ERR_BAD_TYPE, "a non-string value exists");
        ent->str.swap(cmd[2]);
    }
    else
    { // not found -> allocate & insert new KV pair
        Entry *ent = entry_new(T_STR);
        ent->key.swap(key.key);
        ent->node.hcode = key.node.hcode;
        ent->str.swap(cmd[2]);
        hm_insert(&g_data.db, &ent->node);
    }

    return out_nil(out);
}

/* del key:
   - delete an entry.
   - response: 1 if deleted, 0 if not found. */
static void exec_del(std::vector<std::string> &cmd, Buffer &out)
{
    // dummy entry for lookup
    LookupKey key = get_lookup_key(cmd[1]);

    // hashtable delete
    HNode *node = hm_delete(&g_data.db, &key.node, &entry_eq);
    if (node)
        // deallocate the KV pair
        entry_del(container_of(node, Entry, node));

    return out_int(out, node ? 1 : 0);
}

static bool cb_keys(HNode *node, void *args)
{
    Buffer &out = *(Buffer *)args;
    const std::string &key = container_of(node, Entry, node)->key;
    out_str(out, key.data(), key.size());
    return true;
}

/* keys: get all keys in DB. */
static void exec_keys(std::vector<std::string> &, Buffer &out)
{
    out_arr(out, (uint32_t)hm_size(&g_data.db));
    hm_foreach(&g_data.db, &cb_keys, (void *)&out);
}

/* get Entry by LookupKey */
static Entry *get_entry(LookupKey &key)
{
    HNode *hnode = hm_lookup(&g_data.db, &key.node, entry_eq);
    return hnode ? container_of(hnode, Entry, node) : NULL;
}

/* get Entry by key string */
static Entry *get_entry(std::string &key_str)
{
    LookupKey key = get_lookup_key(key_str);
    return get_entry(key);
}

/* zadd zset score name:
   - add (score, name) to zset OR update score of existing name.
   - response: 1 if added, 0 if updated. */
static void exec_zadd(std::vector<std::string> &cmd, Buffer &out)
{
    double score;
    if (!str2dbl(cmd[2], score))
        return out_err(out, ERR_BAD_ARG, "expect double");

    // get entry
    LookupKey key = get_lookup_key(cmd[1]);
    Entry *ent = get_entry(key);

    if (!ent)
    { // not found -> insert new zset
        ent = entry_new(T_ZSET);
        ent->key.swap(key.key);
        ent->node.hcode = key.node.hcode;
        hm_insert(&g_data.db, &ent->node);
    }
    else
    { // check existing key
        if (ent->type != T_ZSET)
            return out_err(out, ERR_BAD_TYPE, "expect zset");
    }

    // add (score, name) or update score of existing name
    const std::string &name = cmd[3];
    bool added = zset_insert(
        &ent->zset, name.data(), name.size(), score);
    return out_int(out, (int64_t)added);
}

/* zrem zset name:
   - remove name from zset.
   - response: 1 if deleted, 0 if not found. */
static void exec_zrem(std::vector<std::string> &cmd, Buffer &out)
{
    // get the zset
    Entry *ent = get_entry(cmd[1]);
    if (!ent)
        return out_int(out, 0);
    if (ent->type != T_ZSET)
        return out_err(out, ERR_BAD_TYPE, "expect zset");
    ZSet *zset = &ent->zset;

    const std::string &name = cmd[2];
    ZNode *znode = zset_lookup(zset, name.data(), name.size());
    if (znode)
        zset_delete(zset, znode);
    return out_int(out, znode ? 1 : 0);
}

/* zscore zset name:
   - get score by name.
   - response with nil if not found. */
static void exec_zscore(std::vector<std::string> &cmd, Buffer &out)
{
    // get the zset
    Entry *ent = get_entry(cmd[1]);
    if (!ent)
        return out_nil(out);
    if (ent->type != T_ZSET)
        return out_err(out, ERR_BAD_TYPE, "expect zset");
    ZSet *zset = &ent->zset;

    const std::string &name = cmd[2];
    ZNode *znode = zset_lookup(zset, name.data(), name.size());
    return znode ? out_dbl(out, znode->score) : out_nil(out);
}

/* zquery zset score name offset limit:
   - seek to 1st pair >= (score, name)
   - move to the successor/predecessor (offset)
   - iterate forward and output.
     response: array of (name, score) tuples in specified range. */
static void exec_zquery(std::vector<std::string> &cmd, Buffer &out)
{
    // parse args
    double score;
    if (!str2dbl(cmd[2], score))
        return out_err(out, ERR_BAD_ARG, "expect double");

    const std::string &name = cmd[3];

    int64_t offset, limit;
    if (!str2int(cmd[4], offset) || !str2int(cmd[5], limit))
        return out_err(out, ERR_BAD_ARG, "expect int");

    // get the zset
    Entry *ent = get_entry(cmd[1]);
    if (!ent)
        return out_arr(out, 0);
    if (ent->type != T_ZSET)
        return out_err(out, ERR_BAD_TYPE, "expect zset");
    ZSet *zset = &ent->zset;

    if (limit <= 0)
        return out_arr(out, 0);

    // seek to the start key
    ZNode *znode = zset_seek_ge(zset, score, name.data(), name.size());
    znode = znode_offset(znode, offset);

    // iterate and output
    size_t arr_size_pos = out_begin_arr(out);
    int64_t n = 0; // TODO: in uint32 range
    while (znode && n < limit)
    {
        out_str(out, znode->name, znode->len);
        out_dbl(out, znode->score);
        n += 2; // (name, score) are counted as separate items
        znode = znode_offset(znode, 1);
    }
    out_end_arr(out, arr_size_pos, (uint32_t)n);
}

/* pexpire key ttl_ms:
   - set TTL for an entry in ms.
   - return 1 if entry exists, 0 otherwise. */
static void exec_pexpire(std::vector<std::string> &cmd, Buffer &out)
{
    int64_t ttl_ms;
    if (!str2int(cmd[2], ttl_ms))
        return out_err(out, ERR_BAD_ARG, "expect int64");
    assert(ttl_ms >= 0); // TODO: in int32 range

    Entry *ent = get_entry(cmd[1]);
    if (!ent)
    {
        out_int(out, 0);
        return;
    }

    entry_set_ttl(ent, (uint64_t)ttl_ms);
    out_int(out, 1);
}

/* persist key:
   - remove TTL for an entry.
   - return 1 if entry exists and has TTL, 0 otherwise. */
static void exec_persist(std::vector<std::string> &cmd, Buffer &out)
{
    Entry *ent = get_entry(cmd[1]);
    if (!ent || ent->heap_idx == (size_t)-1)
    { // entry not found or doesn't have TTL
        out_int(out, 0);
        return;
    }

    entry_remove_ttl(ent);
    out_int(out, 1);
}

/* pttl key:
   - return remaining TTL of an entry in ms.
   - return -2 if entry doesn't exists.
   - return -1 if entry doesn't have TTL. */
static void exec_pttl(std::vector<std::string> &cmd, Buffer &out)
{
    Entry *ent = get_entry(cmd[1]);
    if (!ent)
    {
        out_int(out, -2); // not found
        return;
    }
    if (ent->heap_idx == (size_t)-1)
    {
        out_int(out, -1); // no TTL
        return;
    }

    uint64_t expire_at = g_data.ttl_heap[ent->heap_idx].val;
    uint64_t now_ms = get_monotonic_msec();
    return out_int(out, (expire_at > now_ms) ? (expire_at - now_ms) : 0);
}

/* execute a request command and produce response. */
void exec_cmd(std::vector<std::string> &cmd, Buffer &out)
{
    if (cmd.size() == 2 && cmd[0] == "get")
        exec_get(cmd, out);
    else if (cmd.size() == 3 && cmd[0] == "set")
        exec_set(cmd, out);
    else if (cmd.size() == 2 && cmd[0] == "del")
        exec_del(cmd, out);
    else if (cmd.size() == 1 && cmd[0] == "keys")
        exec_keys(cmd, out);
    else if (cmd.size() == 4 && cmd[0] == "zadd")
        exec_zadd(cmd, out);
    else if (cmd.size() == 3 && cmd[0] == "zrem")
        exec_zrem(cmd, out);
    else if (cmd.size() == 3 && cmd[0] == "zscore")
        exec_zscore(cmd, out);
    else if (cmd.size() == 6 && cmd[0] == "zquery")
        exec_zquery(cmd, out);
    else if (cmd.size() == 3 && cmd[0] == "pexpire")
        exec_pexpire(cmd, out);
    else if (cmd.size() == 2 && cmd[0] == "persist")
        exec_persist(cmd, out);
    else if (cmd.size() == 2 && cmd[0] == "pttl")
        exec_pttl(cmd, out);
    else
        out_err(out, ERR_UNKNOWN, "unknown command.");
}
