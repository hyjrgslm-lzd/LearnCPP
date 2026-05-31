// =====================================================================
// 练习 B1_mutex_family：互斥量家族与 RAII 锁包装器
//   对应文档：Concurrency_Study/03-模块B-互斥与锁.md 的「练习 B-1」
//
//   官方参考：
//     std::mutex          https://en.cppreference.com/w/cpp/thread/mutex
//     std::shared_mutex   https://en.cppreference.com/w/cpp/thread/shared_mutex
//     std::timed_mutex    https://en.cppreference.com/w/cpp/thread/timed_mutex
//     std::lock_guard     https://en.cppreference.com/w/cpp/thread/lock_guard
//     std::unique_lock    https://en.cppreference.com/w/cpp/thread/unique_lock
//     std::shared_lock    https://en.cppreference.com/w/cpp/thread/shared_lock
//
//   学习目标：
//     1. 亲眼看到「无锁自增」的数据竞争，再用 mutex + lock_guard 修复。
//     2. 用 shared_mutex + shared_lock(读) / lock_guard(写) 写读多写少结构。
//     3. 体会 unique_lock 相对 lock_guard 的灵活性（提前 unlock / 移动 / 配 condvar）。
//     4. (进阶) timed_mutex 的 try_lock_for 限时获取。
//
//   本文件用 stdout 测试驱动（main 直接跑各场景并打印观察结果）。
//   未完成的 TODO 用最小占位实现保证 MSVC(VS2026, C++20) 可编译可运行。
// =====================================================================
#include "concurrency_study/log.hpp"

#include <atomic>
#include <chrono>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

namespace {

constexpr int kThreads = 8;
constexpr int kIncrPerThread = 100000;

// ---------------------------------------------------------------------
// 场景 0（已实现·对照组）：无锁自增 —— 故意制造数据竞争。
//   counter++ 是「读—改—写」三步，多个线程会互相覆盖（lost update），
//   最终值几乎从不等于 kThreads * kIncrPerThread。这一步用来让你亲眼
//   看到数据竞争客观存在，不要跳过。
// ---------------------------------------------------------------------
void scenario_race() {
    cs::println("\n=== 场景0：无锁自增（数据竞争对照组） ===");
    int counter = 0; // 故意不加保护
    std::vector<std::thread> ts;
    for (int i = 0; i < kThreads; ++i) {
        ts.emplace_back([&counter] {
            for (int k = 0; k < kIncrPerThread; ++k) {
                ++counter; // 数据竞争：UB。结果不可预测。
            }
        });
    }
    for (auto& t : ts) t.join();
    cs::logf("无锁结果 = ", counter, "  期望 = ", kThreads * kIncrPerThread,
             "  (几乎总是偏小，且每次运行都不同)");
}

// ---------------------------------------------------------------------
// 场景 1（必做 1）：mutex + lock_guard 保护计数器。
// ---------------------------------------------------------------------
void scenario_mutex_counter() {
    cs::println("\n=== 场景1：mutex + lock_guard 保护计数器（必做1） ===");
    int counter = 0;
    std::mutex m;
    std::vector<std::thread> ts;
    for (int i = 0; i < kThreads; ++i) {
        ts.emplace_back([&counter, &m] {
            for (int k = 0; k < kIncrPerThread; ++k) {
                // TODO [必做 1]: 用 std::lock_guard 保护这次自增。
                //   关键点：lock_guard 构造时上锁、析构(本次循环结束)时解锁，
                //   异常安全且无需手写 unlock。务必给它命名(否则是临时对象立即析构)。
                //   参考实现（取消注释即为正确答案）：
                //     std::lock_guard<std::mutex> lk(m);
                //     ++counter;
                //
                // 下面是最小占位：直接自增(仍是竞争)，保证可编译。
                // 填了上面的 lock_guard 后，请删掉这一行占位。
                ++counter;
                (void)m;
            }
        });
    }
    for (auto& t : ts) t.join();
    cs::logf("加锁结果 = ", counter, "  期望 = ", kThreads * kIncrPerThread,
             "  (填好 lock_guard 后应每次都精确相等)");
}

// ---------------------------------------------------------------------
// 场景 2（必做 3）：shared_mutex 读多写少。
//   读：shared_lock(共享锁，多读者可并发)。
//   写：lock_guard<shared_mutex>(独占锁，与所有读写互斥)。
// ---------------------------------------------------------------------
class Config {
public:
    int get(const std::string& key) const {
        // TODO [必做 3a]: 用 std::shared_lock 上「共享锁」，允许多读者并发。
        //   参考实现：
        //     std::shared_lock<std::shared_mutex> lk(mu_);
        //   占位：暂不加锁(读路径在有写者时会有竞争)，保证可编译。
        auto it = data_.find(key);
        return it == data_.end() ? -1 : it->second;
    }

    void set(const std::string& key, int val) {
        // TODO [必做 3b]: 用 std::lock_guard<std::shared_mutex> 上「独占锁」。
        //   注意：写者不能用 shared_lock（那是共享/读模式）。lock_guard 调用
        //   的是 mu_.lock()（独占版），shared_lock 调用 mu_.lock_shared()。
        //   参考实现：
        //     std::lock_guard<std::shared_mutex> lk(mu_);
        data_[key] = val;
    }

private:
    mutable std::shared_mutex mu_;
    std::map<std::string, int> data_;
};

void scenario_shared_mutex() {
    cs::println("\n=== 场景2：shared_mutex 读多写少（必做3） ===");
    Config cfg;
    cfg.set("level", 1);

    std::vector<std::thread> readers;
    for (int i = 0; i < 6; ++i) {
        readers.emplace_back([&cfg, i] {
            for (int k = 0; k < 3; ++k) {
                int v = cfg.get("level");
                cs::logf("reader#", i, " 读到 level=", v);
                std::this_thread::sleep_for(5ms);
            }
        });
    }
    std::thread writer([&cfg] {
        for (int v = 2; v <= 4; ++v) {
            std::this_thread::sleep_for(8ms);
            cfg.set("level", v);
            cs::logf("writer 写入 level=", v);
        }
    });

    for (auto& t : readers) t.join();
    writer.join();
    cs::logf("最终 level=", cfg.get("level"),
             "  (无崩溃 / 无撕裂读 即为通过)");
}

// ---------------------------------------------------------------------
// 场景 3（必做 4）：unique_lock 灵活性 —— 临界区内提前 unlock。
//   先持锁取出需要的数据，unlock 后做耗时的非共享计算，避免长时间占锁。
// ---------------------------------------------------------------------
void scenario_unique_lock() {
    cs::println("\n=== 场景3：unique_lock 提前 unlock 把耗时活移出锁外（必做4） ===");
    std::mutex m;
    int shared_input = 42;

    auto worker = [&](int id) {
        // TODO [必做 4]: 用 std::unique_lock 先上锁取数据，再提前 unlock，
        //   然后在锁外做耗时计算。lock_guard 做不到「提前手动 unlock」。
        //   参考实现：
        //     std::unique_lock<std::mutex> lk(m);
        //     int local = shared_input;       // 锁内只做最小读取
        //     cs::logf("worker#", id, " 持锁取数据 ", local);
        //     lk.unlock();                    // 关键：提前释放锁
        //     cs::logf("worker#", id, " 已释放锁，开始锁外耗时计算");
        //     std::this_thread::sleep_for(20ms); // 模拟耗时(在锁外)
        //     cs::logf("worker#", id, " 计算完成 result=", local * local);
        //
        // 最小占位（持锁全程，含耗时，演示「坏」做法以便对照）：
        std::lock_guard<std::mutex> lk(m);
        int local = shared_input;
        cs::logf("worker#", id, " [占位:持锁做耗时] 取数据 ", local);
        std::this_thread::sleep_for(20ms);
        cs::logf("worker#", id, " [占位] result=", local * local);
    };

    std::thread a(worker, 0), b(worker, 1);
    a.join();
    b.join();
}

// ---------------------------------------------------------------------
// 场景 4（进阶 1）：timed_mutex + try_lock_for 限时获取。
// ---------------------------------------------------------------------
void scenario_timed_mutex() {
    cs::println("\n=== 场景4：timed_mutex try_lock_for 限时获取（进阶1） ===");
    std::timed_mutex tm;

    std::thread holder([&tm] {
        std::lock_guard<std::timed_mutex> lk(tm); // 独占持锁较久
        cs::logf("holder 持锁 100ms ...");
        std::this_thread::sleep_for(100ms);
        cs::logf("holder 释放锁");
    });

    std::this_thread::sleep_for(10ms); // 确保 holder 先拿到锁
    std::thread trier([&tm] {
        // TODO [进阶 1]: 用 try_lock_for(50ms) 限时获取，拿到就干活，超时则放弃。
        //   参考实现：
        //     if (tm.try_lock_for(50ms)) {
        //         std::lock_guard<std::timed_mutex> lk(tm, std::adopt_lock);
        //         cs::logf("trier 在超时前拿到锁");
        //     } else {
        //         cs::logf("trier 50ms 内没拿到，放弃，去做别的事");
        //     }
        //
        // 最小占位：直接 try_lock_for 并打印结果（已是可运行的合理实现）。
        if (tm.try_lock_for(50ms)) {
            std::lock_guard<std::timed_mutex> lk(tm, std::adopt_lock);
            cs::logf("trier 在超时前拿到锁");
        } else {
            cs::logf("trier 50ms 内没拿到，放弃，去做别的事 (预期: 因 holder 持锁 100ms)");
        }
    });

    holder.join();
    trier.join();
}

} // namespace

int main() {
    cs::println("==== B1_mutex_family: 互斥量家族与 RAII 锁包装器 ====");
    scenario_race();          // 对照组：数据竞争
    scenario_mutex_counter(); // 必做1：mutex + lock_guard
    scenario_shared_mutex();  // 必做3：shared_mutex 读多写少
    scenario_unique_lock();   // 必做4：unique_lock 提前 unlock
    scenario_timed_mutex();   // 进阶1：timed_mutex try_lock_for
    cs::println("\n==== 全部场景跑完。对照 README / 03-模块B 文档自检验收点。 ====");
    return 0;
}
