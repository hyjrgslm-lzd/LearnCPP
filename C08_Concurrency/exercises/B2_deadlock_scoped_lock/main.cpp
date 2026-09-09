#include "concurrency_study/exercise_check.hpp"
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <string_view>

constexpr bool part1_cycle_done = false;
constexpr bool part2_scoped_done = false;
constexpr bool part3_alternatives_done = false;
// TODO Part 1：填写等待环，并在注释解释互斥、持有并等待、不可抢占、循环等待。
// Starter 只分析图，不启动永久死锁线程；例如 T1->T2->T1。
constexpr std::string_view student_wait_cycle = "";
struct account { std::mutex mutex; int balance = 10000; };
void student_scoped_transfer(account& from, account& to, int amount) {
    // TODO Part 2：先处理别名；scoped_lock 后检查金额/余额，再扣钱和加钱。
    // 无效输入抛异常且不改变余额；多锁算法不是硬件原子获取。
    (void)from; (void)to; (void)amount;
    throw std::logic_error("TODO Part 2: scoped transfer");
}
void student_adopt_transfer(account& from, account& to, int amount) {
    // TODO Part 3a：同一业务契约，std::lock 获取后 adopt_lock 接管已经持有的锁。
    (void)from; (void)to; (void)amount;
    throw std::logic_error("TODO Part 3a: adopt transfer");
}
void student_ordered_transfer(account& from, account& to, int amount) {
    // TODO Part 3b：先处理别名，用 std::less<const void*> 建立一致获取顺序。
    // 禁用不相关指针的内建 <；两把锁都获取后才更新不变量。
    (void)from; (void)to; (void)amount;
    throw std::logic_error("TODO Part 3b: ordered transfer");
}
int main() {
    if (!(part1_cycle_done && part2_scoped_done && part3_alternatives_done)) {
        std::cerr << "STARTER INCOMPLETE: B2 Part 1-3 未完成；未创建任何线程。\n";
        return 1;
    }
    try {
        cs::check(!student_wait_cycle.empty(), "Part 1: supply waiting-cycle explanation");
        for (auto transfer : {student_scoped_transfer, student_adopt_transfer, student_ordered_transfer}) {
            account a, b;
            auto forward = std::async(std::launch::async, [&] {
                for (int i = 0; i < 1000; ++i) transfer(a, b, 1);
            });
            auto reverse = std::async(std::launch::async, [&] {
                for (int i = 0; i < 1000; ++i) transfer(b, a, 2);
            });
            forward.get(); reverse.get();
            transfer(a, a, 1);
            cs::check(a.balance == 11000 && b.balance == 9000, "exact balances and alias");
            bool rejected = false;
            try { transfer(a, b, 20000); } catch (const std::exception&) { rejected = true; }
            cs::check(rejected && a.balance == 11000 && b.balance == 9000, "invalid transfer preserves balances");
        }
        std::cout << "B2 student OK\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
