#include "concurrency_study/exercise_check.hpp"
#include <array>
#include <atomic>
#include <barrier>
#include <future>
#include <iostream>
#include <latch>
#include <numeric>
#include <vector>

constexpr bool part1_latch_done = false;
constexpr bool part2_completion_done = false;
constexpr bool part3_arrival_done = false;
constexpr bool part4_drop_done = false;
constexpr bool part5_launch_done = false;
void student_ready_and_wait(std::latch& ready, std::latch& go) {
    // TODO Part 1：已写完自己的准备结果；报告一次 ready，再等 go 发令。
    (void)ready; (void)go;
    throw std::logic_error("TODO Part 1: ready then go");
}
struct student_completion {
    const std::array<int, 3>& slots;
    std::array<int, 4>& totals;
    int& phase;
    void operator()() const noexcept {
        // TODO Part 2：汇总 slots 写入 totals[phase] 再推进 phase。
        // 仅做不抛出的数组/整数操作；不要在 completion 内抛 cs::check。
    }
};
template<class Barrier>
void student_arrive_then_wait(Barrier& barrier) {
    // TODO Part 3：arrive 取得 token，再移入 wait，二者之间不修改共享槽。
    (void)barrier;
    throw std::logic_error("TODO Part 3: arrival token");
}
template<class Barrier>
void student_drop(Barrier& barrier) {
    // TODO Part 4：减少本轮剩余与未来初始计数；不等当前 phase 完成。
    (void)barrier;
    throw std::logic_error("TODO Part 4: arrive_and_drop");
}
bool student_allow_launch(const std::shared_future<bool>& launch) {
    // TODO Part 5：等创建方发布 true/false；false 时必须在进入 barrier 前退出。
    (void)launch;
    throw std::logic_error("TODO Part 5: startup rollback gate");
}
void check_student() {
    {
        std::latch ready(4), go(1);
        std::array<int, 4> prepared{};
        std::atomic<int> started{0};
        std::vector<std::future<void>> workers;
        workers.reserve(4);
        try {
            for (int i = 0; i < 4; ++i)
                workers.push_back(std::async(std::launch::async, [&, i] {
                    prepared[i] = i + 1;
                    student_ready_and_wait(ready, go);
                    ++started;
                }));
        } catch (...) { go.count_down(); throw; }
        ready.wait();
        const bool before = started == 0 && std::accumulate(prepared.begin(), prepared.end(), 0) == 10;
        go.count_down();
        for (auto& w : workers) w.get();
        cs::check(before && started == 4, "Part 1: preparation before release");
        ready.wait(); go.wait(); // 已归零可重复等，不可再 count_down(1)。
    }
    std::promise<bool> rejected_launch;
    auto rejected = rejected_launch.get_future().share();
    rejected_launch.set_value(false);
    cs::check(!student_allow_launch(rejected), "Part 5: failed startup declines phase entry");
    std::array<int, 3> slots{};
    std::array<int, 4> totals{};
    int phase = 0;
    std::barrier barrier(3, student_completion{slots, totals, phase});
    std::promise<bool> launch;
    auto start = launch.get_future().share();
    std::vector<std::future<void>> workers;
    workers.reserve(3);
    try {
        for (int i = 0; i < 3; ++i)
            workers.push_back(std::async(std::launch::async, [&, i] {
                if (!student_allow_launch(start)) return;
                for (int r = 0; r < 4; ++r) {
                    slots[i] = (r + 1) * 10 + i;
                    student_arrive_then_wait(barrier);
                }
            }));
    } catch (...) { launch.set_value(false); throw; }
    launch.set_value(true);
    for (auto& w : workers) w.get();
    cs::check(phase == 4 && totals == std::array{33, 63, 93, 123}, "Part 2/3: four exact phases");
    int completions = 0;
    std::barrier shrink(2, [&]() noexcept { ++completions; });
    auto leaving = std::async(std::launch::async, [&] { student_drop(shrink); });
    shrink.arrive_and_wait();
    leaving.get();
    shrink.arrive_and_wait();
    cs::check(completions == 2, "Part 4: drop changes future phases");
}
int main() {
    if (!(part1_latch_done && part2_completion_done && part3_arrival_done && part4_drop_done && part5_launch_done)) {
        std::cerr << "STARTER INCOMPLETE: H1 Part 1-5 未完成；未创建任何线程。\n";
        return 1;
    }
    try { check_student(); std::cout << "H1 student OK\n"; return 0; }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
