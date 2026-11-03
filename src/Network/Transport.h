#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <system_error>
#include <vector>

namespace CE::Network {
// Code Modification - Network: Provide a bounded, owner-pumped transport without exposing socket implementation types.
// Scope: one TCP peer or pending accept and one UDP endpoint per instance. Performance: bounded I/O handlers per pump.
// Toggle: construct and pump only for networked workloads.
struct Endpoint {
    std::array<std::uint8_t, 4> address{};
    std::uint16_t port{};

    bool operator==(const Endpoint&) const = default;
};

struct BindResult {
    Endpoint local;
    std::error_code error;
};

enum class Protocol { Tcp, Udp };
enum class EventKind { Connected, Message, Disconnected, Cancelled, Error };

struct Event {
    Protocol protocol;
    EventKind kind;
    Endpoint peer{};
    std::vector<std::byte> bytes;
    std::error_code error;
};

class Transport {
public:
    static constexpr std::size_t MaxTcpPayload = 16 * 1024;
    static constexpr std::size_t MaxUdpPayload = 1200;
    static constexpr std::size_t SendCapacity = 16;
    static constexpr std::size_t EventCapacity = 32;

    // Use all operations and destroy the transport on its constructing thread.
    Transport();
    ~Transport();
    Transport(const Transport&) = delete;
    Transport& operator=(const Transport&) = delete;

    BindResult listenTcp(Endpoint local);
    std::error_code connectTcp(Endpoint remote);
    BindResult bindUdp(Endpoint local);
    std::error_code sendTcp(std::span<const std::byte> payload);
    std::error_code sendUdp(Endpoint remote, std::span<const std::byte> payload);
    bool cancelTcp() noexcept;
    bool cancelUdp() noexcept;
    std::size_t pump(std::size_t maxHandlers = EventCapacity);
    std::size_t pendingEvents() const noexcept;
    std::optional<Event> poll();
    void shutdown() noexcept;

private:
    struct State;
    std::unique_ptr<State> state_;
};
// End of Code Modification - Network
}
