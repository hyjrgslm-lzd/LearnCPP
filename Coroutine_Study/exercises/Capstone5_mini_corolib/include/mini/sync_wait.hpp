// =============================================================================
// mini/sync_wait.hpp —— 同步阻塞等待 sender / awaitable 完成
//
// 对应文档：14-第三阶段结课-mini协程库实现.md  §"第六层：sync_wait"
//
// 关键约束（与 P2300 stdexec::sync_wait 一致）：
//   - 返回类型固定为 std::optional<std::tuple<Ts...>>；
//   - set_stopped 通道 -> 返回空 optional；
//   - set_error 通道   -> rethrow 异常；
//   - 验证代码必须写：auto opt = mini::sync_wait(...); auto [v] = *opt;
//
// 设计要点：
//   - 内部 sync_wait_receiver + condition_variable + mutex；
//   - 在 set_value 时存结果，*然后* notify（顺序反了等待线程会拿到空结果）。
//
// HALO 验证：
//   - sync_wait(just(42)) 路径下，frame 地址永不逃逸；
//   - 用 Clang -Rpass=coroutine-elide 编译应见 "coroutine frame elided"。
// =============================================================================

#pragma once

#include <condition_variable>
#include <coroutine>
#include <exception>
#include <mutex>
#include <optional>
#include <tuple>
#include <utility>
#include <variant>

namespace mini {

// 简化版：sender 只产出单一 T 的 set_value(T)
// 真实实现需用 completion_signatures 推 Ts...
template <typename T>
class sync_wait_state {
    std::mutex                                              mtx_;
    std::condition_variable                                 cv_;
    bool                                                    done_{false};
    // index: 0 = unset, 1 = value, 2 = error, 3 = stopped (struct stopped_tag)
    struct stopped_tag {};
    std::variant<std::monostate, T, std::exception_ptr, stopped_tag> result_{};
public:
    void set_value(T v) {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            result_.template emplace<1>(std::move(v));
            done_ = true;       // 先存结果
        }
        cv_.notify_one();       // 再 notify
    }
    void set_error(std::exception_ptr ep) {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            result_.template emplace<2>(std::move(ep));
            done_ = true;
        }
        cv_.notify_one();
    }
    void set_stopped() {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            result_.template emplace<3>(stopped_tag{});
            done_ = true;
        }
        cv_.notify_one();
    }

    std::optional<std::tuple<T>> wait() {
        std::unique_lock<std::mutex> lk(mtx_);
        cv_.wait(lk, [&] { return done_; });
        if (result_.index() == 1) return std::tuple<T>{std::move(std::get<1>(result_))};
        if (result_.index() == 2) std::rethrow_exception(std::get<2>(result_));
        return std::nullopt; // stopped
    }
};

// 同步等待一个返回 task<T> 的协程
//
// 注意：本函数是【骨架占位】，永远返回 std::nullopt。
// 必做任务要求实现完整 sender → optional<tuple> 转换：
//   - 构造 sync_wait_receiver<T>；
//   - connect(s, receiver) -> op_state；
//   - start(op_state)；
//   - state.wait() 阻塞至 done，按 set_value/set_error/set_stopped 分流。
// 在补全前，依赖本 sync_wait 的测试都会以 nullopt 短路；这是预期。
template <typename T>
inline std::optional<std::tuple<T>> sync_wait(/* sender or awaitable */ auto&& s) {
    // TODO[必做]: 完整实现：
    //   - 构造 sync_wait_receiver<T>；
    //   - connect(s, receiver) -> op_state；
    //   - start(op_state)；
    //   - state.wait() 阻塞至 done。
    //
    // 简化路径（仅 awaitable）：
    //   把 s 作为 await 操作放到一个 lambda 协程里执行：
    //     auto inner = [&]() -> task<T> { co_return co_await std::move(s); }();
    //     直接 inner.h_.resume()，等 done 后取 promise.result_。
    //
    // 本骨架返回空 optional 占位 —— 必做任务要求替换为完整 sender→optional<tuple> 转换。
    (void)s;
    return std::nullopt;
}

} // namespace mini
