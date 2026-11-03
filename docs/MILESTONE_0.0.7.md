# WiseEngine 0.0.7

## Section 1 – C++ Code

`src/Network/Transport.h` exposes an owner-thread transport. Each instance supports one IPv4 TCP connection or one pending accept, plus one independent IPv4 UDP endpoint. A TCP listener accepts one peer and closes its listening socket. Bind to port zero to obtain an available local port. TCP clients connect asynchronously. All socket progress and completions run through `pump()` on the owner thread.

TCP uses a four-byte unsigned big-endian payload length followed by up to 16 KiB of data. An empty frame is valid. A partial frame produces no message. Oversized or truncated TCP frames close the connection with an error. UDP preserves datagram boundaries, including empty datagrams, and reports an error when an incoming datagram exceeds 1,200 bytes.

## Section 2 – Build Configuration

Network privately links the existing pinned standalone Asio target. Its public header exposes standard C++ types. `WISEENGINE_BUILD_TESTS` enables the loopback contract tests. The interactive application starts no sockets at this milestone.

## Section 3 – Scripts

Run `python cmake/VerifyThirdParty.py` from the engine root. Build with `./build.ps1 -Config Debug -Target all -Tests` and repeat with `-Config Release`. After committing and tagging, run `python cmake/BackupMilestone.py --version 0.0.7` to create and verify the recovery ZIP.

## Section 4 – Documentation

- Construct, pump, poll, cancel, and destroy a transport on the same owner thread. Each pump dispatches up to its caller-selected handler budget. `poll()` transfers one completion to the owner. Completion types cover connection, message, clean TCP disconnect, cancellation, and error.
- Each protocol queues up to 16 sends including the in-flight send. Queue overflow returns `std::errc::no_buffer_space`. A successful send submission means the bytes were copied and accepted into the local send queue.
- The shared event queue holds up to 32 completions. A full queue stops dispatch until the owner drains events. TCP progress then resumes through socket flow control. UDP remains best-effort and the OS may drop datagrams during backpressure.
- Cancellation closes the relevant socket and produces a completion on a subsequent pump. If reopening reports an operation still in progress, keep pumping to drain cancelled callbacks. Permanent shutdown closes both protocols, drains internal cancellation handlers, and discards unread events and unsent buffers.
- Network costs are opt-in. Pending payloads and completed messages use bounded transport-owned buffers. The owner controls handler work per pump. A transport has no DNS, connection deadlines, authentication, encryption, retransmission, or gameplay protocol in this milestone.

Incremental Debug and Release builds each passed all 17 CTest cases. The network contract test passed 50 consecutive runs in each configuration. It used Windows IPv4 loopback for TCP fragmentation, oversized and truncated frames, backpressure, clean disconnect, cancellation, UDP delivery, and shutdown. SDK provenance verification passed for all 25 pinned packages, their submodules, and three local generator packages.

## Section 5 – Usage Examples

Construct two `CE::Network::Transport` objects. On the server call `listenTcp({{127, 0, 0, 1}, 0})` and check the bind result. Pass its returned endpoint to the client's `connectTcp()`, then pump and poll both owners for `Connected`. Call `sendTcp()` with a byte span. For UDP, call `bindUdp()` on both ends, then `sendUdp()` with the destination endpoint and byte span.
