#pragma once
#include <algorithm>
#include <chrono>
#include <climits>
#include <cstdint>
#include <expected>
#include <span>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace c11 {
using clock = std::chrono::steady_clock;
using deadline = clock::time_point;
template<class T> using net_result = std::expected<T, std::error_code>;
#ifdef _WIN32
using socket_handle = SOCKET;
using socket_length = int;
using poll_fd = WSAPOLLFD;
inline constexpr auto invalid_socket = INVALID_SOCKET;
inline int socket_error() noexcept { return WSAGetLastError(); }
inline bool interrupted(int e) noexcept { return e == WSAEINTR; }
inline bool would_block(int e) noexcept { return e == WSAEWOULDBLOCK; }
inline bool connecting(int e) noexcept { return would_block(e) || e == WSAEINPROGRESS; }
inline std::error_code net_error(int e) { return {e, std::system_category()}; }
inline int poll_sockets(std::span<poll_fd> fds, int timeout) {
    return WSAPoll(fds.data(), static_cast<ULONG>(fds.size()), timeout);
}
class socket_runtime {
public:
    socket_runtime() {
        WSADATA data{};
        const int e = WSAStartup(MAKEWORD(2, 2), &data);
        if (e) throw std::system_error(net_error(e));
    }
    ~socket_runtime() { WSACleanup(); }
    socket_runtime(const socket_runtime&) = delete;
    socket_runtime& operator=(const socket_runtime&) = delete;
};
#else
using socket_handle = int;
using socket_length = socklen_t;
using poll_fd = pollfd;
inline constexpr int invalid_socket = -1;
inline int socket_error() noexcept { return errno; }
inline bool interrupted(int e) noexcept { return e == EINTR; }
inline bool would_block(int e) noexcept { return e == EAGAIN || e == EWOULDBLOCK; }
inline bool connecting(int e) noexcept { return would_block(e) || e == EINPROGRESS; }
inline std::error_code net_error(int e) { return {e, std::generic_category()}; }
inline int poll_sockets(std::span<poll_fd> fds, int timeout) { return ::poll(fds.data(), fds.size(), timeout); }
struct socket_runtime {};
#endif

class unique_socket {
    socket_handle handle_ = invalid_socket;
public:
    unique_socket() noexcept = default;
    explicit unique_socket(socket_handle h) noexcept : handle_(h) {}
    ~unique_socket() { reset(); }
    unique_socket(const unique_socket&) = delete;
    unique_socket& operator=(const unique_socket&) = delete;
    unique_socket(unique_socket&& s) noexcept : handle_(s.release()) {}
    unique_socket& operator=(unique_socket&& s) noexcept { if (this != &s) reset(s.release()); return *this; }
    socket_handle get() const noexcept { return handle_; }
    explicit operator bool() const noexcept { return handle_ != invalid_socket; }
    socket_handle release() noexcept { return std::exchange(handle_, invalid_socket); }
    void reset(socket_handle h = invalid_socket) noexcept {
        if (*this) {
#ifdef _WIN32
            ::closesocket(handle_);
#else
            ::close(handle_); // Do not retry close(EINTR): descriptor reuse is possible.
#endif
        }
        handle_ = h;
    }
};

inline net_result<void> nonblocking(socket_handle s) {
#ifdef _WIN32
    u_long enabled = 1;
    if (::ioctlsocket(s, FIONBIO, &enabled)) return std::unexpected(net_error(socket_error()));
#else
    const int flags = ::fcntl(s, F_GETFL, 0);
    if (flags == -1 || ::fcntl(s, F_SETFL, flags | O_NONBLOCK) == -1)
        return std::unexpected(net_error(socket_error()));
#endif
    return {};
}
inline net_result<void> wait_ready(socket_handle s, short events, deadline end) {
    for (;;) {
        const auto remaining = std::chrono::ceil<std::chrono::milliseconds>(end - clock::now()).count();
        if (remaining <= 0) return std::unexpected(std::make_error_code(std::errc::timed_out));
        poll_fd fd{s, events, 0};
        const int n = poll_sockets(std::span{&fd, 1}, static_cast<int>(std::min<long long>(remaining, INT_MAX)));
        if (n > 0) {
            if (fd.revents & POLLNVAL) return std::unexpected(std::make_error_code(std::errc::bad_file_descriptor));
            return {}; // Read/write/SO_ERROR determines the actual outcome, including HUP/ERR.
        }
        if (n < 0 && !interrupted(socket_error())) return std::unexpected(net_error(socket_error()));
    }
}
inline net_result<std::size_t> read_some(socket_handle s, std::span<char> bytes) {
    if (bytes.empty()) return std::size_t{0};
    for (;;) {
        const auto n = ::recv(s, bytes.data(), static_cast<int>(std::min<std::size_t>(bytes.size(), INT_MAX)), 0);
        if (n >= 0) return static_cast<std::size_t>(n);
        const int e = socket_error();
        if (!interrupted(e)) return std::unexpected(net_error(e));
    }
}
inline net_result<std::size_t> write_some(socket_handle s, std::span<const char> bytes) {
    if (bytes.empty()) return std::size_t{0};
#ifdef _WIN32
    constexpr int flags = 0;
#else
    constexpr int flags = MSG_NOSIGNAL;
#endif
    for (;;) {
        const auto n = ::send(s, bytes.data(), static_cast<int>(std::min<std::size_t>(bytes.size(), INT_MAX)), flags);
        if (n >= 0) return static_cast<std::size_t>(n);
        const int e = socket_error();
        if (!interrupted(e)) return std::unexpected(net_error(e));
    }
}
inline net_result<void> send_all(socket_handle s, std::span<const char> bytes, deadline end) {
    while (!bytes.empty()) {
        if (clock::now() >= end) return std::unexpected(std::make_error_code(std::errc::timed_out));
        auto n = write_some(s, bytes);
        if (n) {
            if (*n == 0) return std::unexpected(std::make_error_code(std::errc::broken_pipe));
            bytes = bytes.subspan(*n);
        } else if (would_block(n.error().value())) {
            auto ready = wait_ready(s, POLLWRNORM, end);
            if (!ready) return ready;
        } else return std::unexpected(n.error());
    }
    return {};
}
inline net_result<std::string> receive_exact(socket_handle s, std::size_t size, deadline end) {
    if (size > 64 * 1024) return std::unexpected(std::make_error_code(std::errc::message_size));
    std::string result(size, '\0');
    std::size_t offset = 0;
    while (offset < size) {
        if (clock::now() >= end) return std::unexpected(std::make_error_code(std::errc::timed_out));
        auto n = read_some(s, std::span{result}.subspan(offset));
        if (n) {
            if (*n == 0) return std::unexpected(std::make_error_code(std::errc::connection_reset));
            offset += *n;
        } else if (would_block(n.error().value())) {
            auto ready = wait_ready(s, POLLRDNORM, end);
            if (!ready) return std::unexpected(ready.error());
        } else return std::unexpected(n.error());
    }
    return result;
}
inline net_result<void> shutdown_write(socket_handle s) {
#ifdef _WIN32
    constexpr int how = SD_SEND;
#else
    constexpr int how = SHUT_WR;
#endif
    if (::shutdown(s, how)) return std::unexpected(net_error(socket_error()));
    return {};
}
struct listener {
    unique_socket socket;
    std::uint16_t port = 0;
};
inline net_result<listener> listen_loopback(int family = AF_INET) {
    unique_socket s{::socket(family, SOCK_STREAM, IPPROTO_TCP)};
    if (!s) return std::unexpected(net_error(socket_error()));
    sockaddr_storage address{};
    socket_length length = 0;
    if (family == AF_INET) {
        auto& a = reinterpret_cast<sockaddr_in&>(address);
        a.sin_family = AF_INET; a.sin_addr.s_addr = htonl(INADDR_LOOPBACK); length = sizeof(a);
    } else if (family == AF_INET6) {
        auto& a = reinterpret_cast<sockaddr_in6&>(address);
        a.sin6_family = AF_INET6; a.sin6_addr = in6addr_loopback; length = sizeof(a);
    } else return std::unexpected(std::make_error_code(std::errc::address_family_not_supported));
    if (::bind(s.get(), reinterpret_cast<sockaddr*>(&address), length) || ::listen(s.get(), 16))
        return std::unexpected(net_error(socket_error()));
    if (::getsockname(s.get(), reinterpret_cast<sockaddr*>(&address), &length))
        return std::unexpected(net_error(socket_error()));
    const auto port = family == AF_INET ? ntohs(reinterpret_cast<sockaddr_in&>(address).sin_port)
                                       : ntohs(reinterpret_cast<sockaddr_in6&>(address).sin6_port);
    if (auto r = nonblocking(s.get()); !r) return std::unexpected(r.error());
    return listener{std::move(s), port};
}
inline net_result<unique_socket> accept_socket(socket_handle listening) {
    unique_socket peer{::accept(listening, nullptr, nullptr)};
    if (!peer) return std::unexpected(net_error(socket_error()));
    if (auto r = nonblocking(peer.get()); !r) return std::unexpected(r.error());
    return peer;
}
inline net_result<unique_socket> connect_address(const sockaddr* address, socket_length length, deadline end) {
    unique_socket s{::socket(address->sa_family, SOCK_STREAM, IPPROTO_TCP)};
    if (!s) return std::unexpected(net_error(socket_error()));
    if (auto r = nonblocking(s.get()); !r) return std::unexpected(r.error());
    if (::connect(s.get(), address, length)) {
        const int e = socket_error();
        if (!connecting(e)) return std::unexpected(net_error(e));
        if (auto r = wait_ready(s.get(), POLLWRNORM, end); !r) return std::unexpected(r.error());
        int error = 0;
        socket_length error_size = sizeof(error);
        if (::getsockopt(s.get(), SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &error_size))
            return std::unexpected(net_error(socket_error()));
        if (error) return std::unexpected(net_error(error));
    }
    return s;
}
inline net_result<unique_socket> connect_loopback(std::uint16_t port, deadline end, int family = AF_INET) {
    if (family == AF_INET6) {
        sockaddr_in6 address{}; address.sin6_family = AF_INET6;
        address.sin6_addr = in6addr_loopback; address.sin6_port = htons(port);
        return connect_address(reinterpret_cast<sockaddr*>(&address), sizeof(address), end);
    }
    sockaddr_in address{}; address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK); address.sin_port = htons(port);
    return connect_address(reinterpret_cast<sockaddr*>(&address), sizeof(address), end);
}
} // namespace c11
