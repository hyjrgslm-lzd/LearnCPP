# 练习 I-3：RCU 读-拷贝-更新

> 详尽版见 `../../11-模块I-安全内存回收.md` 的 练习 I-3。

## 目标

用 RCU（read-copy-update，读-复制-更新）实现一个**读多写少**的共享配置：读者侧几乎零开销（不加锁，只标记一下“我在读临界区”），写者侧 publish 新版本 + retire 旧版本，等宽限期（grace period，所有当前读者都离开）后才安全回收旧版本。

本题使用本仓库自带的**教学版** RCU `cs::rcu_*`（`concurrency_study/rcu.hpp`）。

## 前置理解

- **读侧协议**：`rcu_domain::lock/unlock` 包住“读指针 + 解引用使用”的整个临界区；在临界区内读到的指针，保证在临界区结束前不会被回收，因此读者无需加锁也不会 use-after-free。
- **写侧协议**：构造新版本（copy）→ 原子 `publish`（替换全局指针，release 与读者 acquire 配对）→ `rcu_retire`/`obj.retire()` 旧版本（**不立即 delete**）→ `rcu_synchronize`/`rcu_barrier` 等宽限期结束 → 旧版本被安全回收。
- **与 hazard pointer（I-2）对比**：hazard pointer 精确登记“我在用哪个指针”；RCU 用更粗的“等老读者全离开”的宽限期。RCU 读侧更便宜，但回收延迟更依赖最慢的读者。
- **教学版说明**：C++26 在 `std` 命名空间 `<rcu>` 标准化了等价设施（提案 `P2545R4`），但 MSVC VS2026 尚未实现，故用 `cs::` 教学版。`cs::` ↔ `std::` API 差异映射见模块文档对照表。

## 必做任务

1. `// TODO [必做 1]`（读侧）：用 `rcu_default_domain().lock()/unlock()` 包住 `g_config.load()` + 解引用，并校验不变量 `checksum == a + b`。
2. `// TODO [必做 1]`（写侧）：复制新版本 → `g_config.exchange(fresh)` publish → 旧版本 `retire()`（或 `cs::rcu_retire(old)`）→ 周期性 `rcu_barrier()` 等宽限期后回收。
3. 验收：读者全程读到自洽版本（不变量始终成立），无 use-after-free。

## 验收点

- 读者无锁读、写者 publish + retire，全程无崩溃、不变量始终成立。
- 能说清读临界区为何能保证“区内读到的指针不被回收”。
- 能说清写者为何不能直接 `delete` 旧版本、必须 retire 等宽限期。
- 能对比 RCU 与 hazard pointer 的读侧开销与回收延迟取舍。

## 对应官方参考

- 提案 [`P2545R4` Read-Copy Update (RCU)](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2545r4.pdf)（早期：`P0566`）
- cppreference（C++26）[`<rcu>`](https://en.cppreference.com/w/cpp/header/rcu)
- [folly RCU](https://github.com/facebook/folly/blob/main/folly/synchronization/Rcu.h)（工业级对照）

## 构建运行

```bash
cmake --build build-vs2026 --target I3_rcu --config Release
./build-vs2026/I3_rcu/Release/I3_rcu.exe
```
