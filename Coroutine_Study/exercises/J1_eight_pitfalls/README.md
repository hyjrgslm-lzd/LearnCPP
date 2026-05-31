# 练习 J-1：八大经典陷阱重现

对应文档：`12-模块J-陷阱诊断与跨编译器.md` §J-1

## 目标

亲手重现 C++26 协程工程中最常见的八个生命周期陷阱，每个陷阱给出最小复现 +
修复方案。本题的肌肉记忆来自 Segmentation Fault，不是别人写好的总结。

## 必做任务

逐一重现以下八个陷阱（每个不超过 30 行最小复现）：

1. **lambda capture by reference 跨 co_await** —— 引用捕获外部栈变量；
2. **临时量在 co_await 表达式中析构** —— `co_await foo(std::string("x").c_str())`；
3. **co_await 期间 lock_guard 跨挂起点** —— 跨线程解锁 UB；
4. **initial_suspend 抛异常的 frame 泄漏** —— 编译器异常路径行为差异；
5. **detached coroutine 越过创建者生命周期** —— 永远不要 detach；
6. **generator 返回引用的 dangling** —— `generator<T&>` 与临时量；
7. **promise destructor throw** —— 析构必须 noexcept；
8. **自写 generator 的 co_yield 临时量引用** —— promise 只存引用 vs 存值。

骨架文件中每个 trap 拆为一个 namespace，各含 `bad_()` / `good_()` 两个 demo
协程；`main()` 按陷阱编号串行调用。

## 验收点

- 八个陷阱中独立重现至少六个，并能解释崩溃根因；
- 在 Code Review 中能识别这八类模式；
- 看到 `lock_guard + co_await` 立即条件反射地指出是 UB；
- 能解释为什么标准没规定协程帧布局——这正是跨编译器陷阱难统一检测的根因。

## 约束

- 骨架代码 **无需补 TODO 即可编译**。要看见真正的崩溃/泄漏，请取消相应
  `// throw` / `// 危险代码` 行的注释，并配 ASan / MSVC `/RTC1` 运行；
- 中文注释保留，禁止把陷阱的"修复版"误改回 BAD 版；
- 八个陷阱中至少跑一遍 `-fsanitize=address`，记录哪些能被 ASan 抓到，
  哪些抓不到（陷阱 3 / 5 通常 ASan 抓不到，须靠 TSan / 静态分析）。

## 提示

- 单线程测试不会触发陷阱 3，请用至少两线程的执行环境；
- 陷阱 4 在 MSVC / Clang / GCC 上的行为差异是 J-2 的素材；
- 陷阱 8 在标准 `std::generator` 上不会发生 —— 标准 promise 默认拷贝 yield
  值；这里特意用自写 generator 暴露问题。
