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

**Known follow-up (not urgent):** `Message::parse()` stores `_command`
exactly as received; the header comment says "uppercase command name"
but nothing uppercases it yet. IRC commands are case-insensitive, so
either uppercase it in `parse()` or make `dispatch()` compare
case-insensitively — pick one before relying on it.

**Next:** `src/serveur_core.cpp` (or split `serveur.cpp`) — `setupSocket`,
`run`, `acceptClient`, `readFromClient`, `flushClient`, `removeClient` —
the network half, per `architecture.md`.
