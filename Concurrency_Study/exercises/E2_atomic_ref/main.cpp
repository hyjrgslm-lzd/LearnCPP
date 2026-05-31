// =====================================================================
// 练习 E-2：atomic_ref 引用既有对象（std::atomic_ref，C++20）
//   对应文档：Concurrency_Study/07-模块E-原子操作基础.md 的 练习 E-2
//
//   学习目标：
//     - 理解 std::atomic_ref<T>（原子引用，<atomic>，C++20）解决的问题：
//       给一个“本来就是普通非原子类型”的既有对象（如数组某元素、结构体
//       字段）临时套上原子访问，而无需把该类型本身声明成 std::atomic；
//     - 用 atomic_ref 对一个普通 int 数组的某个元素做多线程并发累加，
//       结果精确无丢更新；
//     - 讲清 atomic_ref 的两条硬性要求：被引用对象要满足 atomic_ref 的
//       对齐要求（required_alignment），且 T 必须是可平凡复制
//       （trivially copyable）；以及生命周期约束：atomic_ref 存活期间，
//       对该对象的所有访问都必须经由 atomic_ref，且对象本身不能更早销毁。
//
//   关于内存序：与 E-1 一致，本题所有原子操作都用默认 seq_cst；为什么
//   能更弱留到模块 F。
//
//   官方参考：
//     - https://en.cppreference.com/w/cpp/atomic/atomic_ref
//     - https://en.cppreference.com/w/cpp/atomic/atomic_ref/fetch_add
//     - 提案 P0019R8 "Atomic Ref"
//     - 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 5 章 5.2.5
//
//   编译运行（VS2026, C++20）：
//     cmake --build build-vs2026 --target E2_atomic_ref --config Release
//     ./build-vs2026/E2_atomic_ref/Release/E2_atomic_ref.exe
// =====================================================================
#include "concurrency_study/log.hpp"

#include <atomic>
#include <thread>
#include <type_traits>
#include <vector>

// atomic_ref 要求被引用类型可平凡复制（trivially copyable）；int 显然满足。
static_assert(std::is_trivially_copyable_v<int>,
              "atomic_ref<T> 要求 T 可平凡复制（trivially copyable）");

// =====================================================================
// 必做 1：对普通 int 数组的某个元素用 atomic_ref 并发累加。
//   关键对比：数组本身是 int[]（非原子）。我们并不改它的类型，而是在每个
//   线程里就同一个元素 data[idx] 构造一个 atomic_ref，对它 fetch_add。
//   这样既保持底层数据是普通数组（可被其它非并发代码当普通 int 用），
//   又能在并发阶段获得原子保证。
// =====================================================================
void demo_atomic_ref_array() {
    cs::println("======== 必做 1：atomic_ref 累加普通数组元素 ========");

    // 普通的、非原子的 int 数组。注意它的类型仍是 int[]。
    // alignas 确保目标元素满足 atomic_ref<int>::required_alignment
    //   （对 int 通常 == alignof(int)，这里显式标注以示要求）。
    alignas(std::atomic_ref<int>::required_alignment) int data[4] = {0, 0, 0, 0};

    constexpr int kHotIndex = 2; // 大家一起累加的“热点”元素
    constexpr int kThreads = 8;
    constexpr int kPerThread = 100000;

    std::vector<std::thread> ts;
    for (int i = 0; i < kThreads; ++i) {
        ts.emplace_back([&data] {
            for (int k = 0; k < kPerThread; ++k) {
                // TODO [必做 1]: 就 data[kHotIndex] 这个“普通 int 元素”构造
                //   一个 std::atomic_ref<int>，并用 fetch_add(1) 原子累加。
                //   要点：atomic_ref 是“引用语义”——它不拥有数据，只是把
                //   原子操作叠加到既有对象上；多个线程各自构造指向同一元素的
                //   atomic_ref 是合法且预期的用法。默认内存序 seq_cst。
                //   下面已是正确写法，留作必做 1 的参考实现：
                std::atomic_ref<int> ref(data[kHotIndex]);
                ref.fetch_add(1);
            }
        });
    }
    for (auto& t : ts) t.join();

    const long long expected =
        static_cast<long long>(kThreads) * kPerThread;
    // 并发阶段结束后，atomic_ref 都已析构，data 又是普通数组，可直接读。
    cs::logf("[ref] data[", kHotIndex, "] = ", data[kHotIndex],
             "，期望 = ", expected,
             "（", (data[kHotIndex] == expected ? "精确相等" : "异常"), "）");
    cs::logf("[ref] 其余元素仍为 0：data[0]=", data[0], " data[1]=", data[1],
             " data[3]=", data[3]);
    cs::println("");
}

// =====================================================================
// 必做 2：讲清对齐与生命周期要求（用可运行的小演示佐证）。
//   - required_alignment：atomic_ref<T> 提供的 static constexpr，告诉你
//     被引用对象至少要按多少字节对齐，才能保证无锁/正确的原子访问。
//     对齐不足会导致未定义行为，所以对“可能未对齐”的对象（如打包结构体
//     字段、手动从字节缓冲取出的对象）要格外小心。
//   - 生命周期：atomic_ref 不延长被引用对象寿命；只要还有 atomic_ref
//     活着，被引用对象就必须保持存活，并且对该对象的全部访问都要经由
//     某个 atomic_ref（不能一边 atomic_ref、一边普通读写同一对象）。
// =====================================================================
void demo_alignment_and_lifetime() {
    cs::println("========= 必做 2：对齐 与 生命周期 要求 =========");

    cs::logf("[req] atomic_ref<int>::required_alignment    = ",
             std::atomic_ref<int>::required_alignment, " 字节");
    cs::logf("[req] alignof(int)                           = ",
             alignof(int), " 字节");
    cs::logf("[req] atomic_ref<long long>::required_alignment = ",
             std::atomic_ref<long long>::required_alignment, " 字节");
    cs::logf("[req] atomic_ref<int>::is_always_lock_free   = ",
             std::atomic_ref<int>::is_always_lock_free);

    // TODO [必做 2]: 用一个“先经 atomic_ref 原子写、待其析构后再普通读”的
    //   合法顺序，体会生命周期边界。
    //   要点：value 在 atomic_ref 存活期间只通过 ref 访问；当我们想用普通
    //   方式读它时，必须确保再没有任何 atomic_ref 指向它（这里用作用域
    //   {} 让 ref 提前析构）。下面已是正确写法，留作必做 2 的参考实现：
    int value = 0;
    {
        std::atomic_ref<int> ref(value); // ref 存活期间只经 ref 访问 value
        ref.store(123);
        cs::logf("[life] 经 atomic_ref store(123) 后，ref.load() = ", ref.load());
    } // ref 析构：此后再无 atomic_ref 指向 value
    cs::logf("[life] ref 析构后，普通读 value = ", value, "（合法）");

    cs::println("[note] 错误用法（勿写）：在仍有 atomic_ref 指向某对象时，"
                "再用普通 (++value) 方式并发访问同一对象 —— 这是数据竞争/UB。");
    cs::println("");
}

int main() {
    cs::println("==== E2_atomic_ref：给既有对象套原子访问（C++20） ====\n");

    demo_atomic_ref_array();      // 必做 1：atomic_ref 并发累加数组元素
    demo_alignment_and_lifetime(); // 必做 2：对齐与生命周期要求

    cs::println("==== 全部演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
