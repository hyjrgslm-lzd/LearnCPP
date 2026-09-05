#include <liburing.h>

#include <coroutine>
#include <iostream>

struct uring_loop {
    io_uring ring{};
    int pending = 0;

    struct read_awaiter {
        uring_loop* loop = nullptr;
        int fd = -1;
        void* data = nullptr;
        unsigned size = 0;
        off_t offset = 0;
        std::coroutine_handle<> continuation{};
        int result = 0;

        bool await_ready() const noexcept { return true; }
        bool await_suspend(std::coroutine_handle<> h) noexcept
        {
            continuation = h;
            // TODO: get SQE, io_uring_prep_read, io_uring_sqe_set_data(this),
            // submit, then increment pending only if submit succeeds.
            return false;
        }
        int await_resume() const noexcept
        {
            // TODO: return cqe->res copied by the loop after io_uring_cqe_seen().
            return result;
        }
    };

    void run_once()
    {
        // TODO: io_uring_wait_cqe_timeout, get user_data, cqe_seen, resume exactly once.
    }
};

int main()
{
    (void)sizeof(uring_loop::read_awaiter);
    std::cout << "I2 Linux starter skeleton compiled. It posts no IO yet.\n";
}
