#include "Transport.h"

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/ip/udp.hpp>
#include <asio/read.hpp>
#include <asio/write.hpp>

#include <algorithm>
#include <cassert>
#include <deque>
#include <utility>

namespace CE::Network {
namespace {
// Code Modification - Network: Keep endpoint and frame encodings explicit at the Asio boundary.
// Scope: IPv4 endpoint and wire conversion. Performance: constant time per header. Toggle: none.
using asio::ip::tcp;
using asio::ip::udp;
using Bytes = std::vector<std::byte>;
enum Phase { Idle, Starting, Active, Cancelling };
constexpr std::size_t HeaderSize = 4;

template<class ProtocolType>
typename ProtocolType::endpoint native(Endpoint endpoint)
{
    return {asio::ip::address_v4(endpoint.address), endpoint.port};
}

template<class NativeEndpoint>
Endpoint endpoint(const NativeEndpoint& value)
{
    return {value.address().to_v4().to_bytes(), value.port()};
}

std::error_code status(std::errc code)
{
    return std::make_error_code(code);
}

std::size_t payloadLength(const std::array<std::byte, HeaderSize>& header)
{
    std::size_t length = 0;
    for (const auto part : header) {
        length = (length << 8) | std::to_integer<std::size_t>(part);
    }
    return length;
}

Bytes frame(std::span<const std::byte> payload)
{
    Bytes result(HeaderSize + payload.size());
    auto length = payload.size();
    for (std::size_t index = HeaderSize; index > 0; --index) {
        result[index - 1] = static_cast<std::byte>(length & 0xff);
        length >>= 8;
    }
    if (!payload.empty()) {
        std::copy(payload.begin(), payload.end(), result.begin() + HeaderSize);
    }
    return result;
}
// End of Code Modification - Network
}

// Code Modification - Network: Keep all callbacks and queue changes on the polling owner.
// Scope: one context, TCP lane and UDP lane. Performance: bounded buffers and one pending read/write per lane.
// Toggle: no worker threads or socket opens until requested.
struct Transport::State {
    struct Datagram {
        udp::endpoint destination;
        Bytes bytes;
    };

    void publish(Protocol protocol, EventKind kind, Endpoint peer = {},
        std::span<const std::byte> bytes = {}, std::error_code error = {})
    {
        assert(events.size() < EventCapacity);
        Event event{protocol, kind, peer, {}, error};
        if (!bytes.empty()) {
            event.bytes.assign(bytes.begin(), bytes.end());
        }
        events.push_back(std::move(event));
    }

    void closeTcp() noexcept
    {
        std::error_code ignored;
        listener.close(ignored);
        stream.close(ignored);
    }

    void closeUdp() noexcept
    {
        std::error_code ignored;
        datagram.close(ignored);
    }

    void finishTcp(EventKind kind, std::error_code error = {})
    {
        tcpPhase = Idle;
        closeTcp();
        if (!tcpWritePending) {
            tcpWrites.clear();
        }
        publish(Protocol::Tcp, kind, tcpPeer, {}, error);
    }

    bool completeTcpPrimary()
    {
        tcpPrimaryPending = false;
        if (closed || tcpPhase == Idle) {
            return false;
        }
        if (tcpPhase == Cancelling) {
            finishTcp(EventKind::Cancelled);
            return false;
        }
        return true;
    }

    void connected(std::error_code error)
    {
        if (!completeTcpPrimary()) {
            return;
        }
        if (!error) {
            const auto peer = stream.remote_endpoint(error);
            if (!error) {
                tcpPeer = endpoint(peer);
            }
        }
        if (error) {
            finishTcp(EventKind::Error, error);
            return;
        }
        std::error_code ignored;
        listener.close(ignored);
        tcpPhase = Active;
        publish(Protocol::Tcp, EventKind::Connected, tcpPeer);
        readHeader();
    }

    void readHeader()
    {
        tcpPrimaryPending = true;
        asio::async_read(stream, asio::buffer(header), [this](std::error_code error, std::size_t count) {
            if (!completeTcpPrimary()) {
                return;
            }
            if (error == asio::error::eof) {
                if (count == 0) {
                    finishTcp(EventKind::Disconnected);
                } else {
                    finishTcp(EventKind::Error, status(std::errc::protocol_error));
                }
                return;
            }
            if (error) {
                finishTcp(EventKind::Error, error);
                return;
            }
            const auto length = payloadLength(header);
            if (length > MaxTcpPayload) {
                finishTcp(EventKind::Error, status(std::errc::message_size));
                return;
            }
            if (length == 0) {
                publish(Protocol::Tcp, EventKind::Message, tcpPeer);
                readHeader();
                return;
            }
            readPayload(length);
        });
    }

    void readPayload(std::size_t length)
    {
        tcpPrimaryPending = true;
        asio::async_read(stream, asio::buffer(tcpBuffer.data(), length), [this, length](std::error_code error, std::size_t) {
            if (!completeTcpPrimary()) {
                return;
            }
            if (error) {
                finishTcp(EventKind::Error, error == asio::error::eof ? status(std::errc::protocol_error) : error);
                return;
            }
            publish(Protocol::Tcp, EventKind::Message, tcpPeer, std::span(tcpBuffer).first(length));
            readHeader();
        });
    }

    void writeTcp()
    {
        tcpWritePending = true;
        asio::async_write(stream, asio::buffer(tcpWrites.front()), [this](std::error_code error, std::size_t) {
            tcpWritePending = false;
            tcpWrites.pop_front();
            if (closed || tcpPhase != Active) {
                tcpWrites.clear();
                return;
            }
            if (error) {
                finishTcp(EventKind::Error, error);
                return;
            }
            if (!tcpWrites.empty()) {
                writeTcp();
            }
        });
    }

    void readUdp()
    {
        udpReadPending = true;
        datagram.async_receive_from(asio::buffer(udpBuffer), udpPeer, [this](std::error_code error, std::size_t size) {
            udpReadPending = false;
            if (closed) {
                return;
            }
            if (udpCancelling) {
                udpCancelling = false;
                publish(Protocol::Udp, EventKind::Cancelled);
                return;
            }
            if (error == asio::error::message_size || (!error && size > MaxUdpPayload)) {
                publish(Protocol::Udp, EventKind::Error, {}, {}, status(std::errc::message_size));
            } else if (error) {
                closeUdp();
                if (!udpWritePending) {
                    udpWrites.clear();
                }
                publish(Protocol::Udp, EventKind::Error, {}, {}, error);
                return;
            } else {
                publish(Protocol::Udp, EventKind::Message, endpoint(udpPeer), std::span(udpBuffer).first(size));
            }
            readUdp();
        });
    }

    void writeUdp()
    {
        const auto& next = udpWrites.front();
        udpWritePending = true;
        datagram.async_send_to(asio::buffer(next.bytes), next.destination, [this](std::error_code error, std::size_t size) {
            const auto peer = endpoint(udpWrites.front().destination);
            const auto expected = udpWrites.front().bytes.size();
            udpWritePending = false;
            udpWrites.pop_front();
            if (closed || !datagram.is_open()) {
                udpWrites.clear();
                return;
            }
            if (error || size != expected) {
                publish(Protocol::Udp, EventKind::Error, peer, {}, error ? error : status(std::errc::io_error));
            }
            if (!udpWrites.empty()) {
                writeUdp();
            }
        });
    }

    bool closed{};
    Phase tcpPhase{Idle};
    bool tcpPrimaryPending{};
    bool tcpWritePending{};
    bool udpReadPending{};
    bool udpWritePending{};
    bool udpCancelling{};
    Endpoint tcpPeer{};
    udp::endpoint udpPeer;
    std::array<std::byte, HeaderSize> header{};
    std::array<std::byte, MaxTcpPayload> tcpBuffer{};
    std::array<std::byte, MaxUdpPayload + 1> udpBuffer{};
    std::deque<Bytes> tcpWrites;
    std::deque<Datagram> udpWrites;
    std::deque<Event> events;
    // The context and socket objects are destroyed before their async backing buffers.
    asio::io_context io;
    tcp::acceptor listener{io};
    tcp::socket stream{io};
    udp::socket datagram{io};
};
// End of Code Modification - Network

// Code Modification - Network: Admit bounded work and stop polling when the owner completion queue fills.
// Scope: owner API and socket lifecycle. Performance: bounded by caller handler budget and queue limits.
// Toggle: explicit session, pump budget and per-protocol cancellation.
Transport::Transport() : state_(std::make_unique<State>()) {}

Transport::~Transport()
{
    shutdown();
}

BindResult Transport::listenTcp(Endpoint local)
{
    auto& state = *state_;
    if (state.closed) {
        return {{}, status(std::errc::operation_canceled)};
    }
    if (state.tcpPhase != Idle || state.tcpPrimaryPending || state.tcpWritePending) {
        return {{}, status(std::errc::operation_in_progress)};
    }
    std::error_code error;
    state.listener.open(tcp::v4(), error);
    if (!error) {
        state.listener.bind(native<tcp>(local), error);
    }
    if (!error) {
        state.listener.listen(asio::socket_base::max_listen_connections, error);
    }
    if (!error) {
        const auto bound = state.listener.local_endpoint(error);
        if (!error) {
            local = endpoint(bound);
        }
    }
    if (error) {
        state.closeTcp();
        return {{}, error};
    }
    state.tcpPeer = {};
    state.tcpPhase = Starting;
    state.tcpPrimaryPending = true;
    state.listener.async_accept(state.stream, [&state](std::error_code result) { state.connected(result); });
    return {local, {}};
}

std::error_code Transport::connectTcp(Endpoint remote)
{
    auto& state = *state_;
    if (state.closed) {
        return status(std::errc::operation_canceled);
    }
    if (state.tcpPhase != Idle || state.tcpPrimaryPending || state.tcpWritePending) {
        return status(std::errc::operation_in_progress);
    }
    state.tcpPeer = remote;
    state.tcpPhase = Starting;
    state.tcpPrimaryPending = true;
    state.stream.async_connect(native<tcp>(remote), [&state](std::error_code error) { state.connected(error); });
    return {};
}

BindResult Transport::bindUdp(Endpoint local)
{
    auto& state = *state_;
    if (state.closed) {
        return {{}, status(std::errc::operation_canceled)};
    }
    if (state.datagram.is_open() || state.udpReadPending || state.udpWritePending) {
        return {{}, status(std::errc::operation_in_progress)};
    }
    std::error_code error;
    state.datagram.open(udp::v4(), error);
    if (!error) {
        state.datagram.bind(native<udp>(local), error);
    }
    if (!error) {
        const auto bound = state.datagram.local_endpoint(error);
        if (!error) {
            local = endpoint(bound);
        }
    }
    if (error) {
        state.closeUdp();
        return {{}, error};
    }
    state.readUdp();
    return {local, {}};
}

std::error_code Transport::sendTcp(std::span<const std::byte> payload)
{
    auto& state = *state_;
    if (state.tcpPhase != Active) {
        return status(std::errc::not_connected);
    }
    if (payload.size() > MaxTcpPayload) {
        return status(std::errc::message_size);
    }
    if (state.tcpWrites.size() == SendCapacity) {
        return status(std::errc::no_buffer_space);
    }
    state.tcpWrites.push_back(frame(payload));
    if (!state.tcpWritePending) {
        state.writeTcp();
    }
    return {};
}

std::error_code Transport::sendUdp(Endpoint remote, std::span<const std::byte> payload)
{
    auto& state = *state_;
    if (!state.datagram.is_open()) {
        return status(std::errc::not_connected);
    }
    if (payload.size() > MaxUdpPayload) {
        return status(std::errc::message_size);
    }
    if (state.udpWrites.size() == SendCapacity) {
        return status(std::errc::no_buffer_space);
    }
    State::Datagram packet{native<udp>(remote), {}};
    if (!payload.empty()) {
        packet.bytes.assign(payload.begin(), payload.end());
    }
    state.udpWrites.push_back(std::move(packet));
    if (!state.udpWritePending) {
        state.writeUdp();
    }
    return {};
}

bool Transport::cancelTcp() noexcept
{
    auto& state = *state_;
    if (state.tcpPhase == Idle || state.tcpPhase == Cancelling) {
        return false;
    }
    state.tcpPhase = Cancelling;
    state.closeTcp();
    return true;
}

bool Transport::cancelUdp() noexcept
{
    auto& state = *state_;
    if (!state.datagram.is_open()) {
        return false;
    }
    state.udpCancelling = true;
    state.closeUdp();
    return true;
}

std::size_t Transport::pump(std::size_t maxHandlers)
{
    auto& state = *state_;
    if (state.closed) {
        return 0;
    }
    state.io.restart();
    std::size_t dispatched = 0;
    while (dispatched < maxHandlers && state.events.size() < EventCapacity && state.io.poll_one() != 0) {
        ++dispatched;
    }
    return dispatched;
}

std::size_t Transport::pendingEvents() const noexcept
{
    return state_->events.size();
}

std::optional<Event> Transport::poll()
{
    auto& events = state_->events;
    if (events.empty()) {
        return std::nullopt;
    }
    Event result = std::move(events.front());
    events.pop_front();
    return result;
}

void Transport::shutdown() noexcept
{
    auto& state = *state_;
    if (state.closed) {
        return;
    }
    state.closed = true;
    state.tcpPhase = Idle;
    state.closeTcp();
    state.closeUdp();
    state.io.restart();
    state.io.run();
    state.tcpWrites.clear();
    state.udpWrites.clear();
    state.events.clear();
}
// End of Code Modification - Network
}
