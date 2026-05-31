// =====================================================================
// 练习 J-3：NUMA 概念与线程亲和性（NUMA & thread affinity）
//   对应文档：Concurrency_Study/13-模块J-缓存与伪共享.md 的 练习 J-3
//
//   学习目标（本题以概念 + 小实验为主，不强求真 NUMA 硬件）：
//     - 说清 NUMA（Non-Uniform Memory Access，非一致内存访问）：在多路
//       服务器上，内存被分成若干 NUMA 节点（node），每个节点“贴着”一组
//       CPU 核心。核心访问【本地节点】内存快，访问【远端节点】内存要走
//       处理器互联（如 QPI/UPI/Infinity Fabric），延迟更高、带宽更低 ——
//       访问代价随“数据在哪个节点”而【不一致】，这正是 NUMA 的字面含义；
//     - 理解 first-touch（首次接触）策略：Linux 等系统默认在“某页内存被
//       某线程【第一次写入】时”，把该物理页分配到【该线程当时所在 CPU 的
//       本地 NUMA 节点】。推论：谁先用，就分给谁的本地节点 —— 所以并行程序
//       应让“将来读写某块数据的线程”去【亲手初始化】那块数据，数据才会落在
//       它的本地节点（而非全由主线程初始化、害得其他线程都成了远端访问）；
//     - 认识线程亲和性（thread affinity）：把线程【绑定】到指定 CPU 核心，
//       既能减少线程在核心间迁移导致的缓存损失，又是“让线程稳定待在某 NUMA
//       节点、配合 first-touch 拿到本地内存”的前提。本题演示 Windows 的
//       SetThreadAffinityMask（用 #ifdef _WIN32 包裹；其他平台给出说明）；
//     - 建立直觉：NUMA 上“数据放哪、线程在哪”和算法本身同等重要；忽视
//       NUMA 局部性，跨节点访问会让多路机器的扩展性大打折扣。
//
//   官方参考：
//     - Windows SetThreadAffinityMask:
//       https://learn.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-setthreadaffinitymask
//     - Windows GetLogicalProcessorInformationEx / NUMA APIs:
//       https://learn.microsoft.com/windows/win32/procthread/numa-support
//     - Ulrich Drepper, "What Every Programmer Should Know About Memory"
//       （第 5 节：NUMA 系统；first-touch 与内存放置）
//     - 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 8 章
//       （数据局部性 / 在多核间划分数据）
//
//   编译运行（VS2026, C++20）：
//     cmake --build build-vs2026 --target J3_numa_concept --config Release
// =====================================================================
#include "concurrency_study/log.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

#ifdef _WIN32
// 仅 Windows 提供 SetThreadAffinityMask 等亲和性 API。
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif

// ---------------------------------------------------------------------
// 把【当前线程】绑定到第 cpu_index 号逻辑核（亲和性掩码只置该核对应的位）。
//   返回是否设置成功。非 Windows 平台直接返回 false 并由调用方打印说明。
//
//   说明：亲和性掩码（affinity mask）是一个位图，第 k 位为 1 表示“允许本线程
//   在第 k 号逻辑处理器上运行”。这里只置一位 = 把线程钉死在单个核上。
//   真实 NUMA 调优中，会先用 GetLogicalProcessorInformationEx / GetNumaNode...
//   查出“哪些核属于哪个 NUMA 节点”，再把线程绑到目标节点的核，配合
//   first-touch 让数据落在该节点本地内存。本题只演示“绑核”这一步。
// ---------------------------------------------------------------------
bool pin_current_thread_to_cpu(unsigned cpu_index) {
#ifdef _WIN32
    // DWORD_PTR 的位宽决定一个亲和性组最多管多少逻辑核（32/64）。
    if (cpu_index >= sizeof(DWORD_PTR) * 8) return false;
    const DWORD_PTR mask = (static_cast<DWORD_PTR>(1) << cpu_index);
    // 返回旧掩码；0 表示失败。
    const DWORD_PTR prev = SetThreadAffinityMask(GetCurrentThread(), mask);
    return prev != 0;
#else
    (void)cpu_index;
    // 其他平台：Linux 用 pthread_setaffinity_np + cpu_set_t；
    //          macOS 仅有 thread affinity policy（建议性、不保证）。
    return false;
#endif
}

// 一段“纯算”的热负载：把数组累加很多遍。仅用于让被绑核的线程实际占用 CPU，
// 便于你在任务管理器/性能监视器里观察它是否真的只跑在被指定的那个核上。
std::int64_t crunch(const std::vector<std::int64_t>& data, int passes) {
    std::int64_t acc = 0;
    for (int p = 0; p < passes; ++p) {
        for (std::int64_t x : data) acc += x;
    }
    return acc;
}

int main() {
    cs::println("==== J3_numa_concept：NUMA 概念与线程亲和性 ====");

    const unsigned hw = std::thread::hardware_concurrency();
    cs::logf("本机 hardware_concurrency() = ", hw, " 个逻辑核。");
#ifdef _WIN32
    cs::println("平台：Windows —— 使用 SetThreadAffinityMask 演示绑核。");
#else
    cs::println("平台：非 Windows —— 本演示的绑核步骤将被跳过（见代码内说明）。");
#endif

    // -----------------------------------------------------------------
    // 第一部分：概念讲解（输出到 stdout，便于边跑边读）
    // -----------------------------------------------------------------
    cs::println("\n---- 概念速览 ----");
    cs::println("  NUMA：内存按节点(node)划分，每节点贴着一组核。本地访问快、跨节点访问慢。");
    cs::println("  first-touch：物理页在【首次写入】时被分到【写入线程所在节点】的本地内存。");
    cs::println("    => 让将来使用数据的线程去【亲手初始化】数据，数据才落在它的本地节点。");
    cs::println("  affinity（亲和性）：把线程绑到固定核，减少迁移导致的缓存损失，");
    cs::println("    并让线程稳定待在某 NUMA 节点，是配合 first-touch 拿本地内存的前提。");

    // -----------------------------------------------------------------
    // TODO [必做 1]: 用线程亲和性把若干线程各绑到不同核并运行，观察/说明。
    //   开 N 个线程（N = min(hw, 4)），第 i 个线程：
    //     (a) 调 pin_current_thread_to_cpu(i) 把自己绑到第 i 号核；
    //     (b) 跑 crunch(...) 一段热负载；
    //     (c) 打印自己绑核成功与否、算出的结果。
    //   join 后主线程汇总。
    //   观察建议：运行时打开任务管理器/资源监视器，看各线程是否分别压在不同核上。
    //
    //   （绑核成功与否取决于平台与权限；非 Windows 会打印“跳过”。）
    //
    //   参考实现（已启用以保证可编译运行）：
    // -----------------------------------------------------------------
    cs::println("\n---- 必做 1：把线程绑到不同核（first-touch 的前置：让线程稳定待在某节点）----");
    const unsigned n = (hw == 0) ? 2u : (hw < 4u ? hw : 4u);
    std::vector<std::thread> threads;
    std::atomic<int> pinned_ok{0};

    for (unsigned i = 0; i < n; ++i) {
        threads.emplace_back([i, &pinned_ok] {
            const bool ok = pin_current_thread_to_cpu(i);
            if (ok) pinned_ok.fetch_add(1, std::memory_order_relaxed);

            // first-touch 思想的微缩演示：这块数据由【本线程】亲手初始化，
            // 在真 NUMA 机上它会被分到本线程所在核的本地节点 —— 后续本线程
            // 访问它即为本地访问。（单节点机器上无可观测差异，但写法是对的。）
            std::vector<std::int64_t> local(1 << 16);
            for (std::size_t k = 0; k < local.size(); ++k) {
                local[k] = static_cast<std::int64_t>(k + i); // 首次写 => first-touch
            }

            const std::int64_t r = crunch(local, 64);
            cs::logf("[thread ", i, "] 绑核 ", (ok ? "成功" : "跳过/失败"),
                     "（目标核 #", i, "），本地数据自算结果摘要=", (r & 0xffff));
        });
    }
    for (auto& t : threads) t.join();
    cs::logf("[main] 成功绑核线程数 = ", pinned_ok.load(), " / ", n,
             "（非 Windows 或权限不足时可能为 0，属正常）。");

    // -----------------------------------------------------------------
    // 第二部分：first-touch 的对比思路（说明为主，单节点机上无法量化）
    // -----------------------------------------------------------------
    cs::println("\n---- first-touch 对比思路（真 NUMA 机上才有可测差异）----");
    cs::println("  反例：主线程一次性 new + 初始化整个大数组（全部页落在主线程的本地节点），");
    cs::println("        再分给各 worker 处理 —— 远端 worker 全程跨节点访问，慢。");
    cs::println("  正解：先分好每个 worker 负责的分片，让【各 worker 自己】初始化其分片");
    cs::println("        （first-touch 把页分到各自本地节点），再各自处理 —— 全本地访问，快。");
    cs::println("  在真 NUMA 服务器上把两种写法计时对比，即可看到正解明显胜出；");
    cs::println("  单 NUMA 节点的台式机/笔记本上两者无差异（无远端节点可言）。");

    cs::println("\n  Windows 上查询 NUMA 拓扑可用 GetLogicalProcessorInformationEx、");
    cs::println("  GetNumaHighestNodeNumber、VirtualAllocExNuma（按节点分配内存）等 API；");
    cs::println("  本题不展开这些查询，聚焦概念与绑核演示。");

    // TODO [进阶 1]: 在真 NUMA 机上实现上面“反例 vs 正解”的两种初始化+遍历，
    //   用 steady_clock 计时对比，量化 first-touch 的收益。
    //
    // TODO [进阶 2]: 用 VirtualAllocExNuma（Windows）或 numa_alloc_onnode（Linux,
    //   libnuma）显式把内存分配到指定节点，结合 SetThreadAffinityMask 把线程绑到
    //   同节点的核，构造“本地访问 vs 强制远端访问”的对照实验。

    cs::println("\n==== 演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
