// stdlib
#include <assert.h>
#include <string.h>
// system
#include <poll.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>

#include "common.hpp"
#include "server_utils.hpp"
#include "exec.hpp"
#include "state.hpp"

/* application callback when listenfd is ready:
   accept a new connection; return NULL on error. */
static Conn *handle_accept(int listenfd)
{
    // accept
    struct sockaddr_in client_addr = {};
    socklen_t addrlen = sizeof(client_addr);
    int connfd = accept(
        listenfd, (struct sockaddr *)&client_addr, &addrlen);
    if (connfd < 0)
    {
        msg_errno("accept() error");
        return NULL;
    }

    // (client_addr uses big-endian byte order)
    uint32_t ip = client_addr.sin_addr.s_addr;
    printf("new client from %u.%u.%u.%u:%u\n",
           ip & 255, (ip >> 8) & 255, (ip >> 16) & 255, ip >> 24,
           ntohs(client_addr.sin_port));

    // set the new connfd to non-blocking mode
    fd_set_nb(connfd);

    // create Conn
    Conn *conn = new Conn();
    conn->fd = connfd;
    conn->want_read = true;

    // add to back of idle list
    conn->last_active_ms = get_monotonic_msec();
    dlist_insert_before(&g_data.idle_list, &conn->idle_node);

    // save to map
    if (g_data.fd2conn.size() <= (size_t)conn->fd)
        g_data.fd2conn.resize(conn->fd + 1);
    g_data.fd2conn[conn->fd] = conn;

    return conn;
}

static void conn_destroy(Conn *conn)
{
    close(conn->fd);
    g_data.fd2conn[conn->fd] = NULL;
    dlist_detach(&conn->idle_node);
    delete conn;
}

/* application callback when connfd is writable. */
static void handle_write(Conn *conn)
{
    // non-blocking write
    assert(conn->outgoing.size() > 0);
    ssize_t rv = write(conn->fd, &conn->outgoing[0], conn->outgoing.size());

    if (rv < 0)
    {
        if (errno == EAGAIN) // actually not ready (kernel's TCP buffer is full)
            return;
        else
        {
            msg_errno("write() error");
            conn->want_close = true;
            return;
        }
    }

    // remove written data from 'outgoing' buffer
    buf_consume(conn->outgoing, (size_t)rv);

    // update intention if needed
    if (conn->outgoing.size() == 0) // all data written
    {
        // request-response protocol: alternate between 2 states
        conn->want_read = true;
        conn->want_write = false;
    }
    // else: want_write (still) = true
}

static bool try_one_request(Conn *conn);

/* application callback when connfd is readable. */
static void handle_read(Conn *conn)
{
    // non-blocking read
    uint8_t buf[64 * 1024]; // 64 KB
    ssize_t rv = read(conn->fd, buf, sizeof(buf));

    if (rv < 0)
    {
        if (errno == EAGAIN) // actually not ready (kernel's TCP buffer is empty)
            return;
        else
        {
            msg_errno("read() error");
            conn->want_close = true;
            return;
        }
    }

    if (rv == 0)
    { // EOF
        if (conn->incoming.size() == 0)
            msg("client closed");
        else
            msg("unexpected EOF");
        conn->want_close = true;
        return;
    }

    // add new data to 'incoming' buffer
    buf_append(conn->incoming, buf, (size_t)rv);

    // parse request(s) and generate response(s)
    while (try_one_request(conn))
        ;
    // why use a loop: to support 'pipelineing'
    // - normally a client sends 1 request, then waits for 1 response;
    //   a pipelined client sends n requests, then waits for n responses.
    // - the server still handles each request in order,
    //   but it can get multiple requests in 1 read,
    //   effectively reducing the number of IOs.
    // - requests pipelining also reduces latency on the client side,
    //   because it can get multiple response in 1 RTT (round trip time).

    // update intention if needed
    if (conn->outgoing.size() > 0) // has a response
    {
        // request-response protocol: alternate between 2 states
        conn->want_read = false;
        conn->want_write = true;

        // try to write response without waiting for the next event loop iteration
        // - socket is likely ready to write unless kernel's TCP send buffer is full
        // - some "bad" cases:
        //   . network congestion.
        //   . client haven't read previous responses (in case of request pipelining).
        handle_write(conn);
    }
    // else: want_read (still) = true
}

/* read uint32_t from curr.
   - advance curr and return true on success.
   - return false on failure. */
static bool read_u32(
    const uint8_t *&curr, const uint8_t *end, uint32_t &out)
{
    if (curr + 4 > end)
        return false;
    memcpy(&out, curr, 4);
    curr += 4;
    return true;
}

/* read string of size n from curr.
   - advance curr and return true on success.
   - return false on failure. */
static bool read_str(
    const uint8_t *&curr, const uint8_t *end, size_t n, std::string &out)
{
    if (curr + n > end)
        return false;
    out.assign(curr, curr + n);
    curr += n;
    return true;
}

/* parse request payload (command);
   return 0 if success, -1 if fail. */
static int32_t parse_req(
    const uint8_t *data, size_t size, std::vector<std::string> &out)
{
    const uint8_t *end = data + size;

    // number of strings
    uint32_t nstr = 0;
    if (!read_u32(data, end, nstr))
        return -1;
    if (nstr > K_MAX_ARGS)
        return -1;

    while (out.size() < nstr)
    {
        // string length
        uint32_t len = 0;
        if (!read_u32(data, end, len))
            return -1;

        // string
        out.push_back(std::string());
        if (!read_str(data, end, len, out.back()))
            return -1;
    }

    if (data != end) // trailing garbage
        return -1;

    return 0;
}

/* set start of response in out buffer & reserve space for header. */
static void response_begin(Buffer &out, size_t *header_pos)
{
    *header_pos = out.size(); // continue from current end
    buf_append_u32(out, 0);   // reserve space
}

/* final response payload size */
static size_t response_size(Buffer &out, size_t header_pos)
{
    return out.size() - header_pos - 4;
}

/* backfill final response size into response message header;
   error if response is too big. */
static void response_end(Buffer &out, size_t header_pos)
{
    size_t msg_size = response_size(out, header_pos);
    if (msg_size > K_MAX_MSG)
    {
        out.resize(header_pos + 4);
        out_err(out, ERR_TOO_BIG, "response is too big.");
        msg_size = response_size(out, header_pos);
    }

    // message header
    uint32_t len = (uint32_t)msg_size;
    memcpy(&out[header_pos], &len, 4);
}

/* process 1 request if there's enough data;
   return true on success. */
static bool try_one_request(Conn *conn)
{
    // payload size
    if (conn->incoming.size() < 4)
        return false; // keep reading
    uint32_t len = 0;
    memcpy(&len, conn->incoming.data(), 4);
    if (len > K_MAX_MSG)
    {
        msg("too long");
        conn->want_close = true;
        return false;
    }

    // payload
    if (4 + len > conn->incoming.size())
        return false; // keep reading
    const uint8_t *request = &conn->incoming[4];

    // application logic for 1 request
    std::vector<std::string> cmd;
    if (parse_req(request, len, cmd) < 0)
    {
        msg("bad_request");
        conn->want_close = true;
        return false; // error
    }
    size_t header_pos; // start of response in 'outgoing'
    response_begin(conn->outgoing, &header_pos);
    exec_cmd(cmd, conn->outgoing);
    response_end(conn->outgoing, header_pos);

    // application logic done, remove request message
    buf_consume(conn->incoming, 4 + len);
    // why not just empty 'incoming' buffer:
    // - because there may be more messages (support request 'pipelining').

    return true;
}

/* calculate poll() timeout; return -1 if for 'no timeout'. */
static int32_t next_timer_ms()
{
    uint64_t now_ms = get_monotonic_msec();
    uint64_t next_ms = (uint64_t)-1;

    // check 1st item in idle list (most "stale" connection)
    if (!dlist_empty(&g_data.idle_list))
    {
        Conn *conn = container_of(
            g_data.idle_list.next, Conn, idle_node); // skip dummy head
        next_ms = conn->last_active_ms + K_IDLE_TIMEOUT_MS;
    }

    // check root of min heap for TTL (smallest timeout ms)
    if (!g_data.ttl_heap.empty() && g_data.ttl_heap[0].val < next_ms)
        next_ms = g_data.ttl_heap[0].val;

    if (next_ms == (uint64_t)-1)
        return -1; // no timeout

    if (next_ms <= now_ms) // last process_timers() missed
        return 0;

    return (int32_t)(next_ms - now_ms);
}

/* implement HEqFn: strict equality (compare pointers). */
static bool hnode_same(HNode *node, HNode *key)
{
    return node == key;
}

static void process_timers()
{
    uint64_t now_ms = get_monotonic_msec();

    // remove timed-out connections
    while (!dlist_empty(&g_data.idle_list))
    {
        Conn *conn = container_of(
            g_data.idle_list.next, Conn, idle_node); // skip dummy head
        uint64_t next_ms = conn->last_active_ms + K_IDLE_TIMEOUT_MS;
        if (next_ms > now_ms)
            break; // (idle list is in descending order of idle time)

        printf("removing idle connection: %d\n", conn->fd);
        conn_destroy(conn);
    }

    // remove expired entries (next timeout ms at heap root)
    const size_t k_max_works = 2000;
    size_t nworks = 0;
    const Heap &heap = g_data.ttl_heap;
    while (!heap.empty() && heap[0].val < now_ms)
    {
        Entry *ent = container_of(heap[0].ref, Entry, heap_idx);
        HNode *node = hm_delete(&g_data.db, &ent->node, hnode_same);
        assert(node == &ent->node);

        printf("key expired: %s\n", ent->key.c_str());
        entry_del(ent);
        nworks++;

        // don't stall the server if too many keys expire at once
        if (nworks == k_max_works)
            break;
    }
}

int main()
{
    dlist_init(&g_data.idle_list);

    // listenfd
    int fd = socket(AF_INET, SOCK_STREAM, 0); // IPv4 + TCP
    if (fd < 0)
        die("socket()");

    // allow binding to the same `IP:port` after restart
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // bind
    struct sockaddr_in addr = {}; // holds IPv4:port as big-endian numbers
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);     // port
    addr.sin_addr.s_addr = htonl(0); // 0.0.0.0 (to listen on all network interfaces)
    int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv)
        die("bind()");

    // set listenfd to non-blocking mode
    fd_set_nb(fd);

    // listen
    rv = listen(fd, SOMAXCONN);
    if (rv)
        die("listen()");

    // the event loop
    std::vector<struct pollfd> poll_args;
    while (true)
    {
        // prepare arguments for poll()
        poll_args.clear();
        // - put listenfd in the 1st position
        struct pollfd pfd = {fd, POLLIN, 0};
        poll_args.push_back(pfd);
        // - the rest are connfd's
        for (Conn *conn : g_data.fd2conn)
        {
            if (!conn)
                continue;
            struct pollfd pfd = {conn->fd, POLLERR, 0};
            if (conn->want_read)
                pfd.events |= POLLIN;
            if (conn->want_write)
                pfd.events |= POLLOUT;
            poll_args.push_back(pfd);
        }

        // wait for readiness or timeout
        int32_t timeout_ms = next_timer_ms();
        int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), timeout_ms);
        if (rv < 0)
        {
            if (errno == EINTR)
                continue; // signal interrupt -> not error
            else
                die("poll"); // error
        }

        // handle ready listenfd: accept new connection
        if (poll_args[0].revents)
            handle_accept(fd);

        // handle ready connfd's
        for (size_t i = 1; i < poll_args.size(); i++)
        {
            uint32_t ready = poll_args[i].revents;
            if (ready == 0)
                continue;

            Conn *conn = g_data.fd2conn[poll_args[i].fd];

            // update idle timer, move conn to end of idle list
            conn->last_active_ms = get_monotonic_msec();
            dlist_detach(&conn->idle_node);
            dlist_insert_before(&g_data.idle_list, &conn->idle_node);

            // handle I/O
            if (ready & POLLIN)
            {
                assert(conn->want_read);
                handle_read(conn);
            }
            if (ready & POLLOUT)
            {
                assert(conn->want_write);
                handle_write(conn);
            }

            // close connfd on error or server intention
            if ((ready & POLLERR) || conn->want_close)
                conn_destroy(conn);
        }

        // handle timers
        process_timers();
    }

    return 0;
}