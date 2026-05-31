# 练习 E-2：atomic_ref 引用既有对象

> 详尽版见 `../../07-模块E-原子操作基础.md` 的 练习 E-2。

## 目标

理解 `std::atomic_ref<T>`（原子引用，`<atomic>`，**C++20**）：给一个“本来就是普通非原子类型”的既有对象（数组元素、结构体字段）临时套上原子访问，而**无需把该类型本身声明成 `std::atomic`**。用 `atomic_ref` 对普通 `int` 数组的某元素做多线程并发累加（结果精确无丢更新），并讲清它的对齐（alignment）与生命周期（lifetime）要求。

## 前置理解

- `atomic_ref` 是**引用语义**：它不拥有数据，只是把原子操作叠加到一个既有对象上。多个线程各自构造指向同一对象的 `atomic_ref` 是合法且预期的用法。
- 它解决的痛点：你有一片大的普通 `int[]`（别处当普通数组高效使用），只想在某个并发阶段对个别元素做原子访问——不必把整个数组改成 `atomic<int>[]`。
- 两条硬性要求：
  - **可平凡复制（trivially copyable）**：`T` 必须满足；`int` 满足。
  - **对齐**：被引用对象至少要满足 `atomic_ref<T>::required_alignment`，否则是 UB。对“可能未对齐”的对象（打包结构体字段等）要特别小心。
- **生命周期约束**：`atomic_ref` 不延长对象寿命；只要还有 `atomic_ref` 活着，被引用对象就必须存活，且对它的**全部访问都要经由 `atomic_ref`**（不能一边 `atomic_ref` 一边普通读写同一对象）。
- 默认内存序仍是 `seq_cst`，理由同 E-1，细节留模块 F。

## 必做任务

1. `// TODO [必做 1]`：对普通 `int data[4]` 的热点元素 `data[2]`，在 8 个线程里各自构造 `std::atomic_ref<int>` 并 `fetch_add(1)` 累加 10 万次，验证结果精确、其余元素不受影响。
2. `// TODO [必做 2]`：打印 `required_alignment` / `alignof` / `is_always_lock_free`；用“先经 `atomic_ref` 原子写、待其析构后再普通读”的合法顺序体会生命周期边界。

## 进阶任务

- 思考：把 `atomic_ref` 用在 `std::vector<int>` 的元素上要注意什么（重新分配 reallocation 会使引用失效）？什么场景下 `atomic_ref` 比 `atomic<T>` 更合适？

## 验收点

- 能解释 `atomic_ref` 与 `atomic<T>` 的区别：前者是引用语义、套在既有对象上，后者拥有存储。
- 能用 `atomic_ref` 对普通数组元素做无丢更新的并发累加。
- 能说出 `atomic_ref` 的对齐要求与生命周期约束（存活期间只经 ref 访问、对象不能更早销毁）。

## 对应官方参考

- cppreference [`std::atomic_ref`](https://en.cppreference.com/w/cpp/atomic/atomic_ref) / [`fetch_add`](https://en.cppreference.com/w/cpp/atomic/atomic_ref/fetch_add)
- 提案 P0019R8 "Atomic Ref"
- 《C++ Concurrency in Action, 2nd ed.》(Williams) 第 5 章 5.2.5

## 构建运行

```bash
cmake --build build-vs2026 --target E2_atomic_ref --config Release
./build-vs2026/E2_atomic_ref/Release/E2_atomic_ref.exe
```
