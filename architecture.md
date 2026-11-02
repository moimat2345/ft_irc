# ft_irc — Architecture: the poll() loop

This document is about **one thing**: how the single `poll()` loop works,
and how a message from a client travels through it. It's meant to be read
before writing `Server.cpp`, so the shape of the loop is settled up front
and doesn't need a rewrite halfway through.

It complements [plan.md](plan.md), which covers who does what.

No code here is meant to be copy-pasted as-is — it's pseudo-code to make the
*shape* of the solution concrete, especially the parts that are easy to get
wrong with C++'s `std::string`/`std::vector` if you're not comfortable with
them yet.

---

## Part 1 — The loop mechanics

```
┌─────────────────────────────────────────────────────────────┐
│                         SERVER START                         │
│  create listening socket -> bind -> listen -> non-blocking   │
└───────────────────────────┬───────────────────────────────────┘
                             │
                             v
              ┌─────────────────────────────┐
              │ fds = [ listening_socket ]   │  <- a std::vector<pollfd>
              └──────────────┬──────────────┘
                             │
                 ┌───────────v────────────┐
            ┌───►│        MAIN LOOP        │
            │    └───────────┬────────────┘
            │                │
            │                v
            │    ┌────────────────────────┐
            │    │   poll(fds, timeout)    │   <- the ONE poll() call
            │    └───────────┬────────────┘
            │                │
            │                v
            │     for each entry in fds:
            │                │
            │   ┌────────────┼─────────────────┬──────────────────┐
            │   v             v                 v                  v
            │ nothing    POLLIN on         POLLIN on          POLLOUT on
            │ happened   LISTENING fd      a CLIENT fd        a CLIENT fd
            │   │             │                 │                  │
            │  skip       accept() new      recv() ONCE,       send() ONCE
            │             client, set       append to that     from that
            │             it non-blocking,  client's input     client's
            │             push new pollfd   buffer, try to      output buffer
            │             entry (POLLIN)    pull out full       (see Part 2)
            │                               line(s)                 │
            │                                   │                    │
            │                                   v                    │
            │                          line complete? ->              │
            │                          dispatch command,               │
            │                          queue a reply in the             │
            │                          client's output buffer            │
            │                                   │
            │                          recv() returned 0              │
            │                          or an error?                   │
            │                          -> mark client for removal       │
            │                                   │                       │
            └───────────────────────────────────┴───────────────────────┘
                             │
                             v
              AFTER the for-loop: actually remove/close
              the clients marked for removal (never while
              you're still iterating the fds vector)
```

### ⚠️ Pitfalls that will force a rewrite if missed

- **A freshly `accept()`-ed fd is NOT non-blocking by default.** It does not
  inherit that from the listening socket. You must call
  `fcntl(new_fd, F_SETFL, O_NONBLOCK)` on every new client fd, every time.
- **Never erase an entry from the `pollfd` vector while iterating it in the
  same pass.** Collect the fds to remove in a small list first, then remove
  them after the `for` loop is done (or loop backwards from the end).
- **A closed fd number gets reused by the OS.** When a client disconnects,
  make sure you fully erase its `Client` object from your lookup (e.g. a
  `std::map<int, Client*>`) *before* a new client can be `accept()`-ed and
  possibly get that exact same fd number. Otherwise you'll mix up two
  different people's state.
- **Checking `errno` right after `poll()` itself is fine** (e.g. `EINTR`
  when a signal interrupts it, which just means "call `poll()` again"). The
  subject's ban on `errno` is specifically about `recv`/`send` — don't use
  it there to decide whether to retry a read/write.

### Minimal shape in (pseudo) code

```cpp
// conceptual shape — not copy-paste ready

struct Client {
    int fd;
    std::string inbuf;    // raw bytes received, not yet a full line
    std::string outbuf;   // bytes waiting to be sent out
    std::string nickname;
    bool registered;
    // ...
};

std::vector<pollfd> fds;        // fds[0] is always the listening socket
std::map<int, Client*> clients; // fd -> Client lookup

while (running) {
    int ready = poll(&fds[0], fds.size(), -1);

    if (ready < 0) {
        if (errno == EINTR) continue; // interrupted by a signal, just retry poll()
        break;                        // a real poll() failure
    }

    for (size_t i = 0; i < fds.size(); ++i) {
        if (fds[i].revents == 0) continue; // nothing happened on this fd

        if (fds[i].fd == listening_fd) {
            if (fds[i].revents & POLLIN) acceptNewClient();
            continue;
        }

        if (fds[i].revents & (POLLHUP | POLLERR)) {
            markForRemoval(fds[i].fd);
            continue;
        }
        if (fds[i].revents & POLLIN)  handleClientRead(fds[i].fd);
        if (fds[i].revents & POLLOUT) handleClientWrite(fds[i].fd);
    }

    removeDisconnectedClients(); // safe: happens AFTER the loop above
}
```

---

## Part 2 — One client's full journey (the fragmented-message case)

This traces exactly the scenario the subject tests with `nc -C`: a command
sent in several pieces (`"NIC"` then `"K bob\r\n"`), and shows where the
buffering actually happens.

```
Client                         Server (inside the poll loop)                 Client's stored state
------                         ------------------------------                ----------------------
connect()        --------->    poll() reports POLLIN on listening fd
                                accept() -> new fd 7
                                fcntl(7, F_SETFL, O_NONBLOCK)
                                fds.push_back({7, POLLIN})
                                clients[7] = new Client()                     inbuf = ""

send "NIC"       --------->    poll() reports POLLIN on fd 7
                                recv(7, buf, ...) -> "NIC" (3 bytes)
                                clients[7].inbuf += "NIC"                     inbuf = "NIC"
                                no full line yet ("\n" not found) -> wait

send "K bob\r\n" --------->    poll() reports POLLIN on fd 7
                                recv(7, buf, ...) -> "K bob\r\n"
                                clients[7].inbuf += "K bob\r\n"               inbuf = "NICK bob\r\n"
                                "\n" found! extract "NICK bob"
                                remainder kept in inbuf                       inbuf = ""
                                dispatchCommand(client, "NICK bob")
                                   -> clients[7].nickname = "bob"
                                   -> queueReply(client, ":server 001 ...")   outbuf = ":server 001 ...\r\n"
                                   -> ask poll() to also watch POLLOUT on fd 7

                                poll() reports POLLOUT on fd 7
                                send(7, outbuf.data(), outbuf.size(), 0)
                                   -> say it sent all the bytes
                                clients[7].outbuf.erase(0, n)                 outbuf = ""
                                outbuf now empty -> stop watching POLLOUT
<--------- "001 Welcome bob"
```

### ⚠️ Pitfalls specific to this part

- **`recv()` can hand you less than a full line, more than one line, or
  parts of several lines at once.** That's why `inbuf` is a growing buffer
  you keep appending to and repeatedly search for a line terminator in
  (`while` loop, not `if`) — there could be two full commands sitting in
  the buffer after a single `recv()`.
- **Accept `"\n"` as the line end even without `"\r"`.** Some tools (like
  raw `nc`) won't always send `\r\n`. Split on `\n`, then strip a trailing
  `\r` if it's there, instead of requiring `\r\n` exactly.
- **`send()` can also be partial** — it might report that it only sent part
  of `outbuf`. Always do `outbuf.erase(0, n)` with the *actual* number of
  bytes `send()` reported, never assume the whole buffer went out in one
  call.
- **Only watch `POLLOUT` on a client while it actually has something queued**
  in `outbuf`. Leaving `POLLOUT` permanently set makes `poll()` return
  immediately forever for no reason (the fd is basically always writable),
  which burns CPU and can look like a bug during evaluation.

### Read/write helpers, in the same spirit

```cpp
void handleClientRead(int fd) {
    char buf[1024];
    int n = recv(fd, buf, sizeof(buf), 0); // exactly ONE recv() call here

    if (n <= 0) {
        // n == 0 -> client closed the connection cleanly
        // n < 0  -> a real error happened
        // Either way: no retry, no errno check, just clean up.
        markForRemoval(fd);
        return;
    }

    Client &c = *clients[fd];
    c.inbuf.append(buf, n);

    size_t pos;
    while ((pos = c.inbuf.find('\n')) != std::string::npos) {
        std::string line = c.inbuf.substr(0, pos);
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        c.inbuf.erase(0, pos + 1);
        dispatchCommand(c, line);
    }
}

void handleClientWrite(int fd) {
    Client &c = *clients[fd];
    if (c.outbuf.empty()) return;

    int n = send(fd, c.outbuf.data(), c.outbuf.size(), 0); // ONE send() call

    if (n <= 0) {
        markForRemoval(fd);
        return;
    }

    c.outbuf.erase(0, n); // erase only what was ACTUALLY sent

    if (c.outbuf.empty())
        stopWatchingPollout(fd);
}

void queueReply(Client &c, const std::string &msg) {
    c.outbuf += msg;
    startWatchingPollout(c.fd); // tell poll() we now care about writability
}
```

---

## Quick reference: who builds what here

- Part 1 (the loop itself, accept/removal, the `fds`/`clients` plumbing) —
  **Person A**, per `plan.md`.
- Part 2's `handleClientRead`/`handleClientWrite`/buffer handling — also
  **Person A** (it's still network plumbing, not IRC logic).
- `dispatchCommand(...)` and everything it calls into — **Person B**.
