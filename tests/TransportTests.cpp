#include "Network/Transport.h"

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/ip/udp.hpp>
#include <asio/write.hpp>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace {
// Code Modification - Tests: Verify the actual TCP wire protocol and UDP delivery using loopback peers.
// Scope: transport contract test. Performance: test-only sockets and bounded waits. Toggle: WISEENGINE_BUILD_TESTS.
using namespace CE::Network;
using namespace std::chrono_literals;
constexpr Endpoint Loopback{{127, 0, 0, 1}, 0};
constexpr std::array Sample{std::byte{7}, std::byte{11}};

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void check(std::error_code error)
{
    if (error) {
        throw std::system_error(error);
    }
}

template<class Predicate>
void waitFor(Predicate predicate)
{
    const auto deadline = std::chrono::steady_clock::now() + 4s;
    do {
        if (predicate()) {
            return;
        }
        std::this_thread::sleep_for(1ms);
    } while (std::chrono::steady_clock::now() < deadline);
    throw std::runtime_error("Transport deadline expired");
}

Event receive(Transport& owner, Transport* other = nullptr)
{
    std::optional<Event> result;
    waitFor([&] {
        if (other) {
            other->pump();
        }
        owner.pump();
        result = owner.poll();
        return result.has_value();
    });
    return std::move(*result);
}

void expect(const Event& event, Protocol protocol, EventKind kind)
{
    if (event.protocol != protocol || event.kind != kind) {
        throw std::runtime_error("Unexpected network event: " + event.error.message());
    }
}

void fragmentation()
{
    Transport server;
    const auto local = server.listenTcp(Loopback);
    check(local.error);
    asio::io_context io;
    asio::ip::tcp::socket raw(io);
    raw.connect({asio::ip::address_v4::loopback(), local.local.port});
    raw.set_option(asio::ip::tcp::no_delay(true));
    expect(receive(server), Protocol::Tcp, EventKind::Connected);

    const std::array<unsigned char, 2> headerPart{0, 0};
    asio::write(raw, asio::buffer(headerPart));
    waitFor([&] { return server.pump() != 0; });
    require(!server.poll(), "Partial TCP header produced a message");

    const std::array<unsigned char, 3> payloadPart{0, 2, 7};
    asio::write(raw, asio::buffer(payloadPart));
    waitFor([&] { return server.pump() != 0; });
    require(!server.poll(), "Partial TCP payload produced a message");

    const std::array<unsigned char, 5> rest{11, 0, 0, 0, 0};
    asio::write(raw, asio::buffer(rest));
    const auto message = receive(server);
    expect(message, Protocol::Tcp, EventKind::Message);
    require(message.bytes == std::vector<std::byte>(Sample.begin(), Sample.end()), "TCP reconstructed the wrong bytes");
    const auto empty = receive(server);
    expect(empty, Protocol::Tcp, EventKind::Message);
    require(empty.bytes.empty(), "Empty frame was lost");

    std::array<unsigned char, (Transport::EventCapacity + 2) * 4> emptyFrames{};
    asio::write(raw, asio::buffer(emptyFrames));
    waitFor([&] {
        server.pump();
        return server.pendingEvents() == Transport::EventCapacity;
    });
    require(server.pump() == 0, "A full event queue must pause dispatch");
    std::size_t received = 0;
    while (const auto event = server.poll()) {
        expect(*event, Protocol::Tcp, EventKind::Message);
        ++received;
    }
    require(received == Transport::EventCapacity, "Completion queue exceeded its bound");
    while (received < Transport::EventCapacity + 2) {
        expect(receive(server), Protocol::Tcp, EventKind::Message);
        ++received;
    }

    raw.shutdown(asio::ip::tcp::socket::shutdown_send);
    expect(receive(server), Protocol::Tcp, EventKind::Disconnected);

    const auto next = server.listenTcp(Loopback);
    check(next.error);
    asio::ip::tcp::socket malformed(io);
    malformed.connect({asio::ip::address_v4::loopback(), next.local.port});
    expect(receive(server), Protocol::Tcp, EventKind::Connected);
    const std::array<unsigned char, 4> oversized{0, 0, 64, 1};
    asio::write(malformed, asio::buffer(oversized));
    const auto error = receive(server);
    expect(error, Protocol::Tcp, EventKind::Error);
    require(error.error == std::errc::message_size, "Oversized TCP header was accepted");

    const auto truncatedBind = server.listenTcp(Loopback);
    check(truncatedBind.error);
    asio::ip::tcp::socket truncated(io);
    truncated.connect({asio::ip::address_v4::loopback(), truncatedBind.local.port});
    expect(receive(server), Protocol::Tcp, EventKind::Connected);
    const std::array<unsigned char, 5> truncatedFrame{0, 0, 0, 2, 7};
    asio::write(truncated, asio::buffer(truncatedFrame));
    truncated.shutdown(asio::ip::tcp::socket::shutdown_send);
    const auto incomplete = receive(server);
    expect(incomplete, Protocol::Tcp, EventKind::Error);
    require(incomplete.error == std::errc::protocol_error, "Truncated TCP payload was accepted");
}

void peersAndCancellation()
{
    Transport server;
    Transport client;
    auto listening = server.listenTcp(Loopback);
    check(listening.error);
    require(server.cancelTcp() && !server.cancelTcp(), "Accept cancellation failed");
    expect(receive(server), Protocol::Tcp, EventKind::Cancelled);

    listening = server.listenTcp(Loopback);
    check(listening.error);
    check(client.connectTcp(listening.local));
    expect(receive(client, &server), Protocol::Tcp, EventKind::Connected);
    expect(receive(server), Protocol::Tcp, EventKind::Connected);
    for (std::size_t index = 0; index < Transport::SendCapacity; ++index) {
        check(client.sendTcp(Sample));
    }
    require(client.sendTcp(Sample) == std::errc::no_buffer_space, "TCP send queue exceeded capacity");
    for (std::size_t index = 0; index < Transport::SendCapacity; ++index) {
        const auto message = receive(server, &client);
        expect(message, Protocol::Tcp, EventKind::Message);
        require(message.bytes.size() == Sample.size() && message.bytes.front() == Sample.front(), "TCP bytes were lost");
    }
    require(client.cancelTcp(), "Active TCP cancellation failed");
    expect(receive(client), Protocol::Tcp, EventKind::Cancelled);
    const auto remoteClose = receive(server);
    require(remoteClose.protocol == Protocol::Tcp && (remoteClose.kind == EventKind::Disconnected ||
        (remoteClose.kind == EventKind::Error && remoteClose.error == asio::error::connection_reset)),
        "The remote peer did not observe the cancelled TCP connection");

    const auto left = server.bindUdp(Loopback);
    const auto right = client.bindUdp(Loopback);
    check(left.error);
    check(right.error);
    check(client.sendUdp(left.local, Sample));
    check(client.sendUdp(left.local, {}));
    const auto datagram = receive(server, &client);
    expect(datagram, Protocol::Udp, EventKind::Message);
    require(datagram.peer == right.local && datagram.bytes == std::vector<std::byte>(Sample.begin(), Sample.end()),
        "UDP endpoint or bytes differ");
    const auto empty = receive(server, &client);
    expect(empty, Protocol::Udp, EventKind::Message);
    require(empty.bytes.empty(), "Empty UDP datagram was lost");

    asio::io_context io;
    asio::ip::udp::socket raw(io, asio::ip::udp::v4());
    const std::array<char, Transport::MaxUdpPayload + 8> oversized{};
    raw.send_to(asio::buffer(oversized), {asio::ip::address_v4::loopback(), left.local.port});
    const auto error = receive(server);
    expect(error, Protocol::Udp, EventKind::Error);
    require(error.error == std::errc::message_size, "Truncated UDP datagram reached the owner as valid");

    for (std::size_t index = 0; index < Transport::SendCapacity; ++index) {
        check(client.sendUdp(left.local, Sample));
    }
    require(client.sendUdp(left.local, Sample) == std::errc::no_buffer_space, "UDP send queue exceeded capacity");
    require(client.cancelUdp(), "UDP cancellation failed");
    expect(receive(client), Protocol::Udp, EventKind::Cancelled);

    server.shutdown();
    client.shutdown();
    client.shutdown();
    require(!server.poll() && server.pump() == 0 && server.bindUdp(Loopback).error == std::errc::operation_canceled,
        "Shutdown left events or allowed reopening");
}
// End of Code Modification - Tests
}

int main()
{
    try {
        fragmentation();
        peersAndCancellation();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
