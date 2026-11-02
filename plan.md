# ft_irc — Project Plan

> **See also:** [architecture.md](architecture.md) for a diagram of the
> `poll()` loop itself and a step-by-step trace of how a client's message
> flows through it (including the fragmented-message case from the
> subject's `nc -C` test).

## Goal

Build `ircserv`, a C++98 IRC server that handles multiple clients at once,
non-blocking, with a **single `poll()`** (or equivalent) for all I/O.
No client, no server-to-server communication. Must survive a defense where a
real IRC client (our chosen reference client) connects and uses it.

## Hard constraints (read first, these are "grade = 0" rules)

- Never crash, never quit unexpectedly, no leaks that blow things up.
- Only **one** `poll()`/`select()`/`kqueue()`/`epoll()` call handling *everything*
  (accept, read, write).
- All sockets non-blocking. Never call `read`/`recv`/`write`/`send` outside
  of what `poll()` told us is ready.
- Never use `errno` (e.g. `EAGAIN`) to decide what to do next.
- Must handle partial/fragmented messages (TCP can split or merge lines).
  Test with `nc -C 127.0.0.1 6667` sending a command split with Ctrl+D.
- C++98 only, compiled with `-Wall -Wextra -Werror`, no external lib, no Boost.

## Split: two roles, one shared contract

We split the work into **Core/Network** and **IRC Logic/Commands**. They meet
in the middle through a small, agreed-upon interface (see "Shared contract"
below) so both people can work in parallel without stepping on each other's
files.

Do the kickoff together (see "Day 0" below), then split.

---

## Person A — Core & Network

**Owns:** the server loop, sockets, non-blocking I/O, raw message framing,
the `Client` class, graceful disconnects.

### Tasks

1. **CLI & bootstrap**
   - Parse `./ircserv <port> <password>`, validate port range, error messages.
   - Hint: reject non-numeric ports early; don't overthink validation, just
     don't crash on garbage input.

2. **Listening socket**
   - `socket()`, `setsockopt(SO_REUSEADDR)`, `bind()`, `listen()`.
   - Set it non-blocking (`fcntl(fd, F_SETFL, O_NONBLOCK)` — on macOS this is
     the *only* allowed `fcntl` flag per the subject).
   - Hint: support at least IPv4 to start; `AF_INET` + `sockaddr_in` is enough.

3. **The single poll() loop**
   - One `std::vector<pollfd>` (or `struct pollfd[]`), index 0 = listening
     socket, the rest = connected clients.
   - On `POLLIN` on the listening socket → `accept()` a new client, set it
     non-blocking, push a new `pollfd` with `POLLIN`.
   - On `POLLIN` on a client fd → `recv()` once, append to that client's
     input buffer, try to extract complete lines (split on `\r\n` or `\n`).
   - On `POLLOUT` on a client fd → flush that client's pending output buffer
     (only set `POLLOUT` interest when there's actually something queued —
     don't busy-loop on it).
   - Hint: this is the trickiest/most graded part. Keep it boring and
     explicit. No recv/send happens anywhere else in the codebase.

4. **Client buffering (handles the `nc -C` fragmentation test)**
   - Each `Client` has an input `std::string` buffer that accumulates raw
     bytes, and pulls out one command at a time when a full line arrives.
   - Each `Client` has an output `std::string` buffer so replies queue up
     even if the socket isn't writable yet.
   - Hint: this is what makes "com\^Dman\^Dd\n" work correctly as `command`.

5. **`Client` class**
   - fd, input buffer, output buffer, registration state (got `PASS`? got
     `NICK`? got `USER`?), nickname, username, realname, hostname, list of
     joined channels.

6. **Disconnection handling**
   - Detect `recv() == 0` or error → clean up: remove from all channels,
     notify others if needed, close fd, erase from the pollfd vector.
   - Hint: removing while iterating the pollfd vector is a classic bug —
     collect fds to remove, then erase after the loop.

7. **Low-level command splitting**
   - Turn a raw line into `(prefix?, command, params[])` per the IRC grammar
     (command is a word, params are space-separated, trailing param can
     start with `:` and contain spaces).
   - Hand the parsed command over to Person B's command dispatcher.

### Where to start

Get a listening socket + poll loop that accepts a connection and just echoes
back whatever it receives, before writing any IRC logic. Test with
`nc 127.0.0.1 <port>`.

---

## Person B — IRC Logic & Commands

**Owns:** `Channel` class, command implementations, IRC numeric replies.

### Tasks

1. **`Channel` class**
   - Name, topic, list of members (with operator flag per member), invite
     list, optional key (password), optional user limit.
   - Modes: `i` (invite-only), `t` (topic restricted to ops), `k` (key),
     `o` (operator), `l` (user limit).
   - Hint: a `std::map<std::string, Client*>` for members (or similar) keeps
     "is this nick in the channel" and "who are the ops" easy to check.

2. **Registration commands**
   - `PASS <password>` — must match the server's password, otherwise refuse
     registration.
   - `NICK <nickname>` — validate format, reject duplicates (numeric `433`).
   - `USER <username> 0 * :<realname>`
   - A client is "registered" only once it has valid PASS + NICK + USER.
     Hint: queue/ignore other commands politely until registration is done,
     don't crash on out-of-order commands.

3. **Core messaging commands**
   - `JOIN <channel>[,<channel2>...] [key[,key2]...]` — create channel if it
     doesn't exist (creator becomes operator), check invite-only/key/limit.
   - `PART <channel> [:reason]`
   - `PRIVMSG <target> :<message>` — target can be a nick or a channel;
     forward to all channel members except the sender.
   - `QUIT [:reason]`
   - Hint: every message, forward via Person A's "queue to client's output
     buffer" function — never call `send()` directly from this layer.

4. **Operator commands**
   - `KICK <channel> <nick> [:reason]`
   - `INVITE <nick> <channel>`
   - `TOPIC <channel> [:new topic]` (respects mode `t`)
   - `MODE <channel> <modes> [params]` for `i`, `t`, `k`, `o`, `l`
   - Hint: write one small helper `isOperator(channel, client)` and reuse it
     everywhere instead of duplicating the check.

5. **Numeric replies**
   - Implement the standard IRC numeric replies our reference client expects
     (e.g. `001 Welcome`, `433 Nickname in use`, `461 Not enough parameters`,
     `442 Not on channel`, `482 You're not channel operator`, `324 Channel
     mode is`, `332 Topic`, etc.).
   - Hint: keep a tiny helper that formats `:server <code> <nick> <rest>\r\n`
     so every reply looks the same.

6. **Command dispatcher**
   - A map from command name (string) to handler function, called by Person
     A once a full command has been parsed.

### Where to start

Pick 3-4 real IRC clients' behavior (or docs/RFC 1459 & 2812) as reference
for exact numeric replies, write `NICK`/`USER`/`JOIN`/`PRIVMSG` first since
everything else builds on them, test manually against the chosen reference
client as early as possible.

---

## Shared contract (agree on this together before splitting)

A small `Server` class ties both sides together. Decide together on
something like:

```cpp
class Server {
public:
    void queueToClient(int fd, const std::string &msg);   // Person B calls this, Person A implements it
    Client *getClientByNick(const std::string &nick);
    Channel *getChannel(const std::string &name);
    Channel *getOrCreateChannel(const std::string &name);
    void removeClient(int fd);                            // cleanup on disconnect
    // ...
};
```

As long as this interface is agreed early, A can build the network layer and
B can build command logic in parallel, even before the other side's code
exists (stub it out).

## Day 0 (do together, ~1-2h)

1. Agree on class names and the shared contract above.
2. Write the skeleton: `main`, `Server` with the poll loop, `Client`,
   `Channel` (empty), Makefile.
3. Get it to the point where `nc 127.0.0.1 <port>` can connect and the
   server echoes back raw bytes.
4. Split into the two tracks above.

## Testing checklist (both of you, continuously)

- [ ] `nc -C 127.0.0.1 <port>` fragmented command test (from the subject)
- [ ] Two `nc` clients at once, one sends to a channel, the other receives
- [ ] Reference client: connect, auth with password, set nick/user, join,
      send/receive PRIVMSG, KICK, INVITE, TOPIC, MODE i/t/k/o/l
- [ ] Wrong password rejected
- [ ] Duplicate nickname rejected
- [ ] Client disconnecting mid-channel doesn't crash the server or leave
      ghosts in the channel
- [ ] `-Wall -Wextra -Werror -std=c++98` compiles clean

## README reminder

`README.md` at repo root, in English, first line italicized:
*This project has been created as part of the 42 curriculum by
\<login1\>, \<login2\>.*
Plus Description / Instructions / Resources (incl. how AI was used) sections.

---

# Proposed file structure (approximate — adapt as you go)

```
ft_irc/
├── Makefile
├── README.md
├── plan.md
├── inc/
│   ├── Server.hpp
│   ├── Client.hpp
│   ├── Channel.hpp
│   ├── Command.hpp          # command dispatcher interface
│   └── replies.hpp          # numeric reply formatting helpers
├── src/
│   ├── main.cpp
│   ├── Server.cpp           # socket setup, poll loop, accept/read/write     [Person A]
│   ├── Client.cpp           # Client class: buffers, state                  [Person A]
│   ├── Channel.cpp          # Channel class: members, modes, topic          [Person B]
│   ├── commands/
│   │   ├── Registration.cpp # PASS, NICK, USER                              [Person B]
│   │   ├── Messaging.cpp    # JOIN, PART, PRIVMSG, QUIT                     [Person B]
│   │   ├── Operator.cpp     # KICK, INVITE, TOPIC, MODE                     [Person B]
│   │   └── Dispatcher.cpp   # command name -> handler map                   [Person B]
│   └── utils/
│       └── Parsing.cpp      # raw line -> (command, params)                 [Person A]
└── tests/                   # not graded, but handy during defense
    └── manual_test_notes.md
```

Notes:
- `inc/` vs `src/` split is just a habit — merge into one flat folder if you
  prefer, as long as the Makefile still only recompiles what changed.
- `commands/` as separate files per category keeps merge conflicts low since
  A and B rarely touch the same file.
- Feel free to rename/reshape once you're actually in it — this is a
  starting skeleton, not a contract.
