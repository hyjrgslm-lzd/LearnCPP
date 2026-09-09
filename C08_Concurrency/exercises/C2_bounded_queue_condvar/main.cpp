#include "checks.hpp"
#include <condition_variable>
#include <mutex>
#include <type_traits>

// 每完成一项实现并核对契约，再把对应项改为 true；标记不代替下面的检查。
constexpr bool part1_operations_done = false;
constexpr bool part2_storage_done = false;
constexpr bool part3_mpmc_done = false;
constexpr bool part4_close_done = false;

template<class T>
class student_channel {
    static_assert(std::is_nothrow_move_constructible_v<T>);
    static_assert(std::is_nothrow_destructible_v<T>);
public:
    explicit student_channel(std::size_t capacity) {
        // TODO Part 2：capacity==0 抛 invalid_argument；分配 capacity 个空槽。
        // 不变量：0<=size_<=capacity；head_/tail_ 指向有效环形下标。
        (void)capacity;
        throw std::logic_error("TODO Part 2: channel construction");
    }
    student_channel(const student_channel&) = delete;
    student_channel& operator=(const student_channel&) = delete;

    bool push(T value) {
        // TODO Part 1/3：用同一 mutex 保护检查与插入；满时等“有空位或关闭”。
        // 关闭优先：即便有空位也返回 false。成功后唤醒一名消费者。
        // 参数按值接收：失败不保证保留调用者传入的右值。
        (void)value;
        throw std::logic_error("TODO Part 1: push");
    }
    std::optional<T> pop() {
        // TODO Part 1/3：空且开放时等；关闭且空才返回 nullopt。
        // 持锁移动队首、销毁槽内旧值、推进下标并减 size，再通知生产者。
        throw std::logic_error("TODO Part 1: pop");
    }
    void close() {
        // TODO Part 4：持锁单向设置 closed_；广播两个 CV，保留已有元素。
        // close 不 join；调用者必须等全部访问者结束才析构本对象。
        throw std::logic_error("TODO Part 4: close");
    }
private:
    std::vector<std::optional<T>> slots_;
    std::mutex mutex_;
    std::condition_variable not_full_, not_empty_;
    std::size_t head_ = 0, tail_ = 0, size_ = 0;
    bool closed_ = false;
};

int main() {
    if (!(part1_operations_done && part2_storage_done && part3_mpmc_done && part4_close_done)) {
        std::cerr << "STARTER INCOMPLETE: C2 Part 1-4 未完成；未创建任何线程。\n";
        return 1;
    }
    try {
        // Part 2：0/1/2 容量、move-only、FIFO/环绕；Part 3：600 个唯一 ID；
        // Part 4：满/空两侧关闭与重复 close。每次操作都调用上面的学生类型。
        channel_checks::run<student_channel>();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "C2 student FAIL: " << error.what() << '\n';
        return 1;
    }
}
