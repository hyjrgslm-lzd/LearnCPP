# 08 模块 F：协程帧与 allocator

## 模块目标

前面模块让你看懂了 promise_type 的 8 个 hook 和 co_await 的三步变换。这个模块要把你拉到编译器底层：

- 亲手用编译器 flag 打印协程帧（coroutine frame）的布局，看清楚的参数、局部变量、resume-point 索引在帧内的排布
- 实现 P0912 风格的自定义 promise allocator，把帧分配到预分配 pool 上，统计分配/释放次数
- 触发并诊断 HALO（Heap Allocation eLision Optimization），理解"帧不逃逸"是如何让编译器把堆分配直接优化为栈分配的

如果你跳过这一层，协程帧就始终是一个黑盒，"为什么协程比回调慢"或"为什么这里没有堆分配"之类的问题就永远只能靠猜。

## 模块完成标准

做完本模块，你至少要能稳定说清楚：

- 协程帧不是黑盒：promise、参数副本、局部变量 spill 区、resume-point 索引，在编译器中都是有明确布局的。
- P0912 的 `operator new`/`operator delete` 重载机制让 promise 可以参与帧分配决策，而不是被动接受默认的 `::operator new`。
- HALO 的触发前提是"协程帧在所有 co_await 点之后的调用图中都不逃逸"。破坏任何一个前提，HALO 就静默失败。
- HALO 的性能差异是数量级的：无 HALO 单次协程调用 50-200ns，有 HALO 可以压到 5-20ns。
- 跨编译器的 frame 布局和 HALO 触发能力差异显著，工程中必须针对目标编译器实测。

## 使用建议

- F-1 不要求写很多新代码，重点在于读出编译器 dump 并标注帧结构。
- F-2 需要写一个简单的 pool allocator，建议先做单线程版本。
- F-3 的精华在于"对比两版代码的编译器输出"，不要只跑一遍就说"HALO 没触发"。
- 如果你只能用一个编译器，至少把该编译器的帧 dump 和 HALO 报告读透。

---

## 练习 F-1：观察 frame 布局

### 目标

用编译器 flag 打印协程帧的内存布局，识别帧内的四大区域——promise、参数副本、resume-point 索引、局部变量 spill 区——把"协程帧"从抽象概念变成可观测的结构体。

### 前置理解

- 你已经完成模块 D 和 E，理解协程体的每一步都会编译为一个状态机的状态转换。
- 你知道每个协程调用都会在堆上（或经过 HALO 后在栈上）分配一个 coroutine frame。
- 你知道 co_await 表达式是"挂起点"——编译器在挂起点前后插入状态保存和恢复代码。
- 你接受这题的重点是观察帧布局的结构，不是记住某个特定编译器的 flag 语法。

### 必做任务

1. **准备被观察的协程**：写一个包含以下元素的简单协程，使帧布局足够丰富：

   ```cpp
   #include <coroutine>
   #include <cstdio>
   #include <string>

   struct observer_task {
       struct promise_type {
           observer_task get_return_object() { return {}; }
           std::suspend_never initial_suspend() { return {}; }
           std::suspend_always final_suspend() noexcept { return {}; }
           void return_void() {}
           void unhandled_exception() {}
       };
   };

   // 待观察的协程体
   observer_task observed(int param_a, double param_b, std::string param_c) {
       int local_x = param_a * 2;               // 局部变量，需要 spill
       double local_y = local_x + param_b;       // 另一个局部变量
       co_await std::suspend_always{};           // 挂起点 1
       std::string local_str = param_c;          // 非 trivially destructible 局部变量
       co_await std::suspend_always{};           // 挂起点 2
       printf("%d %f %s\n", local_x, local_y, local_str.c_str());
       co_return;
   }
   ```

2. **GCC 用户**：用 `-fdump-tree-coro` 编译，在 dump 文件中搜索协程帧结构体（通常名字包含 `_Coro_frame` 或 `__frame`）。标注其中：
   - promise 字段（通常位于帧首部附近）
   - 三个参数的副本
   - resume-point 索引（通常是一个整数，记录当前执行到哪个 co_await 点）
   - `local_x`、`local_y`、`local_str` 的 spill 位置

3. **MSVC 用户**：用 `/d1reportSingleClassLayout` 编译，在输出中搜索协程帧的类布局报告。MSVC 的帧通常是一个嵌套在 `promise_type` 附近的 struct，标注同上四个区域。

4. **Clang 用户**：用 `-Xclang -ast-dump` 或 `-emit-llvm` 编译，在 AST/LLVM IR 中定位帧的类型定义。

5. **标注帧布局图**：用手绘或 ASCII 图标注帧中各区域的相对位置和大小：

   ```
   +---------------------------+ <-- frame base
   | promise                   |  (通常在最前面)
   +---------------------------+
   | resume_index              |  (整数，记录当前状态)
   +---------------------------+
   | param_a (copy)            |
   | param_b (copy)            |
   | param_c (copy)            |  参数副本
   +---------------------------+
   | local_x (spill)           |
   | local_y (spill)           |
   | local_str (spill)         |  局部变量 spill 区
   +---------------------------+
   ```

   注意：不同编译器的实际布局顺序可能不同，以你实际 dump 出的结果为准。

6. **观察参数副本的生命周期含义**：
   - 为什么 `param_c` 是 `std::string` 这样非平凡类型的参数会被拷贝到 frame 中，而不是直接在调用栈上操作？
   - 如果你把参数改为按引用传递（`const std::string& param_c`），但协程体在 co_await 后使用它，会发生什么？
   - 记录你的观察。

7. **观察 resume-point 索引**：
   - 追踪 co_await 点数量与 resume_index 可能值的对应关系。
   - 思考：为什么 resume_index 必须是 frame 内的一个字段，而不能是"当前 IP 算出来的"？（提示：协程可以被 resume 执行后又被 destroy，resume 时协程体内的代码不一定在运行。）

8. **为什么会默认堆分配**：协程帧的大小在编译期确定，但帧必须在多次 suspend/resume 之间存活——这意味着它的生命周期跨越了创建函数的栈帧。因此编译器默认走 `operator new`。

### 进阶任务

- 往协程体里添加更多类型的局部变量（`std::vector`、`std::unique_ptr`、一个自定义的非平凡析构类型），观察帧大小的变化和那些类型在帧中的位置。
- 改变你的协程体中 co_await 点的数量和位置，观察 resume_index 的合法取值范围是否随着变化。
- 对比三个编译器的帧大小：为什么同一个协程在不同编译器下帧大小可能差 30% 以上？
- 如果你有两台不同架构的机器（x86-64 vs ARM64），对比帧布局差异。

### 验收点

- 你能从编译器 dump 中找到协程帧的结构体定义。
- 你能在帧中标注出 promise、参数副本、resume_index、局部变量 spill 区四个区域。
- 你能解释参数按值传递时，参数副本为什么必须存在于帧中。
- 你能解释为什么协程帧默认走堆分配，而不可能完全在栈上。
- 你至少做了一个编译器 flag 的帧 dump 并完成标注。

### 观察点

- 协程帧本质上是编译器为你自动生成的一个 struct——你把 co_await 前后的代码想象成这个 struct 的成员函数，而结构体本身会存活到协程结束。
- 帧的大小 = promise 大小 + resume_index + 所有参数副本大小 + 所有溢出到帧的局部变量大小 + 对齐 padding。不同编译器的 padding 策略不同，所以帧大小也不同。
- resume_index 的重要性怎么强调都不为过——没有它，协程被 resume 时根本不知道应该跳到哪个 co_await 之后继续执行。
- 非平凡类型（如 `std::string`）的析构函数必须在帧销毁时被调用，这就是为什么帧的析构函数是编译器生成的——它遍历所有活跃的局部变量并调用析构。

### 常见坑

- 编译器 flag 打错了导致看不到 dump 输出。GCC 的 `-fdump-tree-coro` 输出到 `.coro` 文件而非标准输出。
- MSVC 的 `/d1reportSingleClassLayout` 只对特定类名有效——你需要从编译器错误信息或 `/d1reportAllClassLayout` 中找到协程帧的准确类名（通常是 mangled name）。
- Clang 的 AST dump 非常冗长，直接搜索 `coroutine` 或 `await` 来缩小范围。
- 看到帧 struct 定义后，误以为所有字段的顺序就是内存中的顺序——编译器可能重排字段以优化对齐。
- 把"协程帧"和"协程的栈帧"混为一谈：协程帧是独立分配的，协程体执行时会把帧作为上下文，但协程体内部的普通函数调用仍有自己的栈帧。

### 提示

- 先在协程体内只放一个 co_await 点，编译，看帧大小。再增加到两个、三个，对比 resume_index 的变化。
- 如果你用 GCC，`-fdump-tree-coro` 会生成一个 `.c.022t.coro` 或类似文件名的 dump 输出。里面会有注释标明哪些变量溢出到了 frame。
- MSVC 用户可以在 Godbolt 上用 x86-64 msvc v19.latest + `/d1reportSingleClassLayout` 看到完整的类布局而不需要本地安装特定版本。
- 标注帧布局时不需要使用精确的字节偏移——标注出字段的相对顺序已经足够。

### 复盘问题

- 为什么协程帧必须是一个独立于调用栈的、可长期存活的对象？
- 如果协程帧从堆分配改为池分配，哪些代码需要改变？
- resume_index 为什么不能用"当前 coroutine_handle 关联的代码地址"来替代？
- 从帧布局的角度，为什么 HALO 在帧不逃逸时是可行的优化？
- 如果编译器不做帧分析优化，协程带来的开销主要是哪些部分？（提示：三部分——堆分配、帧析构的非平凡类型遍历、resume 时的状态分发。）

### 对应官方参考

- GCC `-fdump-tree-coro` 文档
- MSVC `/d1reportSingleClassLayout` 调试 flag
- Clang `-Xclang -ast-dump` 文档
- Gor Nishanov "C++ Coroutines: Under the covers" CppCon 2016
- Andreas Fertig 《Programming with C++20》第 8 章（协程帧与 HALO）

---

## 练习 F-2：自定义 promise allocator

### 目标

实现 P0912 风格的自定义 promise allocator：promise 通过重载 `operator new`/`operator delete` 接管帧分配，将帧定向到一个简单的 pool allocator。通过统计分配/释放次数和帧大小，建立"协程帧分配 = 可定制"的直觉。

### 前置理解

- 你已经完成 F-1，理解协程帧的结构和默认堆分配路径。
- 你知道 P0912R5 的机制：promise_type 可以重载 `static void* operator new(size_t size)`，编译器在分配 frame 时会优先调用这个重载而非全局 `::operator new`。
- 你知道 `get_return_object_on_allocation_failure()` 是 P0912 的兜底——如果 `operator new` 抛异常或返回 nullptr（nothrow 版本），promise 可以通过这个函数返回一个"失败态"的 task。
- 你接受这题先做单线程 pool，不追求多线程安全或复杂分配策略。

### 必做任务

1. **实现一个最小 `coroutine_pool`**：

   ```cpp
   #include <cstddef>
   #include <vector>
   #include <cassert>

   class coroutine_pool {
       struct block {
           size_t size;
           bool in_use = false;
           alignas(std::max_align_t) unsigned char data[];
       };

       std::vector<std::unique_ptr<unsigned char[]>> buffers_;
       // 简化版：预分配 64KB，用 bump allocator
       unsigned char* memory_;
       size_t total_size_;
       size_t used_ = 0;
       size_t alloc_count_ = 0;
       size_t free_count_ = 0;

   public:
       explicit coroutine_pool(size_t size = 65536)
           : total_size_(size)
           , memory_(new unsigned char[size])
       {}

       void* allocate(size_t size) {
           assert(used_ + size <= total_size_);
           void* ptr = memory_ + used_;
           used_ += size;
           ++alloc_count_;
           return ptr;
       }

       void deallocate(void* /* ptr */, size_t /* size */) {
           // bump allocator 不支持 free
           ++free_count_;
           // 生产级实现应该支持回收
       }

       void print_stats() const {
           printf("[pool] allocs=%zu frees=%zu used=%zu/%zu bytes\n",
                  alloc_count_, free_count_, used_, total_size_);
       }
   };
   ```

   这是一个 bump-up allocator，无法回收内存。它的目的是统计分配次数和大小——真正的生产级 allocator 用 free-list 或 slab。

2. **为 promise_type 添加自定义 operator new/delete**：

   ```cpp
   struct pool_task {
       struct promise_type {
           // P0912: 自定义 operator new —— 编译器优先调用此重载
           static void* operator new(size_t size) {
               return global_pool.allocate(size);
           }

           // 对应的 operator delete
           static void operator delete(void* ptr, size_t size) {
               global_pool.deallocate(ptr, size);
           }

           pool_task get_return_object() { /* ... */ }
           std::suspend_always initial_suspend() { return {}; }
           std::suspend_always final_suspend() noexcept { return {}; }
           void return_void() {}
           void unhandled_exception() {}
       };
   };
   ```

3. **写 10 个短协程，在循环中创建并运行它们**，运行后打印 pool 统计：

   - 每次迭代创建协程、resume 到完成、销毁 frame
   - 观察分配次数和总分配量是否与协程帧大小的预期一致
   - 对比使用默认 `::operator new` 和自定义 pool 时的分配行为差异

4. **实现 `get_return_object_on_allocation_failure`** 兜底：

   ```cpp
   static pool_task get_return_object_on_allocation_failure() {
       // 分配失败时返回一个"错误态"的 task
       // 或者抛异常
       throw std::bad_alloc();
   }
   ```

   这个函数在 `operator new` 失败（抛异常或返回 nullptr）后被自动调用。它让你有机会返回一个"失败态"task。

5. **观测帧大小**：在 `operator new(size_t size)` 中打印 `size`，对不同的协程体观测帧大小：
   - 只有一个 co_await + 一个 int 的协程，帧大小约多少？
   - 有 5 个局部变量 + 3 个 co_await 点的协程，帧大小增加了多少？
   - 局部变量中包含 `std::string` 或 `std::vector` 时，帧大小是否只增加对象大小，还是包括额外的间接开销？

6. **对比**:把 `pool_task` 替换为用默认 `::operator new` 的普通 task，用 perf/VTune/simple-timing 测一下 10000 次协程创建/销毁：
   - pool 版本的耗时（应在几微秒级）
   - 默认 `::operator new` 的耗时（通常在几十微秒级）
   - 分析差异来自何处

### 进阶任务

- 把 bump allocator 升级为 slab allocator：预分配 N 个固定大小的槽位，支持 free 后复用。验证帧分配/释放的配对性。
- 添加线程安全支持：用 `std::atomic` 或 `thread_local pool` 使 pool 在多线程环境中安全使用。
- 让 `pool_task` 同时支持 `co_await`（即把它变成一个完整的 task 类型而非仅用于观测的骨架）。
- 研究 `stdexec/exec/task.hpp` 中 promise_type 的 allocator 接入方式，与你的实现做对比。

### 验收点

- 你的自定义 `operator new`/`operator delete` 被编译器实际调用（在打印统计中可观测）。
- pool 统计正确反映了协程创建次数和帧总大小。
- 你理解了 `get_return_object_on_allocation_failure` 的触发时机和作用。
- 你能说明为什么 P0912 的 allocator 定制机制比"全协程序改默认 new"更精准——因为每个 promise_type 可以有不同的 allocator。
- 你跑出了 pool 版本和默认 new 版本的性能对比数据。

### 观察点

- P0912 的设计精妙之处在于：allocator 附着在 promise_type 上，而不是附着在 task 类型上。这意味着同一个 task 类型的不同 promise_type 可以对帧分配做出不同决定。
- 帧大小在编译期是完全确定的（虽然 HALO 会在运行期消除分配）。因此 pool 可以预分配精确匹配帧大小的槽位。
- 帧分配是协程性能的一个重要因素，但不是唯一的因素——非平凡局部变量的析构、resume 时的状态分发、多级 indirect call 都可能更贵。
- 在"大量短生命周期协程"的场景（如 HTTP 请求处理），pool allocator 可以显著降低 malloc/free 开销。

### 常见坑

- operator new 签名写错导致编译器 fallback 到默认 operator new（静默失败）。正确签名为 `static void* operator new(size_t)`。
- operator delete 忘记写或不匹配，导致编译器报 linkage 错误或静默使用默认 delete。
- pool 不是 thread_local 的，在多线程环境中的统计被并发访问污染。
- bump allocator 在连续创建大量协程后 OOM——因为回收不了内存。练习目的不是做一个生产 pool，而是观测分配模式。
- 在 `operator new` 里做重操作（如 printf + flush），改变计时结果——观测代码本身变成了性能瓶颈。

### 提示

- 先写一个只打印 "operator new called, size = X" 的 `operator new`，确认编译器调用了它。
- pool 统计打印放在 main 函数末尾，不要在 `operator new` 里每分配一次就打印一次——那样会淹没有用信息。
- 帧大小受编译器版本和编译选项影响——比如 debug 模式下帧会比 release 大很多。确保你的观测环境一致。
- 如果对 allocator 不熟，先复习一下 C++17 的 `std::pmr::memory_resource` 概念——协程 allocator 的设计思路和它同源。

### 复盘问题

- 为什么 P0912 把 allocator 设计为 promise_type 的成员重载，而不是一个独立的 traits 模板参数？
- 如果 pool allocator 的槽位固定为 128 字节，而某个协程的帧是 200 字节，会发生什么？
- `get_return_object_on_allocation_failure` 的存在意味着什么设计态度？——框架不假设分配总是成功的。
- 为什么生产级的协程框架（如 folly coro）几乎都提供了 allocator 定制能力？
- 从 P0912 的设计中，你能看到 C++ 标准化过程中"不破坏现有代码"和"允许最大灵活性"之间的权衡吗？

### 对应官方参考

- P0912R5: "Coroutines with allocator support"
- cppreference: `coroutine_traits` 和 `promise_type::operator new`
- Lewis Baker "Custom allocators for C++ coroutines" blog
- Andreas Fertig 《Programming with C++20》第 8.5 节（allocator）

---

## 练习 F-3：HALO 触发与失败诊断

### 目标

写两版 generator 消费代码——一版帧不逃逸（HALO 可触发）、一版故意破坏 HALO 前提——用编译器 flag 诊断 HALO 是否触发，并实测性能差异。建立"HALO 不是一个黑盒魔法，而是条件明确的编译期优化"的直觉。

### 前置理解

- 你已经完成 F-1 和 F-2，理解协程帧的布局和分配机制。
- 你知道 HALO（Heap Allocation eLision Optimization）的原理：如果编译器能够证明协程帧的地址不会逃逸到 co_await 之后的外部代码，就可以把帧分配从堆移到调用者的栈帧中。
- HALO 的触发前提（必要条件，不是充分条件）：
  1. 协程返回类型不是拥有 frame 的类型（如 `std::generator` 不拥有 frame，但 `co_await` 它的函数可能有自己的帧）。
  2. 协程帧的地址从未传递给任何"可能逃逸"的上下文（如存到全局变量、传给 opaque 函数、作为返回值的一部分）。
  3. 所有 co_await 点之后的调用路径中，帧地址不逃逸。
- 你接受这题的目标是"触发 HALO 或诊断为什么没触发"，而不是"保证每台机器上一定会触发 HALO"。跨编译器的 HALO 能力差异很大。
- 关键性能数据点（来自 Gor Nishanov CppCon 2018）：
  - **无 HALO**：协程创建 + destroy = 50-200ns（取决于帧大小和 malloc 实现）
  - **有 HALO**：协程创建 + destroy = 5-20ns（本质上是调整栈指针 + 几个 mov）

### 必做任务

1. **写版本 A：HALO 可触发的 generator 消费代码**：

   ```cpp
   #include <generator>
   #include <cstdio>

   // 简单 generator —— 帧地址从不逃逸
   std::generator<int> simple_range(int n) {
       for (int i = 0; i < n; ++i) {
           co_yield i;
       }
   }

   // 关键：generator 本身不持有 frame 地址，
   // 且在消费代码中 frame 地址从未逃逸到外部
   void consumer_A() {
       for (int val : simple_range(10)) {   // 范围 for —— 局部消费
           printf("%d\n", val);             // 帧地址不逃逸
       }
   }
   ```

2. **写版本 B：故意破坏 HALO 前提**：

   ```cpp
   // 版本 B：把 generator 引用存到全局——帧地址逃逸
   std::generator<int>* g_storage = nullptr;

   std::generator<int> leaker() {
       for (int i = 0; i < 5; ++i) {
           co_yield i * 10;
       }
   }

   void consumer_B() {
       auto g = leaker();
       g_storage = &g;    // 帧地址逃逸到全局变量！HALO 无法触发
       for (int val : g) {
           printf("%d\n", val);
       }
       g_storage = nullptr;
   }
   ```

3. **Clang 用户诊断 HALO**：用 `-Rpass=coroutine-elide` 或 `-Rpass-analysis=coroutine-elide` 编译：
   ```bash
   clang++ -std=c++23 -O2 -Rpass=coroutine-elide coro_test.cpp -o coro_test
   ```
   - 期望版本 A 的输出中出现类似 `coroutine frame elided` 的 remark
   - 期望版本 B 要么没有 remark，要么出现 `coroutine frame cannot be elided because ...` 的 remark

4. **GCC 用户诊断 HALO**：用 `-fdump-tree-coro` 检查 dump 输出，搜索 `elide` 或检查 frame 分配是否从 `operator new` 变成了栈分配。GCC 14+ 的 HALO 能力在逐步增强，但不是所有模式都支持。

5. **MSVC 用户诊断 HALO**：MSVC 从 VS 2019 16.8 开始支持 HALO。在 Release build 下用 `/d2CoroElideInfo` （如果可用）或在 debugger 中观察帧分配的代码路径。

6. **性能实测**：对两版代码分别跑 1000 万次迭代，用 `std::chrono::high_resolution_clock` 计时：

   ```cpp
   auto start = std::chrono::high_resolution_clock::now();
   for (int i = 0; i < 10'000'000; ++i) {
       consumer_A();  // 或 consumer_B()
   }
   auto end = std::chrono::high_resolution_clock::now();
   auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
   printf("time per iteration: %ld ns\n", ns / 10'000'000);
   ```

   - 记录版本 A（可能 HALO）和版本 B（不可能 HALO）的每次迭代耗时。
   - 如果差异不大（< 2x），说明 HALO 在两个版本里都没触发——需要进一步诊断原因。
   - 如果版本 A 显著更快（5-10x），说明 HALO 发挥了作用。

7. **更多破坏 HALO 的模式**：
   - 把协程的 `coroutine_handle` 存到一个容器中（如 `std::vector`）
   - 在 co_await 之后访问一个通过引用捕获的外部变量
   - 协程返回类型中包含 `std::coroutine_handle<>` 作为成员
   - 每个模式都对应一条"帧地址逃逸"的路径

8. **记录诊断结论**：你的编译器在哪些条件下可以触发 HALO？哪些条件是你预期会触发但实际上没触发？

### 进阶任务

- 尝试在不同优化级别（-O0、-O1、-O2、-O3）下观察 HALO 的触发情况。你可能会发现 -O0 下 HALO 几乎从不触发。
- 如果你有多个编译器可用（Clang 和 GCC），对比两者的 HALO 诊断能力差异。
- 研究 `std::generator` 的源码实现（libstdc++ 或 libc++），观察它为什么被设计为不持有 frame 所有权——这正是 HALO 的关键前提之一。
- 用 `perf stat` 或硬件计数器统计两版代码的 L1 cache miss 和 branch misprediction——HALO 的好处不仅仅是省了 malloc，还包括更好的局部性。

### 验收点

- 你能写出两个版本的消费代码：一版 HALO 可触发，一版 HALO 被阻断。
- 你用编译器 flag 观察到了 HALO 触发（或确认未触发及其原因）。
- 你跑出了两个版本的性能数据，并能解释差异的来源。
- 你能列举至少三条"破坏 HALO"的代码模式。
- 你能向同事解释：为什么 HALO 不是"编译器自动做的魔法"，而是需要满足严格前提的优化。

### 观察点

- HALO 的本质是逃逸分析（escape analysis）——编译器要证明帧地址不会逃逸到比协程寿命更长的上下文。这和 Java 的逃逸分析、Go 的 escape analysis 是同一个思路。
- C++ 需要 HALO 的核心原因是"协程帧默认堆分配"，而其他语言（如 Rust）的协程帧本来就是放在调用栈上的。C++ 历史包袱导致了 HALO 是一个"补偿性优化"。
- HALO 的触发条件非常脆弱——哪怕是一个看似无害的 `std::vector::push_back(coro_handle)` 调用，都可能导致 HALO 静默失败。
- `std::generator` 的 `begin()`/`end()` 返回迭代器而不暴露 `coroutine_handle`，这不是偶然的设计——它就是为了最大化 HALO 的触发机会。

### 常见坑

- 在-O0 或 debug 模式下期望 HALO 触发——几乎所有 HALO 实现都要求至少 -O1 或 -O2。
- 把 `consumer_A` 写成接受一个 `const std::generator<int>&` 参数的函数——参数跨函数边界，编译器做逃逸分析会变保守。
- 在范围 for 循环内部又 `co_await` 了其他东西——这引入了新的协程嵌套，逃逸分析范围被打破。
- 性能计时被 printf 的 I/O 时间主导，完全看不到 HALO 的影响。正确做法：计时时 minimize I/O，甚至完全去掉 printf。
- Clang 的 `-Rpass=coroutine-elide` 在某些版本中需要 `-O2` 才生效——如果在 -O1 下没有 remark，升到 -O2 试试。
- 用 `std::generator<T>` 返回引用（`co_yield some_reference;`）——帧中出现对局部变量的引用，这会显著改变逃逸分析结果。

### 提示

- 如果你的编译器不支持 HALO 诊断 flag，可以从"版本 A 和 B 的性能是否有显著差异"来反推 HALO 状态。
- 用 Godbolt 写版本 A，看生成的汇编：如果在协程创建点看不到 `call operator new`，说明 HALO 触发了。
- 尝试在协程体里不写任何 co_await 之外的东西——极简的协程体最容易触发 HALO。然后再逐步加代码，观察哪一步让 HALO 消失。
- 性能数据点 50-200ns vs 5-20ns 来自 CppCon 2018 的 benchmark。你的实际数字可能因为硬件和编译器版本有所不同，但数量级差异应该一致。

### 复盘问题

- 为什么 HALO 的实现需要逃逸分析，而不仅仅是"协程创建点和消费点在同一个函数内"？
- 如果 HALO 在某个编译器版本上不触发，你会怎么向团队成员解释并建议应对策略？
- 为什么 C++ 需要 HALO（补偿性优化），而 Rust 的协程实现不依赖这种优化？
- HALO 的脆弱性对协程在生产代码中的使用有什么实际影响？——你是否应该依赖 HALO 来保证性能？
- 在什么场景下，就算没有 HALO，协程的性能也足够好？

### 对应官方参考

- Gor Nishanov "HALO: Heap Allocation eLision Optimization" CppCon 2018
- Clang `-Rpass=coroutine-elide` 文档
- Andreas Fertig 《Programming with C++20》第 8.4 节（HALO）
- libstdc++ / libc++ `std::generator` 实现源码

---

## 做完模块 F 之后，你现在应该能说清楚什么

至少把下面几句话说顺：

- 协程帧不是黑盒。用编译器 flag 可以打印完整的帧布局：promise 在前，然后是 resume_index、参数副本、局部变量 spill 区。帧的析构函数由编译器生成，负责按顺序析构所有已构造的局部变量。
- P0912 通过 promise_type 的 `operator new`/`operator delete` 重载实现了帧分配定制。同一个 task 类型的不同 promise_type 可以使用不同的 allocator，而框架不需要感知这一层。
- HALO 让编译器在"帧不逃逸"的前提下把堆分配优化为栈分配。性能差异是数量级的（50-200ns vs 5-20ns），但触发条件脆弱——任何一个帧地址逃逸路径都会静默破坏 HALO。
- 跨编译器的 frame 布局和 HALO 能力差异显著。工程中不能假设"某个编译器会 HALO"，必须对目标编译器实测。