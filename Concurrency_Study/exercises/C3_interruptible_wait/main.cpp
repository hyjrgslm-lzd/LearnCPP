// =====================================================================
// 练习 C-3：可中断的条件等待（condition_variable_any + stop_token）
//   对应文档：Concurrency_Study/04-模块C-条件变量.md 的 练习 C-3
//
//   学习目标：
//     - 掌握 std::condition_variable_any 带 std::stop_token 的可中断
//       wait 重载（C++20）：cv.wait(lk, stoken, pred)；
//     - 理解它相对“close 队列”那套“在谓词里塞一个布尔标志”做法的优势：
//       取消由标准库 + stop_callback 驱动，request_stop 会自动唤醒等待者；
//     - 对比 C2 的“关闭队列”与本题的“stop_token 取消”两种唤醒退出方式。
//
//   关键事实：
//     - std::condition_variable（配普通 mutex，C++11）没有 stop_token 重载；
//       带 stop_token 的可中断 wait 是 std::condition_variable_any 的能力，
//       且该重载是 C++20 新增的。
//     - condition_variable_any 可配任意满足 BasicLockable 的锁；这里用
//       std::mutex + std::unique_lock 即可。
//
//   官方参考：
//     - https://en.cppreference.com/w/cpp/thread/condition_variable_any
//     - https://en.cppreference.com/w/cpp/thread/condition_variable_any/wait
//       （“Predicate + stop_token” 重载，C++20）
//     - https://en.cppreference.com/w/cpp/thread/stop_token
//     - 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 4 章
//
//   编译运行（VS2026, C++20）：
//     cmake --build build-vs2026 --target C3_interruptible_wait --config Release
// =====================================================================
#include "concurrency_study/log.hpp"

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>
#include <stop_token>
#include <thread>

// =====================================================================
// 一个只装单值的“信箱”，演示 condition_variable_any + stop_token。
//   消费者用可中断 wait 等待信箱非空；主线程既可以投递（正常唤醒），
//   也可以 request_stop（取消唤醒）。
// =====================================================================
class Mailbox {
public:
    void put(int value) {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            slot_ = value;
        }
        cv_.notify_one(); // 信箱多了内容，唤醒一个等待者
    }

    // -----------------------------------------------------------------
    // 必做 1：可中断等待。
    //   返回有值 → 正常取到数据；返回 nullopt → 收到 stop 请求而退出。
    //
    //   cv.wait(lk, stoken, pred) 的语义（C++20）：
    //     等价于 while (!stoken.stop_requested() && !pred()) wait(lk);
    //     并在 stop_requested() 为真时返回 pred() 的值（通常为 false）。
    //   request_stop() 会通过内部 stop_callback 自动 notify，把卡在 wait
    //   上的线程唤醒——无需我们手动 notify。
    // -----------------------------------------------------------------
    std::optional<int> take(std::stop_token stoken) {
        std::unique_lock<std::mutex> lk(mtx_);

        // TODO [必做 1]: 用带 stop_token 的可中断 wait 等待 slot_ 有值。
        //   要写：
        //     bool got = cv_.wait(lk, stoken, [&]{ return slot_.has_value(); });
        //   wait 返回后：got==true 表示谓词满足（有数据）；
        //              got==false 表示是因 stop 请求而醒来。
        const bool got =
            cv_.wait(lk, stoken, [&] { return slot_.has_value(); });

        if (!got) {
            // 因取消而退出：此时谓词不满足（信箱仍空）。
            return std::nullopt;
        }

        int value = *slot_;
        slot_.reset();
        return value;
    }

private:
    std::mutex mtx_;
    std::condition_variable_any cv_; // 注意：_any 才支持 stop_token 重载
    std::optional<int> slot_;
};

// =====================================================================
// 必做 1：worker 用 cv.wait(lk, stoken, pred)，主线程 request_stop 唤醒退出。
// =====================================================================
void demo_interruptible() {
    cs::println("======== 必做 1：condition_variable_any + stop_token ========");
    Mailbox box;

    // jthread 自带 stop_source，并把 stop_token 作为首参传给可调用对象，
    // 析构时自动 request_stop + join —— 与本题取消语义天然契合。
    std::jthread worker([&box](std::stop_token st) {
        cs::logf("[worker] 启动，循环等待信箱（可被 stop 中断）…");
        while (true) {
            auto msg = box.take(st);
            if (!msg) {
                cs::logf("[worker] take 因 stop 请求返回 nullopt，退出循环。");
                break;
            }
            cs::logf("[worker] 取到消息：", *msg);
        }
        cs::logf("[worker] 已优雅退出。");
    });

    // 先正常投递两条，观察“正常唤醒”路径。
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    box.put(42);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    box.put(7);

    // 再让 worker 进入空等待，然后请求停止，观察“取消唤醒”路径。
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    cs::logf("[main] request_stop —— 应自动唤醒卡在 wait 上的 worker。");

    // TODO [必做 1]: 请求停止。jthread 析构也会自动 request_stop，
    //   这里显式调用是为了让“取消唤醒”发生在我们观察得到的时刻。
    worker.request_stop();

    // jthread 析构自动 join。
    worker.join();
    cs::println("");
}

// =====================================================================
// 进阶 1：对比两种“唤醒退出”方式。
//   方式 A（C2 风格）：在谓词里塞一个 closed_ 布尔，close() 时置位并
//                      notify_all —— 退出条件由“业务状态”携带。
//   方式 B（本题风格）：用 stop_token，request_stop() 由标准库自动唤醒 ——
//                      退出条件由“取消通道”携带，与业务谓词正交。
//   这里用方式 A 复刻一个最小可关闭队列，与上面的 stop_token 版对照。
// =====================================================================
class ClosableQueue {
public:
    void push(int v) {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            q_.push(v);
        }
        cv_.notify_one();
    }
    void close() {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            closed_ = true;
        }
        cv_.notify_all(); // 广播：唤醒全部，让它们重查 closed_
    }
    // 退出条件“closed_”塞进了普通条件变量的谓词里（无 stop_token）。
    std::optional<int> pop() {
        std::unique_lock<std::mutex> lk(mtx_);
        cv_.wait(lk, [&] { return !q_.empty() || closed_; });
        if (q_.empty()) return std::nullopt; // closed_ 且排空
        int v = q_.front();
        q_.pop();
        return v;
    }

private:
    std::mutex mtx_;
    std::condition_variable cv_; // 普通 cv 即可：退出靠业务标志，不靠 stop_token
    std::queue<int> q_;
    bool closed_ = false;
};

void demo_close_vs_stop() {
    cs::println("======== 进阶 1：close() 标志 vs stop_token 取消 对比 ========");
    ClosableQueue q;

    // TODO [进阶 1]: 跑通方式 A，并在文档复盘问题里写出两者取舍：
    //   - close() 标志：退出原因与业务状态耦合，适合“数据流结束”语义；
    //   - stop_token：退出原因独立于业务，可由上层统一取消（jthread 析构
    //     即自动触发），且能跨多个等待点复用同一个 stop_source。
    std::jthread consumer([&q] {
        while (auto v = q.pop()) {
            cs::logf("[closable-consumer] pop ", *v);
        }
        cs::logf("[closable-consumer] 检测到 closed 且排空，退出。");
    });

    q.push(1);
    q.push(2);
    q.push(3);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    cs::logf("[main] close() 队列（方式 A：业务标志 + notify_all）。");
    q.close();

    consumer.join();
    cs::println("");
}

int main() {
    cs::println("==== C3_interruptible_wait：可中断的条件等待 ====\n");

    demo_interruptible();   // 必做 1：condition_variable_any + stop_token
    demo_close_vs_stop();   // 进阶 1：close 标志 vs stop_token 取消

    cs::println("==== 演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
