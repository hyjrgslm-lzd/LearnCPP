#include <liburing.h>
#include <coroutine_study/exercise_check.hpp>

#include <array>
#include <cerrno>
#include <coroutine>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <utility>

struct uring_loop {
    struct awaiter_base {
        std::coroutine_handle<> continuation{};
        int result = 0;
    };

    io_uring ring{};
    int pending = 0;

    uring_loop()
    {
        const int rc = io_uring_queue_init(8, &ring, 0);
        coroutine_study::check(rc == 0, "io_uring_queue_init failed");
    }
    ~uring_loop() { io_uring_queue_exit(&ring); }

    void run()
    {
        while (pending > 0) {
            io_uring_cqe* cqe = nullptr;
            __kernel_timespec timeout{.tv_sec = 5, .tv_nsec = 0};
            const int rc = io_uring_wait_cqe_timeout(&ring, &cqe, &timeout);
            if (rc == -ETIME) {
                throw std::runtime_error("io_uring timed out waiting for completion");
            }
            if (rc < 0) {
                throw std::runtime_error(std::string{"io_uring_wait_cqe_timeout failed: "} + std::strerror(-rc));
            }
            coroutine_study::check(cqe != nullptr, "io_uring returned null CQE");

            auto* base = static_cast<awaiter_base*>(io_uring_cqe_get_data(cqe));
            base->result = cqe->res;
            io_uring_cqe_seen(&ring, cqe);
            --pending;
            base->continuation.resume();
        }
    }
};

struct read_awaiter : uring_loop::awaiter_base {
    uring_loop& loop;
    int fd;
    void* data;
    unsigned size;
    off_t offset;

    read_awaiter(uring_loop& l, int file, void* d, unsigned n, off_t off) noexcept
        : loop(l), fd(file), data(d), size(n), offset(off)
    {
    }

    bool await_ready() const noexcept { return false; }

    bool await_suspend(std::coroutine_handle<> h) noexcept
    {
        continuation = h;
        auto* sqe = io_uring_get_sqe(&loop.ring);
        if (!sqe) {
            result = -ENOMEM;
            return false;
        }
        io_uring_prep_read(sqe, fd, data, size, offset);
        io_uring_sqe_set_data(sqe, static_cast<awaiter_base*>(this));
        const int submitted = io_uring_submit(&loop.ring);
        if (submitted < 1) {
            result = submitted < 0 ? submitted : -EIO;
            return false;
        }
        ++loop.pending;
        return true;
    }

    int await_resume() const noexcept { return result; }
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

task read_once(uring_loop& loop, int fd, std::string& out)
{
    std::array<char, 64> buf{};
    int n = co_await read_awaiter{loop, fd, buf.data(), static_cast<unsigned>(buf.size()), 0};
    coroutine_study::check(n > 0, "io_uring read failed");
    out.assign(buf.data(), static_cast<size_t>(n));
}

int main()
{
    auto path = std::filesystem::temp_directory_path() / "coroutine_i2_io_uring.txt";
    constexpr std::string_view payload = "io_uring-loopback";
    {
        int fd = ::open(path.string().c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0600);
        coroutine_study::check(fd >= 0, "temp file open for write failed");
        coroutine_study::check(::write(fd, payload.data(), payload.size()) == static_cast<ssize_t>(payload.size()), "temp file write failed");
        ::close(fd);
    }

    int fd = ::open(path.string().c_str(), O_RDONLY);
    coroutine_study::check(fd >= 0, "temp file open for read failed");

    uring_loop loop;
    std::string received;
    auto t = read_once(loop, fd, received);
    loop.run();
    ::close(fd);
    std::filesystem::remove(path);

    coroutine_study::check(received == payload, "io_uring payload mismatch");
    std::cout << "I2 Linux reference passed: CQE user_data resumed coroutine\n";
}
