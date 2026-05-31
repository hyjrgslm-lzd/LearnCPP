// =====================================================================
// 练习 C-1：条件变量与谓词等待（condition_variable + predicate）
//   对应文档：Concurrency_Study/04-模块C-条件变量.md 的 练习 C-1
//
//   学习目标：
//     - 掌握 std::condition_variable 的 wait / notify 协议；
//     - 理解“wait 为什么必须配谓词（predicate）”——防虚假唤醒
//       （spurious wakeup）与丢失唤醒（lost wakeup）；
//     - 区分 notify_one 与 notify_all 的语义；
//     - 亲手实现一次“主线程置位 ready 后通知 worker 开工”的交接
//       （handoff），并演示不带谓词的错误版本如何丢失/虚假唤醒。
//
//   官方参考：
//     - https://en.cppreference.com/w/cpp/thread/condition_variable
//     - https://en.cppreference.com/w/cpp/thread/condition_variable/wait
//     - 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 4 章
//
//   编译运行（VS2026, C++20）：
//     cmake --build build-vs2026 --target C1_condvar_predicate --config Release
// =====================================================================
#include "concurrency_study/log.hpp"

#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

// ---------------------------------------------------------------------
// 共享状态：一个标志位 + 一把互斥量 + 一个条件变量。
//   条件变量从不单独使用：它总是与一把 mutex 和一个“共享谓词状态”
//   绑定。谓词（这里是 ready）必须在持锁状态下读写，否则数据竞争。
// ---------------------------------------------------------------------
struct Handoff {
    std::mutex mtx;
    std::condition_variable cv;
    bool ready = false; // 谓词状态：主线程是否已发令开工
};

// =====================================================================
// 必做 1：用 cv + mutex + 谓词实现正确的 handoff。
//   主线程置位 ready 后通知 worker，worker 等到 ready 才开工。
// =====================================================================
void worker_correct(Handoff& h) {
    cs::logf("[correct] worker 启动，准备等待开工令…");

    // TODO [必做 1]: 用谓词版 wait 等待 ready 变为 true。
    //   要点：
    //     1) 先用 unique_lock 锁住 h.mtx（wait 需要可解锁/重锁的锁）。
    //     2) 调用 h.cv.wait(lk, [&]{ return h.ready; });
    //        谓词版等价于 while (!h.ready) h.cv.wait(lk);
    //        ——它在“被唤醒后”重新检查谓词，因此天然免疫虚假唤醒，
    //        且若通知早于 wait（谓词已为 true）则根本不会睡。
    //   下面给出最小占位（直接读取，不演示等待），保证可编译。
    //   真正应写：
    //     std::unique_lock<std::mutex> lk(h.mtx);
    //     h.cv.wait(lk, [&] { return h.ready; });
    {
        std::unique_lock<std::mutex> lk(h.mtx);
        // 占位：用谓词版 wait（已是正确写法，留作必做 1 的参考实现）。
        h.cv.wait(lk, [&] { return h.ready; });
    }

    cs::logf("[correct] worker 收到开工令，开始干活。");
}

void demo_correct() {
    cs::println("================ 必做 1：正确的谓词等待 ================");
    Handoff h;

    std::thread t(worker_correct, std::ref(h));

    // 故意晚一点发令，让 worker 先进入 wait（演示“正常唤醒”路径）。
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 发令：持锁修改谓词，解锁后通知。
    {
        std::lock_guard<std::mutex> lk(h.mtx);
        h.ready = true;
        cs::logf("[correct] 主线程置位 ready=true。");
    }
    // notify_one 唤醒（至多）一个等待者。这里只有一个 worker，足够。
    h.cv.notify_one();

    t.join();
    cs::println("");
}

// =====================================================================
// 必做 2：演示“不带谓词”的错误版本如何丢失唤醒（lost wakeup）。
//   错误点：worker 无条件 wait，而主线程的 notify 可能发生在 worker
//   真正进入 wait 之前——那一次 notify 就被永久丢失，worker 卡死。
//   为了让程序仍能结束（不卡死整个测试），这里用一个“带超时的等待”
//   把丢失唤醒变成可观测的超时，而不是真的死锁。
// =====================================================================
void worker_buggy_no_predicate(Handoff& h) {
    cs::logf("[buggy] worker 启动（无谓词，易丢失唤醒）…");

    std::unique_lock<std::mutex> lk(h.mtx);
    // TODO [必做 2]: 这是“反面教材”。无谓词的裸 wait：
    //     h.cv.wait(lk);   // 危险：notify 早于 wait 则永久丢失
    //   用 wait_for 带超时把“丢失唤醒”显式暴露成超时，便于观察：
    auto woke = h.cv.wait_for(lk, std::chrono::milliseconds(300));
    if (woke == std::cv_status::timeout) {
        cs::logf("[buggy] 超时返回：notify 早于 wait → 丢失唤醒！"
                 "（注意 ready=", h.ready, "，其实早就 true 了）");
    } else {
        cs::logf("[buggy] 侥幸被唤醒（时序恰好命中），ready=", h.ready);
    }
    // 即便“被唤醒”，无谓词版还会被虚假唤醒骗到：醒来时 ready 未必为真。
}

void demo_lost_wakeup() {
    cs::println("======== 必做 2：无谓词 → 丢失唤醒（反面教材） ========");
    Handoff h;

    // 关键时序：主线程“抢跑”，在 worker 还没 wait 前就 notify。
    {
        std::lock_guard<std::mutex> lk(h.mtx);
        h.ready = true;
        cs::logf("[buggy] 主线程抢先置位 ready 并 notify_one…");
    }
    h.cv.notify_one(); // 这一发通知打在空气上——此刻无人在 wait。

    // worker 才姗姗启动，错过了上面那一发 notify。
    std::thread t(worker_buggy_no_predicate, std::ref(h));
    t.join();

    cs::logf("[buggy] 结论：谓词版 wait 会先检查 ready=true 而立即返回，"
             "根本不会丢；裸 wait 则被永久挂起（此处用超时模拟）。");
    cs::println("");
}

// =====================================================================
// 进阶 1：notify_one vs notify_all。
//   多个 worker 等待同一个谓词时：
//     - notify_one 只唤醒一个，其余继续睡；
//     - notify_all 唤醒全部，各自重新检查谓词后竞争前进。
//   当“一次事件应让所有等待者前进”（如广播 ready），必须 notify_all；
//   当“一次事件只能让一个等待者前进”（如队列里多了一个元素），
//   notify_one 足够且更高效。
// =====================================================================
void demo_notify_all() {
    cs::println("============== 进阶 1：notify_one vs notify_all ==============");
    Handoff h;
    constexpr int kWorkers = 3;
    std::vector<std::thread> ts;

    for (int i = 0; i < kWorkers; ++i) {
        ts.emplace_back([&h, i] {
            std::unique_lock<std::mutex> lk(h.mtx);
            h.cv.wait(lk, [&] { return h.ready; }); // 谓词版，安全
            cs::logf("[all] worker ", i, " 被广播唤醒并通过谓词。");
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    {
        std::lock_guard<std::mutex> lk(h.mtx);
        h.ready = true;
        cs::logf("[all] 主线程广播：ready=true，notify_all 唤醒全部 worker。");
    }

    // TODO [进阶 1]: 此处用 notify_all 广播给所有等待者。
    //   思考：若这里误用 notify_one，会发生什么？
    //   （答案：只有一个 worker 被唤醒，另外两个永远卡在 wait——
    //    因为 ready 这个事件只被“消费”一次唤醒。这正是“广播型”
    //    谓词必须 notify_all 的原因。）
    h.cv.notify_all();

    for (auto& t : ts) t.join();
    cs::println("");
}

int main() {
    cs::println("==== C1_condvar_predicate：条件变量与谓词等待 ====\n");

    demo_correct();      // 必做 1：正确的谓词等待
    demo_lost_wakeup();  // 必做 2：无谓词 → 丢失唤醒反面教材
    demo_notify_all();   // 进阶 1：notify_one vs notify_all

    cs::println("==== 全部演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
