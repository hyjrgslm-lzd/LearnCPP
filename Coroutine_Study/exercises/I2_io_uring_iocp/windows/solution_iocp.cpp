#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <coroutine_study/exercise_check.hpp>

#include <array>
#include <coroutine>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

#pragma comment(lib, "ws2_32.lib")

struct wsa_guard {
    wsa_guard()
    {
        WSADATA data{};
        coroutine_study::check(WSAStartup(MAKEWORD(2, 2), &data) == 0, "WSAStartup failed");
    }
    ~wsa_guard() { WSACleanup(); }
};

struct socket_guard {
    SOCKET s = INVALID_SOCKET;
    explicit socket_guard(SOCKET socket = INVALID_SOCKET) noexcept : s(socket) {}
    ~socket_guard()
    {
        if (s != INVALID_SOCKET) {
            closesocket(s);
        }
    }
    socket_guard(const socket_guard&) = delete;
    socket_guard& operator=(const socket_guard&) = delete;
};

struct iocp_loop {
    struct awaiter_base {
        OVERLAPPED ov{};
        std::coroutine_handle<> continuation{};
        DWORD transferred = 0;
        DWORD error = 0;
        BOOL ok = FALSE;
        bool posted_failure = false;
    };

    HANDLE port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    int pending = 0;

    iocp_loop()
    {
        coroutine_study::check(port != nullptr, "CreateIoCompletionPort failed");
    }

    ~iocp_loop()
    {
        if (port) {
            CloseHandle(port);
        }
    }

    void bind(SOCKET s)
    {
        auto handle = reinterpret_cast<HANDLE>(s);
        coroutine_study::check(CreateIoCompletionPort(handle, port, 0, 0) == port, "CreateIoCompletionPort bind failed");
    }

    void run()
    {
        while (pending > 0) {
            DWORD bytes = 0;
            ULONG_PTR key = 0;
            OVERLAPPED* ov = nullptr;
            BOOL ok = GetQueuedCompletionStatus(port, &bytes, &key, &ov, 5000);
            if (!ov) {
                throw std::runtime_error(ok ? "IOCP returned null OVERLAPPED" : "IOCP timed out or failed with null OVERLAPPED");
            }

            auto* base = reinterpret_cast<awaiter_base*>(ov);
            if (!base->posted_failure) {
                base->ok = ok;
                base->transferred = bytes;
                base->error = ok ? 0 : GetLastError();
            }
            --pending;
            base->continuation.resume();
        }
    }
};

static_assert(offsetof(iocp_loop::awaiter_base, ov) == 0);

struct recv_awaiter : iocp_loop::awaiter_base {
    iocp_loop& loop;
    SOCKET socket;
    char* data;
    DWORD size;

    recv_awaiter(iocp_loop& l, SOCKET s, char* d, DWORD n) noexcept
        : loop(l), socket(s), data(d), size(n)
    {
    }

    bool await_ready() const noexcept { return false; }

    bool await_suspend(std::coroutine_handle<> h) noexcept
    {
        continuation = h;
        WSABUF buf{size, data};
        DWORD flags = 0;
        const int rc = WSARecv(socket, &buf, 1, nullptr, &flags, &ov, nullptr);
        if (rc == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
            error = WSAGetLastError();
            posted_failure = true;
            ok = FALSE;
            return false;
        }
        ++loop.pending;
        return true;
    }

    int await_resume() const noexcept
    {
        return ok ? static_cast<int>(transferred) : -static_cast<int>(error);
    }
};

struct task {
    struct promise_type {
        task get_return_object() noexcept
        {
            return task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() noexcept { std::terminate(); }
    };

    std::coroutine_handle<promise_type> h{};
    explicit task(std::coroutine_handle<promise_type> handle) noexcept : h(handle) {}
    task(task&& other) noexcept : h(std::exchange(other.h, {})) {}
    ~task()
    {
        if (h) {
            h.destroy();
        }
    }
};

task recv_once(iocp_loop& loop, SOCKET s, std::string& out)
{
    std::array<char, 64> buf{};
    int n = co_await recv_awaiter{loop, s, buf.data(), static_cast<DWORD>(buf.size())};
    coroutine_study::check(n > 0, "IOCP recv failed");
    out.assign(buf.data(), static_cast<size_t>(n));
}

int main()
{
    wsa_guard wsa;
    socket_guard listener{WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED)};
    coroutine_study::check(listener.s != INVALID_SOCKET, "WSASocket listener failed");

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    coroutine_study::check(bind(listener.s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0, "bind listener failed");
    coroutine_study::check(listen(listener.s, 1) == 0, "listen failed");

    int len = sizeof(addr);
    coroutine_study::check(getsockname(listener.s, reinterpret_cast<sockaddr*>(&addr), &len) == 0, "getsockname failed");
    const auto port = ntohs(addr.sin_port);

    std::exception_ptr client_error;
    std::thread client([port, &client_error] {
        try {
            socket_guard s{socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)};
            coroutine_study::check(s.s != INVALID_SOCKET, "client socket failed");
            sockaddr_in peer{};
            peer.sin_family = AF_INET;
            peer.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            peer.sin_port = htons(port);
            coroutine_study::check(connect(s.s, reinterpret_cast<sockaddr*>(&peer), sizeof(peer)) == 0, "client connect failed");
            const char payload[] = "iocp-loopback";
            coroutine_study::check(send(s.s, payload, static_cast<int>(std::strlen(payload)), 0) > 0, "client send failed");
        } catch (...) {
            client_error = std::current_exception();
        }
    });

    socket_guard server{accept(listener.s, nullptr, nullptr)};
    coroutine_study::check(server.s != INVALID_SOCKET, "accept failed");

    iocp_loop loop;
    loop.bind(server.s);
    std::string received;
    auto t = recv_once(loop, server.s, received);
    loop.run();
    client.join();
    if (client_error) {
        std::rethrow_exception(client_error);
    }

    coroutine_study::check(received == "iocp-loopback", "IOCP payload mismatch");
    std::cout << "I2 Windows reference passed: IOCP resumed coroutine with loopback bytes\n";
}
