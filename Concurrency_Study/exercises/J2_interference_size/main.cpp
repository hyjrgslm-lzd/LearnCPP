// =====================================================================
// 练习 J-2：硬件干扰尺寸常量（hardware interference size）
//   对应文档：Concurrency_Study/13-模块J-缓存与伪共享.md 的 练习 J-2
//
//   学习目标：
//     - 认识 C++17 在 <new> 引入的一对实现定义常量：
//         std::hardware_destructive_interference_size（破坏性干扰尺寸）
//         std::hardware_constructive_interference_size（建设性干扰尺寸）
//       它们把“缓存行大小”这个曾经只能硬编码 64 的魔法数，变成了
//       标准、可移植的编译期常量；
//     - 说清两者的语义对立面：
//         * destructive（破坏性）：两个被【不同线程】频繁访问的对象，至少应
//           相距这么多字节，才能保证落在【不同缓存行】、互不干扰 ——
//           用它做对齐 / 填充来【隔开】，避免伪共享（false sharing）；
//         * constructive（建设性）：两个对象若想【促进真共享 / 提升局部性】
//           （希望它们被同一线程一起访问时同处一条缓存行、一次载入），
//           其合并大小不应超过这么多字节 —— 用它把相关数据【聚拢】；
//     - 标准只保证二者 >= alignof(std::max_align_t)，且常见实现里
//       destructive >= constructive（典型机器上二者都等于 64）；
//     - 用 destructive 常量做 alignas，验证 alignof 的确被抬到一条缓存行，
//       并对照“紧凑打包”结构体，量化二者的尺寸差异。
//
//   官方参考：
//     - https://en.cppreference.com/w/cpp/thread/hardware_destructive_interference_size
//     - 提案 P0154R1（Hardware interference size）
//     - 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 8 章
//       （数据布局 / 伪共享）
//     - Ulrich Drepper, "What Every Programmer Should Know About Memory"
//
//   编译运行（VS2026, C++20）：
//     cmake --build build-vs2026 --target J2_interference_size --config Release
// =====================================================================
#include "concurrency_study/log.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <new>      // std::hardware_destructive/constructive_interference_size

// ---------------------------------------------------------------------
// 取得两个干扰尺寸常量（带保守回退）。
//
//   __cpp_lib_hardware_interference_size 是判定实现是否提供这两个常量的
//   特性测试宏（feature-test macro）。若缺失（极少数实现），退回 64。
//
//   告警提示：GCC 会对“跨 ABI 边界使用该常量”发 -Winterference-size，
//   理由是该值依赖编译目标、可能影响二进制布局兼容性；建议库 ABI 里写死
//   常量而非直接用它。MSVC（含 VS2026）【没有】这个告警。本练习只在单个
//   可执行内使用，不涉及 ABI 边界，任一实现都安全。
// ---------------------------------------------------------------------
#ifdef __cpp_lib_hardware_interference_size
constexpr std::size_t kDestructive  = std::hardware_destructive_interference_size;
constexpr std::size_t kConstructive = std::hardware_constructive_interference_size;
#else
constexpr std::size_t kDestructive  = 64; // 保守回退。
constexpr std::size_t kConstructive = 64;
#endif

// =====================================================================
// 策略 A —— destructive（破坏性）：隔开，避免伪共享
//
//   两个会被【不同线程】各自高频写的计数器。用 alignas(kDestructive) 把每个
//   推到独立缓存行，让“线程各写各的”不会因共享缓存行而互相失效。
//   语义：我【不希望】这两者落在同一行 —— 它们之间是“破坏性干扰”。
// =====================================================================
struct DestructiveLayout {
    alignas(kDestructive) std::atomic<std::int64_t> writer0{0};
    alignas(kDestructive) std::atomic<std::int64_t> writer1{0};
    // 两个成员各按 kDestructive 对齐 → 分属不同缓存行 → 无伪共享。
};

// =====================================================================
// 策略 B —— constructive（建设性）：聚拢，促进局部性 / 真共享
//
//   一组【总是被同一段代码一起读取】的相关字段（例如一个小记录的若干列）。
//   我们【希望】它们落在同一条缓存行里，一次缓存载入就能全部拿到，提升
//   空间局部性（spatial locality）。做法：把这一簇字段打包，并让整簇按
//   kConstructive 对齐、且合并大小不超过 kConstructive，避免它被切到两行。
//   语义：我【希望】这些字段同处一行 —— 它们之间是“建设性干扰”（共用一行有益）。
// =====================================================================
struct alignas(kConstructive) ConstructiveLayout {
    // 一簇一起访问的热字段：放在一起 → 同一缓存行 → 一次载入命中全部。
    std::int32_t hot_a{0};
    std::int32_t hot_b{0};
    std::int64_t hot_c{0};
    // 提示：真实代码里应静态断言这簇字段总大小 <= kConstructive
    //       （见下方 static_assert），以免它跨越两条缓存行而失去“聚拢”意义。
};

// 紧凑打包对照组：两个计数器相邻、无任何对齐处理（极可能同一缓存行）。
struct PackedLayout {
    std::atomic<std::int64_t> writer0{0};
    std::atomic<std::int64_t> writer1{0};
};

int main() {
    cs::println("==== J2_interference_size：硬件干扰尺寸常量 ====");

    // -----------------------------------------------------------------
    // TODO [必做 1]: 打印两个干扰尺寸常量的值，并说清各自含义。
    //   打印 kDestructive 与 kConstructive；指出 destructive 用于“隔开避免
    //   伪共享”，constructive 用于“聚拢促进真共享/局部性”，二者均
    //   >= alignof(max_align_t)。
    //
    //   参考实现（已启用以保证可编译运行）：
    // -----------------------------------------------------------------
    cs::println("\n---- 两个干扰尺寸常量（实现定义值）----");
    cs::logf("hardware_destructive_interference_size  = ", kDestructive,
             " 字节  （破坏性：不同线程的对象至少相距这么远以【隔开】，避免伪共享）");
    cs::logf("hardware_constructive_interference_size = ", kConstructive,
             " 字节  （建设性：一起访问的对象合并不超过这么大以【聚拢】，促进局部性）");
    cs::logf("alignof(std::max_align_t) = ", alignof(std::max_align_t),
             "  （标准保证：上面两者都 >= 此值）");
#ifndef __cpp_lib_hardware_interference_size
    cs::println("  注意：本实现未提供该常量，上面用的是保守回退 64。");
#endif
    cs::println("  说明：x86-64 上常见实现里二者通常都等于 64（一条缓存行）；具体为实现定义。");

    // -----------------------------------------------------------------
    // TODO [必做 2]: 定义两个对齐策略的结构体并验证 alignof。
    //   （结构体已在上方定义：DestructiveLayout / ConstructiveLayout。）
    //   这里打印它们的 alignof 与 sizeof，并对照紧凑打包版，验证：
    //     - DestructiveLayout 的 alignof == kDestructive，两个计数器被推到
    //       不同缓存行（sizeof 至少 2 * kDestructive）；
    //     - ConstructiveLayout 的 alignof == kConstructive，热字段簇被聚拢、
    //       不超过一条缓存行。
    //
    //   参考实现（已启用以保证可编译运行）：
    // -----------------------------------------------------------------
    cs::println("\n---- 策略 A：destructive（隔开，避免伪共享）----");
    cs::logf("alignof(DestructiveLayout) = ", alignof(DestructiveLayout),
             "（应 == kDestructive = ", kDestructive, "）");
    cs::logf("sizeof(DestructiveLayout)  = ", sizeof(DestructiveLayout),
             "（两个 writer 各占独立缓存行，故 >= 2*kDestructive）");
    static_assert(alignof(DestructiveLayout) == kDestructive,
                  "DestructiveLayout 必须按 destructive 干扰尺寸对齐");

    cs::println("\n---- 策略 B：constructive（聚拢，促进真共享/局部性）----");
    cs::logf("alignof(ConstructiveLayout) = ", alignof(ConstructiveLayout),
             "（应 == kConstructive = ", kConstructive, "）");
    cs::logf("sizeof(ConstructiveLayout)  = ", sizeof(ConstructiveLayout),
             "（热字段簇聚拢在一起，整簇 <= 一条缓存行最佳）");
    static_assert(alignof(ConstructiveLayout) == kConstructive,
                  "ConstructiveLayout 必须按 constructive 干扰尺寸对齐");
    // 一起访问的热字段簇本身不应超过 constructive 尺寸，否则会被切到两行、
    // 失去“聚拢”意义。这里只断言三个字段的裸大小（不含尾部对齐填充）。
    static_assert(sizeof(std::int32_t) + sizeof(std::int32_t) + sizeof(std::int64_t)
                      <= kConstructive,
                  "热字段簇应能装进一条缓存行，否则聚拢无意义");

    cs::println("\n---- 对照组：packed（紧凑打包，无对齐处理）----");
    cs::logf("alignof(PackedLayout) = ", alignof(PackedLayout));
    cs::logf("sizeof(PackedLayout)  = ", sizeof(PackedLayout),
             "（两个计数器挤在一起，多半同一缓存行 —— 正是伪共享温床）");

    // -----------------------------------------------------------------
    // 小结对比
    // -----------------------------------------------------------------
    cs::println("\n---- 一句话区分 ----");
    cs::println("  destructive  = 「离我远点」：被不同线程写的东西要隔到不同缓存行（防伪共享）。");
    cs::println("  constructive = 「凑过来」  ：被同一线程一起读的东西要塞进同一缓存行（提局部性）。");

    // TODO [进阶 1]: 用一个 std::atomic_flag 或 atomic<int> 数组做“分片计数器”，
    //   分别按 (a) 紧凑打包 (b) alignas(kDestructive) 两种布局放置，多线程压测，
    //   量化 destructive 对齐带来的吞吐提升（与 J-1 呼应）。
    //
    // TODO [进阶 2]: 阅读一个真实库（如 folly 的 cacheline.h 或 Boost.Align）
    //   如何封装“缓存行对齐”，对照标准常量的可移植性与库实现的取舍，
    //   并解释 GCC 的 -Winterference-size 告警在“跨 ABI 复用对齐结构体”时
    //   为什么有意义（MSVC 无此告警，注明）。

    cs::println("\n==== 演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
