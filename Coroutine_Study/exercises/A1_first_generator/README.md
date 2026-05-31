# 练习 A-1：第一个 generator

> 详尽版本见 `../../02-模块A-三关键字与最小协程.md` 的 `练习 A-1` 章节。
> 本 README 仅摘抄"目标 / 必做任务 / 验收点"。

## 目标

用 `std::generator<int>` 写一个斐波那契数列生成器和一个按行 yield 文本的生成器，配合
`std::views::take` 消费，亲手感受 `co_yield` 的惰性语义。

## 必做任务

1. 写一个 `fibonacci()` 函数，返回 `std::generator<int>`。协程体内用 `co_yield` 依次产出
   斐波那契数列的前 N 项。
2. 在 `main()` 中用 `std::views::take` 消费这个 generator 的前 10 项；注意临时
   generator 的生命周期。
3. 在协程体内的每次 `co_yield` 前后各加一行日志，观察执行顺序。
4. 在 `main()` 的迭代循环中也各加一行日志。两边对比，确认：迭代器不推进，协程体就不继续执行。
5. 写 `read_lines()` 返回 `std::generator<std::string>`，用 `co_yield` 逐行产出 5~8 行模拟文本。
6. 用 range-based for 消费这个 generator，打印行号与内容。
7. 画一张时序图：标注协程创建点、每次 `co_yield` 挂起点、每次 `++` 恢复点、final_suspend 销毁点。

## 进阶任务

- 把 `fibonacci()` 改成无限 generator，再用 `std::views::take(15)` 截取。
- 写 `filter_even()`：接收 `std::generator<int>&&`，用 `co_yield` 只产出其中的偶数。
- 把 `read_lines()` 改成按"段"yield，每段 3 行拼在一起。

## 验收点

- 日志能证明：main 的循环行打印在前，协程体的 `co_yield` 前打印在后；迭代器不推进，
  协程体不运行。
- 你能解释为什么 generator 对象（含协程帧）不能作为临时对象立即析构。
- 你能画出从 `auto gen = fibonacci(10)` 到 `for (auto v : gen)` 这条链上协程帧的
  创建、第一次 resume、每次 `co_yield` 后等待的全过程。
