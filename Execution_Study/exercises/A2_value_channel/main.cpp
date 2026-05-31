#include <stdexec/execution.hpp>
#include <iostream>
#include <string>
#include <atomic>

namespace ex = stdexec;

// ── 自定义结构体 ──────────────────────────────────────
struct TaskInput {
    int         base;
    std::string label;
};

std::ostream& operator<<(std::ostream& os, const TaskInput& t) {
    return os << "TaskInput{base=" << t.base << ", label=\"" << t.label << "\"}";
}

int main() {
    // 递增序号，用于确认日志打印顺序
    int counter = 0;

    // ══════════════════════════════════════════════════════
    // 第一部分：int 值通道  just(5) -> then(+1) -> then(*2) -> sync_wait
    // ══════════════════════════════════════════════════════
    std::cout << "[" << counter++ << "] 主线程: 即将构造 sender（尚未执行）\n";

    // TODO [必做]: 构造 sender 链条
    //   auto sender1 = ex::just(5)
    //       | ex::then([&counter](int v) {
    //             std::cout << "[" << counter++ << "] stage_1_prepare : 输入=" << v << "\n";
    //             return ???;  // v + 1
    //         })
    //       | ex::then([&counter](int v) {
    //             std::cout << "[" << counter++ << "] stage_2_transform: 输入=" << v << "\n";
    //             return ???;  // v * 2
    //         });

    std::cout << "[" << counter++ << "] 主线程: sender 已构造，准备 sync_wait\n";

    // TODO [必做]: 用 sync_wait 消费 sender1 并取出结果
    //   auto [result1] = ex::sync_wait(std::move(sender1)).value();

    int result1 = 0;  // 替换为 sync_wait 得到的值

    std::cout << "[" << counter++ << "] 主线程: sync_wait 完成, result = " << result1 << "\n";

    // 预期结果: (5 + 1) * 2 = 12
    std::cout << "[验证] 预期 12, 实际 " << result1 << " => "
              << (result1 == 12 ? "OK" : "FAIL") << "\n\n";

    // ══════════════════════════════════════════════════════
    // 第二部分：TaskInput 结构体值通道
    // ══════════════════════════════════════════════════════
    counter = 0;
    std::cout << "--- 结构体版本 ---\n";
    std::cout << "[" << counter++ << "] 主线程: 即将构造结构体 sender\n";

    // TODO [必做]: 用 TaskInput 构造 sender 链条
    //   auto sender2 = ex::just(TaskInput{10, "hello"})
    //       | ex::then([&counter](TaskInput t) {
    //             std::cout << "[" << counter++ << "] stage_1_prepare : 输入=" << t << "\n";
    //             ???  // 修改 t.base += 5, t.label += "_processed"
    //             return t;
    //         })
    //       | ex::then([&counter](TaskInput t) {
    //             std::cout << "[" << counter++ << "] stage_2_transform: 输入=" << t << "\n";
    //             ???  // 修改 t.base *= 3
    //             return t;
    //         });

    std::cout << "[" << counter++ << "] 主线程: 结构体 sender 已构造，准备 sync_wait\n";

    // TODO [必做]: 用 sync_wait 消费 sender2
    //   auto [result2] = ex::sync_wait(std::move(sender2)).value();

    TaskInput result2{0, ""};  // 替换为 sync_wait 得到的值

    std::cout << "[" << counter++ << "] 主线程: sync_wait 完成, result = " << result2 << "\n";

    // 预期: base = (10+5)*3 = 45, label = "hello_processed"
    std::cout << "[验证] 预期 base=45, 实际 base=" << result2.base << " => "
              << (result2.base == 45 ? "OK" : "FAIL") << "\n";

    // ══════════════════════════════════════════════════════
    // TODO [进阶]: 插入一个返回 void 的 then 阶段
    //   观察 value channel 形状如何变化——下游 lambda 的参数变为 ()。
    //   再做一个版本：让某个阶段只记录日志但不修改值（透传），
    //   比较"有副作用的透传"和"真正变换值"的区别。
    // ══════════════════════════════════════════════════════

    return 0;
}
