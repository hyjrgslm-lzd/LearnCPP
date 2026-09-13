#include <c11/socket.hpp>
#include <c11/connection.hpp>
#include <check.hpp>
#include <array>
#include <iostream>
#include <thread>
using namespace std::chrono_literals;
int main() {
    c11::socket_runtime runtime;
    auto listener = c11::listen_loopback();
    check(listener.has_value(), "loopback listener created");
    const auto end = c11::clock::now() + 5s;
    auto client = c11::connect_loopback(listener->port, end);
    check(client.has_value(), "client connected");
    check(c11::wait_ready(listener->socket.get(), POLLRDNORM, end).has_value(), "accept readiness");
    auto peer = c11::accept_socket(listener->socket.get());
    check(peer.has_value(), "peer accepted");
    std::string failure;
    std::jthread server([&] {
        c11::framed_connection state;
        std::array<char, 4096> bytes{};
        while (state.state() != c11::connection_state::closed && c11::clock::now() < end) {
            short events = state.can_read() ? POLLRDNORM : 0;
            if (!state.output().empty()) events |= POLLWRNORM;
            c11::poll_fd fd{peer->get(), events, 0};
            if (c11::poll_sockets(std::span{&fd, 1}, 20) < 0) { failure = "poll failed"; return; }
            if (state.can_read() && (fd.revents & (POLLRDNORM | POLLHUP))) {
                auto n = c11::read_some(peer->get(), bytes);
                if (n && *n == 0) { if (!state.eof()) { failure = "truncated EOF"; return; } }
                else if (n) { if (!state.receive(std::string_view(bytes.data(), *n))) { failure = "decode failed"; return; } }
                else if (!c11::would_block(n.error().value())) { failure = n.error().message(); return; }
            }
            if (!state.output().empty() && (fd.revents & POLLWRNORM)) {
                auto n = c11::write_some(peer->get(), state.output());
                if (n && *n) state.sent(*n);
                else if (!n && !c11::would_block(n.error().value())) { failure = n.error().message(); return; }
            }
            if (fd.revents & (POLLERR | POLLNVAL)) { failure = "poll error"; return; }
        }
        if (state.state() != c11::connection_state::closed) failure = "server deadline";
        peer->reset(); // Real EOF follows the complete write drain.
    });
    const std::string input = *c11::encode_frame("first") + *c11::encode_frame(std::string("a\0b", 3));
    for (char c : input) check(c11::send_all(client->get(), std::span{&c, 1}, end).has_value(), "fragment transmitted");
    check(c11::shutdown_write(client->get()).has_value(), "client half-closes write direction");
    auto received = c11::receive_exact(client->get(), input.size(), end);
    check(received && *received == input, "real socket returns complete frames after half-close");
    check(c11::wait_ready(client->get(), POLLRDNORM, end).has_value(), "final EOF readiness");
    std::array<char, 1> last{};
    const auto eof = c11::read_some(client->get(), last);
    check(eof && *eof == 0, "peer EOF after drain");
    server.join();
    check(failure.empty(), failure);
    std::cout << "real TCP partial-input and half-close checks passed\n";
}
