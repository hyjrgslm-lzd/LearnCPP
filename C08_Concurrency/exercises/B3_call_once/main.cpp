#include "concurrency_study/exercise_check.hpp"
#include <atomic>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <vector>

constexpr bool part1_once_done = false;
constexpr bool part2_retry_done = false;
constexpr bool part3_static_done = false;
constexpr bool part4_argument_done = false;
struct first_attempt_failed {};
struct student_resource {
    std::once_flag flag;
    std::unique_ptr<int> value;
    int attempts = 0; // 只能在序列化的 active 初始化体里修改；join 后读。
    void initialize(int initial) {
        // TODO Part 2/4：第一次 attempts 增为1时抛 first_attempt_failed，
        // 第二次用 initial 构造完整资源并发布，失败不发布半成品。
        (void)initial;
        throw std::logic_error("TODO Part 2/4: initialize");
    }
    int get(int initial) {
        // TODO Part 1/4：call_once(flag, 初始化函数, initial)，成功后按值读资源。
        // 不要在锁外手工检查普通指针，也不要吞掉第一次初始化的异常。
        (void)initial;
        throw std::logic_error("TODO Part 1: call_once");
    }
};
std::atomic<int> static_constructions{0};
int student_local_resource() {
    // TODO Part 3：函数内 static，用初始化 lambda 令计数+1并返回456。
    throw std::logic_error("TODO Part 3: local static");
}
int main() {
    if (!(part1_once_done && part2_retry_done && part3_static_done && part4_argument_done)) {
        std::cerr << "STARTER INCOMPLETE: B3 Part 1-4 未完成；未创建任何线程。\n";
        return 1;
    }
    try {
        student_resource resource;
        std::atomic<int> failures{0};
        std::vector<std::future<void>> readers;
        for (int i = 0; i < 8; ++i)
            readers.push_back(std::async(std::launch::async, [&] {
                // 只重试明确的教学异常，最多两次；其他错误原样进入 future。
                bool success = false;
                for (int retry = 0; retry < 2 && !success; ++retry) {
                    try {
                        cs::check(resource.get(123) == 123, "Part 1/4: published argument");
                        success = true;
                    } catch (const first_attempt_failed&) { ++failures; }
                }
                cs::check(success, "Part 2: retry eventually succeeds");
                cs::check(student_local_resource() == 456, "Part 3: static value");
            }));
        for (auto& reader : readers) reader.get();
        cs::check(resource.attempts == 2 && failures == 1, "Part 2: exactly one failure then success");
        cs::check(resource.get(999) == 123 && resource.attempts == 2, "Part 4: success is not reinitialized");
        cs::check(static_constructions == 1, "Part 3: static initialized once");
        std::cout << "B3 student OK\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
