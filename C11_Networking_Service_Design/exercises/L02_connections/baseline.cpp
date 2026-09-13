#include <c11/socket.hpp>
#include <c11/frame_contract.hpp>
#include <check.hpp>
#include <charconv>
#include <iostream>
#include <thread>
using namespace std::chrono_literals;

// Sequential, one bounded request at a time; a loop inside receive_exact handles
// short reads. This is a correct baseline, not the deliberately wrong one-read model.
c11::net_result<std::string> read_frame(c11::socket_handle socket, c11::deadline end) {
    auto header = c11::receive_exact(socket, 8, end);
    if (!header) return std::unexpected(header.error());
    std::size_t size = 0;
    const auto [last, error] = std::from_chars(header->data(), header->data()+8, size);
    if (error != std::errc{} || last != header->data()+8 || size > c11::max_frame)
        return std::unexpected(std::make_error_code(std::errc::protocol_error));
    return c11::receive_exact(socket, size, end);
}
int main() {
    c11::socket_runtime runtime;
    auto listener = c11::listen_loopback();
    check(listener.has_value(), "baseline listener");
    auto end = c11::clock::now() + 5s;
    auto client = c11::connect_loopback(listener->port, end);
    check(client.has_value(), "baseline connect");
    check(c11::wait_ready(listener->socket.get(), POLLRDNORM, end).has_value(), "baseline accept readiness");
    auto peer = c11::accept_socket(listener->socket.get());
    check(peer.has_value(), "baseline accept");
    std::string server_result;
    bool sent = false;
    std::jthread server([&] {
        auto body = read_frame(peer->get(), end);
        if (!body) return;
        server_result = *body;
        const auto response = *c11::encode_frame(*body);
        sent = c11::send_all(peer->get(), response, end).has_value();
    });
    const std::string body("hello\0world", 11);
    const auto wire = *c11::encode_frame(body);
    check(c11::send_all(client->get(), wire, end).has_value(), "baseline send request");
    auto result = read_frame(client->get(), end);
    check(result && *result == body, "baseline complete response");
    server.join();
    check(sent && server_result == body, "baseline peer completed");
    // Controlled transport provides only a prefix on its first read. Run the
    // candidate algorithm on that input, then compare against the frame contract.
    const auto one_read_candidate = [](std::string_view available) { return std::string(available); };
    check(one_read_candidate(std::string_view(wire).substr(0, 3)) != wire,
          "one-read candidate cannot deliver a complete frame from partial input");
    std::cout << "correct sequential TCP baseline; one-read candidate rejected by controlled input\n";
}
