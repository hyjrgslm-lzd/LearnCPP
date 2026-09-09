// =====================================================================
// 练习 B-2：task<T> 顺序异步组合
//   对应文档：03-模块B-generator与task的使用.md / 练习 B-2
//   官方参考：
//     - cppreference: https://en.cppreference.com/w/cpp/language/coroutines
//     - Lewis Baker:  https://lewissbaker.github.io/2017/11/17/understanding-operator-co-await
//     - cppcoro task: https://github.com/lewissbaker/cppcoro/blob/master/include/cppcoro/task.hpp
//
// 学习目标：
//   - 看见 co_await 链让"异步顺序"读起来像同步代码
//   - 看见异常沿 co_await 链自然传播（fetch 抛 -> sync_wait 重新抛）
//   - 与回调金字塔对比错误处理与值流的差异
// =====================================================================

#include <coroutine_study/lazy_task.hpp>

#include <chrono>
#include <format>
#include <functional>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <syncstream>
#include <thread>

using namespace std::chrono_literals;
using coroutine_study::lazy_task;

namespace {

template <class... Args>
void log(const char* tag, Args&&... args) {
    std::osyncstream os{std::cout};
    os << "[" << tag << "] ";
    ((os << args), ...);
    os << "  (tid=" << std::this_thread::get_id() << ")\n";
}

struct User    { int id; std::string name;  std::string email; };
struct Profile { int user_id; std::string display_name; std::string bio; };
struct ValidatedProfile {
    int user_id; std::string display_name; bool is_valid; int score;
};

// ─────────────────────────────────────────────────────────────────────
// 必做：三个独立的 lazy_task 协程
// ─────────────────────────────────────────────────────────────────────
lazy_task<User> fetch_user(int user_id) {
    log("fetch_user", "begin id=", user_id);
    std::this_thread::sleep_for(50ms);

    // TODO [必做 5]: 加 30% 概率抛 std::runtime_error("fetch failed");
    //   验证异常沿 co_await 链传播到 sync_wait

    log("fetch_user", "done");
    co_return User{user_id, "alice", "alice@example.com"};
}

lazy_task<Profile> parse_profile(User user) {
    log("parse_profile", "begin id=", user.id);
    std::this_thread::sleep_for(30ms);
    log("parse_profile", "done");
    co_return Profile{user.id, user.name + "_display", "bio of " + user.name};
}

lazy_task<ValidatedProfile> validate_profile(Profile profile) {
    log("validate_profile", "begin id=", profile.user_id);
    std::this_thread::sleep_for(20ms);
    log("validate_profile", "done");
    co_return ValidatedProfile{profile.user_id, profile.display_name,
                                /*is_valid=*/true, /*score=*/88};
}

// ─────────────────────────────────────────────────────────────────────
// 必做：顶层串联
// ─────────────────────────────────────────────────────────────────────
lazy_task<std::string> process_user(int user_id) {
    // TODO [必做 2]:
    //   auto user    = co_await fetch_user(user_id);
    //   auto profile = co_await parse_profile(user);
    //   auto result  = co_await validate_profile(profile);
    //   if (result.is_valid)
    //       co_return std::format("User {} ({}) validated, score={}",
    //                             result.user_id, result.display_name, result.score);
    //   else
    //       co_return std::format("User {} ({}) failed validation",
    //                             result.user_id, result.display_name);
    //
    // 占位实现，让骨架默认可编译运行：
    auto user    = co_await fetch_user(user_id);
    auto profile = co_await parse_profile(user);
    auto result  = co_await validate_profile(profile);
    co_return std::format("user={} score={}", result.user_id, result.score);
}

// ─────────────────────────────────────────────────────────────────────
// 必做 6：等价的回调嵌套版（用于对比"错误处理在哪"）
// ─────────────────────────────────────────────────────────────────────
// TODO [必做 6]: 写 callback 版（伪代码）
//   void process_user_cb(int id, std::function<void(std::string)> done) {
//       fetch_user_cb(id, [done](User u) {
//           parse_profile_cb(u, [done](Profile p) {
//               validate_profile_cb(p, [done](auto r) {
//                   done(...);
//               });
//           });
//       });
//   }
//   把"如果某一层报错"分别加到 lambda 里——感受错误处理的分散性。

// ─────────────────────────────────────────────────────────────────────
// 进阶任务
// ─────────────────────────────────────────────────────────────────────
// TODO [进阶 A]: 让三个 task "同时启动 + 分别等待"（lazy_task 不支持 when_all 时的近似版本）
// TODO [进阶 B]: 在 process_user 顶层 try-catch，捕获 fetch 抛的异常，降级返回默认串
// TODO [进阶 C]: 把某步返回类型改成 lazy_task<std::optional<T>>

}  // namespace

int main() {
    log("main", "─── B-2：task 顺序异步组合 ───");

    try {
        auto t = process_user(/*user_id=*/42);
        std::string s = coroutine_study::sync_wait(std::move(t));
        log("main", "result = ", s);
    } catch (const std::exception& e) {
        log("main", "[exception] ", e.what(),
            "  —— 验证：异常沿 co_await 链传到了 sync_wait");
    }

    return 0;
}
