#include "checks.hpp"
#include "concurrency_study/bounded_channel.hpp"

// 可复用已经学过的通道；线程创建、打包、提交、关闭必须由 student_pool 实现。
// Part 1：先画 submit -> channel -> worker -> future 的所有权流，再填写下面各项。
constexpr bool part1_flow_done = false;
constexpr bool part2_tasks_done = false;
constexpr bool part3_types_done = false;
constexpr bool part4_backpressure_done = false;
constexpr bool part5_shutdown_done = false;
constexpr bool part6_results_done = false;
constexpr bool part7_lifetime_done = false;

class student_pool {
public:
    explicit student_pool(std::size_t worker_count, std::size_t capacity) : tasks_(capacity) {
        // TODO Part 2/7：worker_count==0 抛 invalid_argument；启动固定数量 worker。
        // 第 k 个线程创建失败时，必须关闭通道并 join 已创建者，再重抛。
        // reserve/共享资源准备应在启动前完成；不完整对象不会调用自身析构。
        (void)worker_count;
        throw std::logic_error("TODO Part 2/7: construct workers");
    }
    student_pool(const student_pool&) = delete;
    student_pool& operator=(const student_pool&) = delete;
    ~student_pool() noexcept {
        // TODO Part 5/7：禁止本池 worker 自销毁；外部析构执行 drain + join。
        // 不可用 detach 绕过自 join；不能让异常逃逸 noexcept 析构。
    }

    template<class F, class... A>
    auto submit(F&& f, A&&... args)
        -> std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<A>...>> {
        // TODO Part 2/3/4/6：按值衰减保存 fn/args，以右值调用它们。
        // 用 packaged_task 传值/异常，先 get_future 再移入 move_only_function。
        // 满队列等待；close 拒收抛 runtime_error；禁止同步调用 f 冒充异步。
        // 同池 worker 的 submit 应在等空间之前抛 logic_error。
        (void)f;
        ((void)args, ...);
        throw std::logic_error("TODO Part 2/3/4: submit");
    }
    void shutdown() {
        // TODO Part 5/7：先识别同池 worker 并拒绝；close 拒收且唤醒所有等待者。
        // worker 排空 accepted 任务再退出；序列化多个关闭者的 join，重复调用合法。
        throw std::logic_error("TODO Part 5: shutdown");
    }
private:
    void worker_loop(std::size_t index) {
        // TODO Part 2/6/7：循环 pop，关闭且空才退出；在队列锁外调用任务。
        // 用户异常进入任务 future；其他 worker 异常记录到 errors_[index]，join 后回传。
        (void)index;
        throw std::logic_error("TODO Part 2: worker loop");
    }
    inline static thread_local const student_pool* current_pool_ = nullptr;
    cs::bounded_channel<std::move_only_function<void()>> tasks_;
    std::vector<std::exception_ptr> errors_;
    std::mutex shutdown_mutex_;
    std::vector<std::jthread> workers_; // 必须先于共享通道销毁
};

int main() {
    if (!(part1_flow_done && part2_tasks_done && part3_types_done && part4_backpressure_done &&
          part5_shutdown_done && part6_results_done && part7_lifetime_done)) {
        std::cerr << "STARTER INCOMPLETE: Capstone1 Part 1-7 未完成；未创建任何线程。\n";
        return 1;
    }
    try {
        // 实际构造 student_pool；检查 worker ID、move-only、不同 &/&& 返回类型、
        // 容量 1/关闭竞态、300 个独立 ID、异常 future、幂等 shutdown 和生命周期。
        pool_checks::run<student_pool>();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Capstone1 student FAIL: " << error.what() << '\n';
        return 1;
    }
}
