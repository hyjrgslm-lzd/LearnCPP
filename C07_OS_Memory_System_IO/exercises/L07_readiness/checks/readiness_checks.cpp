#include <readiness.hpp>
#include <check.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace {

#ifdef _WIN32
class wsa_runtime {
public:
    wsa_runtime() {
        WSADATA data{};
        const int rc = ::WSAStartup(MAKEWORD(2, 2), &data);
        check(rc == 0, "WSAStartup succeeds");
    }
    ~wsa_runtime() { ::WSACleanup(); }
    wsa_runtime(const wsa_runtime&) = delete;
    wsa_runtime& operator=(const wsa_runtime&) = delete;
};

class socket_handle {
public:
    socket_handle() = default;
    explicit socket_handle(SOCKET s) : s_(s) {}
    ~socket_handle() { reset(); }
    socket_handle(const socket_handle&) = delete;
    socket_handle& operator=(const socket_handle&) = delete;
    socket_handle(socket_handle&& other) noexcept : s_(std::exchange(other.s_, INVALID_SOCKET)) {}
    socket_handle& operator=(socket_handle&& other) noexcept {
        if (this != &other) {
            reset();
            s_ = std::exchange(other.s_, INVALID_SOCKET);
        }
        return *this;
    }
    SOCKET get() const { return s_; }
    SOCKET release() { return std::exchange(s_, INVALID_SOCKET); }
    void reset(SOCKET next = INVALID_SOCKET) {
        if (s_ != INVALID_SOCKET) ::closesocket(s_);
        s_ = next;
    }
private:
    SOCKET s_ = INVALID_SOCKET;
};

void require_socket(bool ok, const char* message) {
    check(ok, message);
}

void set_nonblocking(SOCKET s) {
    u_long one = 1;
    require_socket(::ioctlsocket(s, FIONBIO, &one) == 0, "ioctlsocket enables nonblocking mode");
}

void set_small_buffers(SOCKET s) {
    const int size = 4096;
    (void)::setsockopt(s, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&size), sizeof(size));
    (void)::setsockopt(s, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&size), sizeof(size));
}

struct socket_pair {
    socket_handle reader;
    socket_handle writer;
};

socket_pair make_pair() {
    socket_handle listener(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
    require_socket(listener.get() != INVALID_SOCKET, "loopback listener socket created");
    set_small_buffers(listener.get());

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ::htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    require_socket(::bind(listener.get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0,
        "loopback listener binds 127.0.0.1 ephemeral port");
    require_socket(::listen(listener.get(), 1) == 0, "loopback listener starts listening");

    int len = sizeof(addr);
    require_socket(::getsockname(listener.get(), reinterpret_cast<sockaddr*>(&addr), &len) == 0,
        "loopback listener exposes chosen port");

    socket_handle client(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
    require_socket(client.get() != INVALID_SOCKET, "loopback client socket created");
    set_small_buffers(client.get());
    require_socket(::connect(client.get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0,
        "loopback client connects locally");

    socket_handle server(::accept(listener.get(), nullptr, nullptr));
    require_socket(server.get() != INVALID_SOCKET, "loopback server accepts local client");
    set_small_buffers(server.get());
    set_nonblocking(server.get());
    return socket_pair{std::move(server), std::move(client)};
}

void send_all(SOCKET s, std::string_view data) {
    while (!data.empty()) {
        const int chunk = static_cast<int>(std::min<std::size_t>(data.size(), 4096));
        const int n = ::send(s, data.data(), chunk, 0);
        require_socket(n > 0, "loopback send makes progress");
        data.remove_prefix(static_cast<std::size_t>(n));
    }
}

bool poll_readable(SOCKET s, int timeout_ms) {
    WSAPOLLFD fd{};
    fd.fd = s;
    fd.events = POLLRDNORM;
    const int n = ::WSAPoll(&fd, 1, timeout_ms);
    require_socket(n >= 0, "WSAPoll returns a status");
    return n > 0 && (fd.revents & (POLLRDNORM | POLLIN | POLLHUP | POLLERR)) != 0;
}

#else
class fd_handle {
public:
    fd_handle() = default;
    explicit fd_handle(int fd) : fd_(fd) {}
    ~fd_handle() { reset(); }
    fd_handle(const fd_handle&) = delete;
    fd_handle& operator=(const fd_handle&) = delete;
    fd_handle(fd_handle&& other) noexcept : fd_(std::exchange(other.fd_, -1)) {}
    fd_handle& operator=(fd_handle&& other) noexcept {
        if (this != &other) {
            reset();
            fd_ = std::exchange(other.fd_, -1);
        }
        return *this;
    }
    int get() const { return fd_; }
    int release() { return std::exchange(fd_, -1); }
    void reset(int next = -1) {
        if (fd_ != -1) (void)::close(fd_);
        fd_ = next;
    }
private:
    int fd_ = -1;
};

void require_sys(bool ok, const char* message) {
    check(ok, message);
}

void set_nonblocking(int fd) {
    const int flags = ::fcntl(fd, F_GETFL, 0);
    require_sys(flags != -1, "fcntl reads descriptor flags");
    require_sys(::fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1, "fcntl enables nonblocking mode");
}

struct socket_pair {
    fd_handle reader;
    fd_handle writer;
};

socket_pair make_pair() {
    int fds[2] = {-1, -1};
#ifdef SOCK_CLOEXEC
    int rc = ::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, fds);
    if (rc == -1 && errno == EINVAL) rc = ::socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
#else
    const int rc = ::socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
#endif
    require_sys(rc == 0, "socketpair creates local connected endpoints");
    fd_handle reader(fds[0]);
    fd_handle writer(fds[1]);
    set_nonblocking(reader.get());
    return socket_pair{std::move(reader), std::move(writer)};
}

void send_all(int fd, std::string_view data) {
    while (!data.empty()) {
        const auto chunk = std::min<std::size_t>(data.size(), 4096);
#ifdef MSG_NOSIGNAL
        const ssize_t n = ::send(fd, data.data(), chunk, MSG_NOSIGNAL);
#else
        const ssize_t n = ::send(fd, data.data(), chunk, 0);
#endif
        require_sys(n > 0, "socketpair send makes progress");
        data.remove_prefix(static_cast<std::size_t>(n));
    }
}

bool poll_readable(int fd, int timeout_ms) {
    pollfd pfd{};
    pfd.fd = fd;
    pfd.events = POLLIN;
    const int n = ::poll(&pfd, 1, timeout_ms);
    require_sys(n >= 0, "poll returns a status");
    return n > 0 && (pfd.revents & (POLLIN | POLLHUP | POLLERR)) != 0;
}
#endif

std::uint64_t runtime_seed() {
    const auto now = static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    const auto addr = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(&now));
    return now ^ (addr << 1);
}

std::uint64_t seed_value() {
    static const std::uint64_t seed = runtime_seed();
    return seed;
}

std::mt19937_64& rng() {
    static std::mt19937_64 engine(seed_value());
    return engine;
}

std::string make_payload(std::size_t min_size, std::size_t max_size) {
    std::uniform_int_distribution<std::size_t> size_dist(min_size, max_size);
    std::uniform_int_distribution<int> byte_dist(33, 126);
    std::string out(size_dist(rng()), '\0');
    for (char& ch : out) ch = static_cast<char>(byte_dist(rng()));
    return out;
}

void expect_drain(c07_l07::native_endpoint endpoint, std::size_t max_bytes, std::string_view expected,
    bool would_block, bool eof, bool budget, const char* label) {
    const auto result = c07_l07::drain(endpoint, max_bytes);
    check(result.has_value(), label);
    check(result->bytes == expected, "runtime payload matches drained bytes");
    check(result->would_block == would_block, label);
    check(result->eof == eof, label);
    check(result->budget_exhausted == budget, label);
}

void exercise_empty_socket() {
    auto pair = make_pair();
    expect_drain(pair.reader.get(), 128, "", true, false, false, "empty nonblocking endpoint reports would-block");
}

void exercise_multifragment_socket() {
    auto pair = make_pair();
    const std::string data = make_payload(80, 96);
    send_all(pair.writer.get(), data);
    check(poll_readable(pair.reader.get(), 1000), "readiness reports queued data before drain");
    expect_drain(pair.reader.get(), 128, data, true, false, false, "second fragment drained");
    check(!poll_readable(pair.reader.get(), 0), "drained socket has no remaining level-triggered readiness");
}

void exercise_repeated_calls() {
    auto pair = make_pair();
    const std::string first = make_payload(7, 13);
    const std::string second = make_payload(7, 13);
    send_all(pair.writer.get(), first);
    expect_drain(pair.reader.get(), 64, first, true, false, false, "first drain consumes first burst");
    send_all(pair.writer.get(), second);
    expect_drain(pair.reader.get(), 64, second, true, false, false, "second drain consumes later burst");
}

void exercise_peer_close() {
    auto pair = make_pair();
    const std::string data = make_payload(11, 20);
    send_all(pair.writer.get(), data);
#ifdef _WIN32
    require_socket(::shutdown(pair.writer.get(), SD_SEND) == 0, "writer half-closes send side");
#else
    require_sys(::shutdown(pair.writer.get(), SHUT_WR) == 0, "writer half-closes send side");
#endif
    expect_drain(pair.reader.get(), 64, data, false, true, false, "drain distinguishes queued bytes followed by EOF");
}

void exercise_budget_boundary() {
    auto pair = make_pair();
    const std::string data = make_payload(30, 40);
    send_all(pair.writer.get(), data);
    expect_drain(pair.reader.get(), 5, std::string_view(data).substr(0, 5), false, false, true,
        "finite budget stops before completion");
    expect_drain(pair.reader.get(), 64, std::string_view(data).substr(5), true, false, false,
        "later drain resumes from remaining bytes");
}

void exercise_backpressure() {
    auto pair = make_pair();
#ifdef _WIN32
    u_long one = 1;
    require_socket(::ioctlsocket(pair.writer.get(), FIONBIO, &one) == 0, "writer enters nonblocking mode for backpressure probe");
#else
    set_nonblocking(pair.writer.get());
#endif
    const std::string block = make_payload(4096, 4096);
    std::string written;
    std::size_t sent = 0;
    bool blocked = false;
    for (int i = 0; i != 4096 && !blocked; ++i) {
#ifdef _WIN32
        const int n = ::send(pair.writer.get(), block.data(), static_cast<int>(block.size()), 0);
        if (n > 0) {
            sent += static_cast<std::size_t>(n);
            written.append(block.data(), static_cast<std::size_t>(n));
        }
        else {
            const int e = ::WSAGetLastError();
            if (e == WSAEWOULDBLOCK) blocked = true;
            else require_socket(false, "nonblocking send fails only at would-block");
        }
#else
#ifdef MSG_NOSIGNAL
        const ssize_t n = ::send(pair.writer.get(), block.data(), block.size(), MSG_NOSIGNAL);
#else
        const ssize_t n = ::send(pair.writer.get(), block.data(), block.size(), 0);
#endif
        if (n > 0) {
            sent += static_cast<std::size_t>(n);
            written.append(block.data(), static_cast<std::size_t>(n));
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK) blocked = true;
        else require_sys(false, "nonblocking send fails only at would-block");
#endif
    }
    check(sent > 0, "backpressure probe fills at least one socket buffer chunk");
    check(blocked, "backpressure reaches would-block within bounded fill");
    const auto drained = c07_l07::drain(pair.reader.get(), 8192);
    check(drained.has_value(), "drain relieves a bounded amount of backpressure");
    check(!drained->bytes.empty(), "drain returns payload while relieving backpressure");
    check(drained->bytes == std::string_view(written).substr(0, drained->bytes.size()),
        "backpressure drain returns written prefix");
}

#ifndef _WIN32
void exercise_epoll_edge_triggered_needs_drain() {
    int raw[2] = {-1, -1};
    require_sys(::pipe2(raw, O_NONBLOCK | O_CLOEXEC) == 0, "pipe2 creates nonblocking pipe for epoll ET observation");
    fd_handle read_end(raw[0]);
    fd_handle write_end(raw[1]);
    fd_handle ep(::epoll_create1(EPOLL_CLOEXEC));
    require_sys(ep.get() != -1, "epoll_create1 succeeds");
    epoll_event event{};
    event.events = EPOLLIN | EPOLLET;
    event.data.u64 = 7;
    require_sys(::epoll_ctl(ep.get(), EPOLL_CTL_ADD, read_end.get(), &event) == 0, "epoll accepts nonblocking pipe in ET mode");
    require_sys(::write(write_end.get(), "abcdef", 6) == 6, "pipe receives test bytes");
    epoll_event out{};
    require_sys(::epoll_wait(ep.get(), &out, 1, 1000) == 1, "ET epoll reports first transition to readable");
    char one{};
    require_sys(::read(read_end.get(), &one, 1) == 1 && one == 'a', "counterexample intentionally reads only one byte");
    check(::epoll_wait(ep.get(), &out, 1, 0) == 0, "ET does not repeat an event just because bytes remain");
    std::array<char, 8> rest{};
    const ssize_t n = ::read(read_end.get(), rest.data(), rest.size());
    check(n == 5 && std::string_view(rest.data(), 5) == "bcdef", "bytes still existed after missed ET notification");
}

void exercise_epoll_oneshot_rearm() {
    int raw[2] = {-1, -1};
    require_sys(::pipe2(raw, O_NONBLOCK | O_CLOEXEC) == 0, "pipe2 creates nonblocking pipe for EPOLLONESHOT observation");
    fd_handle read_end(raw[0]);
    fd_handle write_end(raw[1]);
    fd_handle ep(::epoll_create1(EPOLL_CLOEXEC));
    require_sys(ep.get() != -1, "epoll_create1 succeeds for oneshot");
    epoll_event event{};
    event.events = EPOLLIN | EPOLLONESHOT;
    event.data.u64 = 9;
    require_sys(::epoll_ctl(ep.get(), EPOLL_CTL_ADD, read_end.get(), &event) == 0, "epoll accepts oneshot registration");
    require_sys(::write(write_end.get(), "ab", 2) == 2, "oneshot pipe receives first bytes");
    epoll_event out{};
    require_sys(::epoll_wait(ep.get(), &out, 1, 1000) == 1, "oneshot reports first readiness");
    std::array<char, 8> buffer{};
    require_sys(::read(read_end.get(), buffer.data(), buffer.size()) == 2, "oneshot handler drains first readiness");
    require_sys(::write(write_end.get(), "cd", 2) == 2, "oneshot pipe receives bytes while disabled");
    check(::epoll_wait(ep.get(), &out, 1, 0) == 0, "oneshot stays disabled until explicit rearm");
    require_sys(::epoll_ctl(ep.get(), EPOLL_CTL_MOD, read_end.get(), &event) == 0, "epoll_ctl MOD rearms oneshot registration");
    require_sys(::epoll_wait(ep.get(), &out, 1, 1000) == 1, "rearmed oneshot reports later bytes");
}

void exercise_regular_file_epoll_eperm() {
    const auto path = std::filesystem::temp_directory_path() / ("c07_l07_epoll_regular_file_" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".txt");
    { std::ofstream(path, std::ios::binary) << "regular files are not epoll readiness sources here\n"; }
    fd_handle file(::open(path.c_str(), O_RDONLY | O_CLOEXEC));
    require_sys(file.get() != -1, "regular file opens for EPERM observation");
    fd_handle ep(::epoll_create1(EPOLL_CLOEXEC));
    require_sys(ep.get() != -1, "epoll_create1 succeeds for regular-file observation");
    epoll_event event{};
    event.events = EPOLLIN;
    errno = 0;
    const int rc = ::epoll_ctl(ep.get(), EPOLL_CTL_ADD, file.get(), &event);
    const int saved = errno;
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
    check(rc == -1 && saved == EPERM, "epoll_ctl rejects regular file with EPERM");
}
#endif

} // namespace

int main() {
#ifdef _WIN32
    wsa_runtime wsa;
#endif
    std::cout << "L07 readiness seed=" << seed_value() << "\n";
    exercise_empty_socket();
    exercise_multifragment_socket();
    exercise_repeated_calls();
    exercise_peer_close();
    exercise_budget_boundary();
    exercise_backpressure();
#ifndef _WIN32
    exercise_epoll_edge_triggered_needs_drain();
    exercise_epoll_oneshot_rearm();
    exercise_regular_file_epoll_eperm();
#endif
    std::cout << "L07 readiness checks passed\n";
}
