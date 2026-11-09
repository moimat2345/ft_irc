# ft_irc — Dev Log

A running log of what actually got done, session by session. Written in
plain terms so either of us (or a future version of us, two weeks from now)
can tell what state the project was in on a given day without digging
through commits.

See [plan.md](plan.md) for who's doing what, and [architecture.md](architecture.md)
for the poll() loop design.

---

## 2026-11-02 — Planning + skeleton headers

**Goal for the session:** stop staring at the subject and get something
on disk — the plan, the architecture doc, and the shape of every class.

**Did:**
- Wrote `plan.md` (task split between the two of us) and `architecture.md`
  (diagram + pseudo-code for the single `poll()` loop and how a client's
  message flows through it).
- Built `index.html`, a colored/interactive version of the same plan, for
  a quicker read than two markdown files.
- Set up `header/includes.hpp`: every standard/system header the project
  needs (sockets, poll, fcntl, signal, string/vector/map/set), in one
  place so every other header just includes this one.
- Declared `header/client.hpp`: the `Client` class — socket fd, nickname,
  username, realname, hostname, a `_recvBuffer`/`_sendBuffer` pair for
  partial/fragmented messages, `_passOk`/`_registered` flags, full
  Orthodox Canonical Form (default ctor, copy ctor, `operator=`, dtor).
- Declared `header/channel.hpp`: the `Channel` class — name, topic,
  members, operators and invited users as `std::vector`/`std::set` of
  `Client *`, the four channel modes (`i`/`t`/`k`/`l`), and a
  `broadcast()` helper.
- Declared `header/message.hpp`: a small `Message` class whose job is to
  turn one raw line into a command name + parameter list.
- Declared `header/serveur.hpp`: the `Server` class — owns the `pollfd`
  vector, the `fd -> Client` and `name -> Channel` maps, the network-side
  methods (`setupSocket`, `acceptClient`, `readFromClient`, `flushClient`,
  `removeClient`), and one `cmdXxx()` method per IRC command (PASS, NICK,
  USER, JOIN, PART, PRIVMSG, QUIT, KICK, INVITE, TOPIC, MODE). Copy
  constructor and `operator=` are declared private and left unimplemented
  on purpose — a `Server` should never be copied.
- Wrote `src/main.cpp`: argument count check, port validation (numeric,
  1–65535), password validation (non-empty, no whitespace), then
  constructs and runs the `Server` inside a `try`/`catch` so a thrown
  exception prints a clean error instead of crashing.

**Status at end of session:** every class interface exists on paper
(headers only). No `.cpp` implementation yet except `main.cpp`. Nothing
compiles into a running server yet — next session is about filling in the
`.cpp` files, starting with constructors/destructors.

**Next:** implement `client.cpp` and `channel.cpp` (constructors, getters/
setters, buffer handling), then `serveur.cpp`'s networking half.

---

## 2026-11-08 — Makefile, `client.cpp`, `channel.cpp`, `message.cpp` done

**Goal for the session:** get something that actually compiles, and finish
the `Client` class end to end (solo today, teammate hasn't started yet).

**Did:**
- Wrote the `Makefile`: `NAME`/`all`/`clean`/`fclean`/`re`, compiles with
  `-Wall -Wextra -Werror -std=c++98`. Source files are picked up
  automatically via `$(wildcard src/*.cpp)` (no need to edit it when a new
  `.cpp` is added), and `-MMD -MP` generates per-file `.d` dependency files
  so editing a header only recompiles what actually includes it — no
  unnecessary relinking.
- Added `getHostname()`/`setHostname()` to `header/client.hpp` — missing
  piece spotted while reviewing: `_hostname` existed but had no way to be
  set. `setHostname()` will be called from `Server::acceptClient()` once
  the network side exists, using the client's IP from the socket.
- Implemented `src/client.cpp` in full:
  - Canonical form (constructor, copy constructor, `operator=`, destructor).
    Destructor is intentionally empty — `Client` does not close its own fd,
    that's `Server`'s job (avoids a double-`close()`).
  - Getters/setters for fd, nick, username, realname, hostname.
  - `getPrefix()` — builds `"nick!user@host"`.
  - `isPassOk()`/`isRegistered()`/`tryRegister()` — registration only
    flips to true once PASS succeeded and both nick/username are set.
  - `appendToRecvBuffer()`/`extractLine()` — accumulates raw bytes and
    pulls out one full line at a time, handling both `\r\n` and bare `\n`,
    including the case where `recv()` splits a message right between the
    `\r` and the `\n`.
  - `queueMessage()`/`hasPendingData()`/`getSendBuffer()` — the output
    buffer side, ready for `Server::flushClient()` to drain later.
- Compiles clean under `-Wall -Wextra -Werror -std=c++98` — no warnings
  at any point while building it up.
- Reorganized `header/channel.hpp`: was previously grouped loosely, now
  split into commented sections (`canonical form`, `identity & topic`,
  `members`, `operators`, `invites`, `messaging`, `modes: i, t, k, l`),
  matching the section style already used in `serveur.hpp`. Each mode's
  `set`/`is`/`get` are kept next to each other instead of grouped by
  method type.
- Implemented `src/channel.cpp` in full, following the header's section
  order:
  - Canonical form.
  - `addMember()`/`addOperator()` both guard against invalid state
    (`addMember` skips if already a member; `addOperator` refuses anyone
    who isn't a member first).
  - `removeMember()` uses the erase-remove idiom on `_members` (a
    `std::vector`) and also erases the client from `_operators` and
    `_invited`, so a client who leaves can't keep operator rights or a
    stale invite.
  - `isMember()`/`isOperator()`/`isInvited()` via `std::find` (vector) /
    `.find()` (set).
  - `broadcast()` — loops `_members`, queues `msg` on everyone except
    `exclude` (the sender) via `Client::queueMessage()`.
  - Mode getters/setters, trivial.
- Implemented `src/message.cpp`: canonical form, then `parse()` — splits
  a raw IRC line into `_command` (first word) and `_params` (the rest),
  using `std::istringstream` + `>>` to read word by word, with the IRC
  rule that a parameter starting with `:` means "take the rest of the
  line as one single parameter, spaces included" (read via `getline`).
  `getCommand()`/`getParams()` finish the class.

**Status at end of session:** the whole "common" layer is done —
`client.cpp`, `channel.cpp`, `message.cpp` all compile clean with no
warnings. Only `Server::setupSocket()` and `Server::run()` are still
missing to link `ircserv` — everything else is in place for whoever
starts the network core.

## 2026-11-09 — Theory notes, README, listening socket, poll() loop started
 
**Goal for the session:** start the network core (Person A's half of
`plan.md`): read up on the socket functions first, then get
`Server::setupSocket()` working so `ircserv` really binds and listens on
the port.
 
**Did (committed):**
- Started `NOTES.md` — theory notes written *before* the code: `recv()`,
  `fcntl()` + `O_NONBLOCK`, `poll()` and `struct pollfd`, `socket()`,
  `bind()` (why the port goes through `htons`), `listen()` (the backlog and
  the socket → bind → listen → accept sequence), `setsockopt()`, and
  connection-oriented vs connectionless sockets. Also linked RFC 2812 and
  added `Service-Client.png`. The "Blocking and Non-Blocking File
  Descriptors" section is only a stub for now.
- Created `README.md`: the mandatory italic first line (with both logins,
  `matnusko` and `nkuydin`) and the three required section headings.
  Only "Resources" has content so far (one link about core socket
  functions). **"Description" and "Instructions" are still empty** — the
  subject requires both, plus a note in "Resources" on how AI was used.
- `header/includes.hpp`: added `LOG_SERVER(message)` and the `BLUE`/`RESET`
  ANSI colour codes, so server logs look like `Server -> ...`.
- Implemented `Server::setupSocket()` in `src/serveur.cpp` (Person A,
  task 2): `socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)` →
  `fcntl(F_SETFL, O_NONBLOCK)` → `setsockopt(SO_REUSEADDR)` → `bind()` on
  `INADDR_ANY`:port (struct zeroed with `memset`, port through `htons`) →
  `listen(SOMAXCONN)`. Every step throws a `std::runtime_error` on failure,
  which `main()` already catches and prints as `Fatal error: ...` instead
  of crashing. `fcntl` is used only in the form the subject allows.
  Port and password are logged at start-up.
- Added `Server::run()` as a placeholder: it only logs "Main loop not
  configured yet" and returns.
- `header/serveur.hpp`: the private copy constructor and `operator=` now
  have an empty inline body (`{ (void)other; ... }`) instead of being
  declared only. The 2026-11-02 line saying they were "left unimplemented
  on purpose" is therefore out of date.
- `src/main.cpp`: `atoi` → `std::atoi` (C++ header style, as the subject
  prefers).


**Started, commented out yet** (working tree at the time of writing):
- `setupSocket()` now pushes the listening socket into `_pollFds`, so it
  is index 0 of the vector, as `architecture.md` describes.
- First skeleton of the single `poll()` loop in `Server::run()`:
  `poll(&_pollFds[0], _pollFds.size(), -1)`, retry on `EINTR`, walk
  through all fds, test `revents & POLLIN`, and split into "listening
  socket → new client" / "client socket → read data". **Both branches are
  only comments so far, so this version does not compile**
  (`expected primary-expression before 'else'`, `serveur.cpp` line 60).
  The last committed state does compile.
- Whitespace-only tweak in `main.cpp`.

**Status at end of session:** the committed code builds clean under
`-Wall -Wextra -Werror -std=c++98`. `./ircserv <port> <password>` starts,
binds, listens and then exits straight away, because `run()` is still a
stub. Person A tasks 1 (CLI) and 2 (listening socket) are done, task 3
(poll loop) is started, task 6 (disconnections) is not. Tasks 4, 5 and 7
are mostly covered already by `client.cpp` / `message.cpp` from 11-08.


**Next:** finish the loop, in the order of `architecture.md`:
`acceptClient()` (`accept`, set non-blocking, `setHostname()` from the
IP, new `pollfd` with `POLLIN`, new `Client` in `_clients`), then
`readFromClient()` (one `recv()` → `appendToRecvBuffer()` → loop on
`extractLine()`), then `flushClient()` with `POLLOUT` only when
`hasPendingData()`, and `removeClient()`. First milestone from the plan:
an echo test with `nc 127.0.0.1 <port>`.
