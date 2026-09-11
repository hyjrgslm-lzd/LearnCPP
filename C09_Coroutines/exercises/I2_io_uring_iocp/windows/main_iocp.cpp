#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>

#include <coroutine>
#include <iostream>

struct iocp_loop {
    HANDLE port = nullptr;
    int pending = 0;

    struct recv_awaiter {
        OVERLAPPED ov{};
        iocp_loop* loop = nullptr;
        SOCKET socket = INVALID_SOCKET;
        char* data = nullptr;
        DWORD size = 0;
        std::coroutine_handle<> continuation{};
        DWORD transferred = 0;
        DWORD error = 0;

        bool await_ready() const noexcept { return true; }
        bool await_suspend(std::coroutine_handle<> h) noexcept
        {
            continuation = h;
            // TODO: call WSARecv on an overlapped socket, then return true only
            // after a request is actually pending on loop->port.
            return false;
        }
        int await_resume() const noexcept
        {
            // TODO: return byte count or -WSA error after GetQueuedCompletionStatus fills state.
            return static_cast<int>(transferred) - static_cast<int>(error);
        }
    };

    void bind(SOCKET s)
    {
        (void)s;
        // TODO: CreateIoCompletionPort(reinterpret_cast<HANDLE>(s), port, ...).
    }

    void run_once()
    {
        // TODO: GetQueuedCompletionStatus with a finite timeout, restore recv_awaiter from OVERLAPPED*.
    }
};

int main()
{
    iocp_loop::recv_awaiter awaiter;
    if (awaiter.await_ready()) {
        std::cout << "student check failed: recv_awaiter must suspend until IOCP completion arrives\n";
        return 1;
    }
    std::cout << "I2 Windows starter structural check passed.\n";
}
