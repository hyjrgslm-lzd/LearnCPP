// I-2 (Linux) io_uring awaiter
// 文档参考：11-模块I-真实异步IO与并发框架.md 「练习 I-2」
// 官方参考：
//   - liburing 仓库：https://github.com/axboe/liburing
//   - Jens Axboe "Efficient IO with io_uring"
//   - lordoftheio_uring 教程：https://unixism.net/loti/
//
// 目标：为 Linux io_uring 写最小 awaiter，把 SQE/CQE 的 completion 翻译为
//      coroutine_handle::resume()。awaiter 在 await_suspend 中投递 SQE 并
//      用 io_uring_sqe_set_data(this) 把自身指针编码进 user_data；
//      事件循环在 CQE 里取出 user_data，找回 awaiter，设置 cqe_，再 resume 协程。

#include <coroutine>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <utility>

#include <liburing.h>

// ============ uring_loop：单线程事件循环 ============
struct uring_loop {
    io_uring ring{};
    bool running = true;
    int  pending = 0;   // 投递但未完成的请求计数

    explicit uring_loop(unsigned entries = 256) {
        if (io_uring_queue_init(entries, &ring, 0) < 0) {
            std::perror("io_uring_queue_init");
            std::abort();
        }
    }
    ~uring_loop() { io_uring_queue_exit(&ring); }
    uring_loop(const uring_loop&) = delete;
    uring_loop& operator=(const uring_loop&) = delete;

    // 主循环：取出 CQE → 还原 awaiter → resume 协程
    void run() {
        while (running && pending > 0) {
            io_uring_cqe* cqe = nullptr;
            int rc = io_uring_wait_cqe(&ring, &cqe);
            if (rc < 0) {
                std::fprintf(stderr, "io_uring_wait_cqe failed: %s\n",
                             std::strerror(-rc));
                continue;
            }
            // 任意 awaiter 都至少有 cqe_ + coro_ 两个字段
            // 这里用 base 抽取
            auto* base = static_cast<awaiter_base*>(io_uring_cqe_get_data(cqe));
            base->cqe_res_ = cqe->res;
            io_uring_cqe_seen(&ring, cqe);
            --pending;
            base->coro_.resume();
        }
    }

    void stop() { running = false; }

    // ============ awaiter 基类（提供 cqe_res_ 与 coro_）============
    struct awaiter_base {
        std::coroutine_handle<> coro_;
        int cqe_res_ = 0;
    };
};

// ============ uring_read_awaiter：read 操作的 awaiter ============
struct uring_read_awaiter : uring_loop::awaiter_base {
    uring_loop* loop_;
    int     fd_;
    void*   buf_;
    unsigned nbytes_;
    off_t   offset_;

    uring_read_awaiter(uring_loop* loop, int fd, void* buf,
                       unsigned nbytes, off_t offset) noexcept
        : loop_(loop), fd_(fd), buf_(buf), nbytes_(nbytes), offset_(offset) {}

    bool await_ready() noexcept { return false; }

    void await_suspend(std::coroutine_handle<> h) noexcept {
        coro_ = h;
        auto* sqe = io_uring_get_sqe(&loop_->ring);
        io_uring_prep_read(sqe, fd_, buf_, nbytes_, offset_);
        // 关键：sqe_set_data(this) —— this 指针随 CQE 原样返回，
        //      事件循环可以用它找回本 awaiter 并 resume 协程
        io_uring_sqe_set_data(sqe, static_cast<awaiter_base*>(this));
        io_uring_submit(&loop_->ring);
        ++loop_->pending;
    }

    int await_resume() noexcept {
        // cqe->res：>=0 表示读取字节数；<0 表示 -errno
        return cqe_res_;
    }
};

// ============ 用户协程 ============
struct read_file_task {
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    struct promise_type {
        // 关键：把协程帧 handle 交给 read_file_task 持有，否则 final_suspend 为
        // suspend_always 时谁都不 destroy()，协程帧泄漏。
        read_file_task get_return_object() noexcept {
            return read_file_task{handle_type::from_promise(*this)};
        }
        std::suspend_never  initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend()   noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() noexcept { std::terminate(); }
    };

    handle_type h_{};
    explicit read_file_task(handle_type h) noexcept : h_(h) {}
    read_file_task(read_file_task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    read_file_task& operator=(read_file_task&& o) noexcept {
        if (this != &o) { if (h_) h_.destroy(); h_ = std::exchange(o.h_, {}); }
        return *this;
    }
    read_file_task(const read_file_task&) = delete;
    read_file_task& operator=(const read_file_task&) = delete;
    ~read_file_task() { if (h_) h_.destroy(); }
};

read_file_task read_and_print(uring_loop& loop, const char* path) {
    int fd = ::open(path, O_RDONLY);
    if (fd < 0) {
        std::perror("open");
        co_return;
    }
    char buf[4096];
    int n = co_await uring_read_awaiter{&loop, fd, buf, sizeof(buf), 0};
    if (n > 0) {
        ::write(STDOUT_FILENO, buf, static_cast<size_t>(n));
    } else if (n < 0) {
        std::fprintf(stderr, "[read] errno=%d (%s)\n", -n, std::strerror(-n));
    }
    ::close(fd);
    co_return;
}

int main(int argc, char* argv[])
{
    const char* path = (argc > 1) ? argv[1] : "/etc/hostname";
    std::printf("===== I-2 (Linux) io_uring awaiter — reading %s =====\n", path);

    uring_loop loop;
    auto task = read_and_print(loop, path);  // 持有协程帧；它会立即挂起在 co_await
    loop.run();                               // 事件循环驱动 CQE → resume
    // task 在 main 结束时析构 → 协程帧在 final_suspend 处被 destroy()，无泄漏

    // TODO [必做]：在笔记中画 kernel 队列与协程帧的协作时间线
    //   sqe_set_data(this) → SQE 入 SQ → kernel 处理 → CQE 入 CQ →
    //   wait_cqe → user_data → awaiter → cqe_seen → coro.resume()
    // TODO [进阶]：用 io_uring_prep_timeout + IORING_OP_LINK_TIMEOUT 实现超时。
    // TODO [进阶]：实现 SQ polling 模式（IORING_SETUP_SQPOLL），对比延迟。

    std::printf("\n===== Done =====\n");
    return 0;
}
