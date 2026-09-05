# 练习 J-1：八大经典陷阱重现

对应文档：`12-模块J-陷阱诊断与跨编译器.md` §J-1

## 目标

亲手重现 C++ 协程工程中最常见的八个生命周期陷阱，每个陷阱给出最小复现、
分类和修复方案。默认 starter/reference 只跑安全路径；真正的 UB 用显式 unsafe
开关或单独 sanitizer 目标观察。

## 必做任务

逐一重现以下八个陷阱（每个不超过 30 行最小复现）：

1. **lambda capture by reference 跨 co_await** —— 引用捕获外部栈变量；
2. **awaiter 保存临时对象派生指针** —— `foo(std::string("x").c_str())`；
3. **co_await 期间 lock_guard 跨挂起点** —— 跨线程恢复时可触发非 owner unlock UB；
4. **initial_suspend 抛异常** —— 清理/诊断路径不能作为库设计依赖；
5. **detached coroutine 越过创建者生命周期** —— 永远不要 detach；
6. **消费者把 yield 窗口内的指针持久化** —— 推进 generator 后旧指针悬空；
7. **promise destructor throw** —— 进入 `std::terminate`，不可恢复；
8. **自写 generator 持久暴露 string_view** —— view 只在当前 yield 窗口内有效。

骨架文件中每个 trap 拆为一个 namespace，各含 `bad_()` / `good_()` 两个 demo
协程；`main()` 按陷阱编号串行调用。

## Starter / Reference

- `main.cpp` 是安全 starter：可以直接编译运行，危险代码默认只以注释展示。
- `solution.cpp` 是参考答案：输出八类陷阱分类，并用 `coroutine_study::check` 验证安全修复路径。
- `COROUTINE_STUDY_BUILD_REFERENCE=ON` 时会生成 `J1_eight_pitfalls_reference` 并加入 CTest。

## 验收点

- 八个陷阱中独立重现至少六个，并能解释崩溃根因；
- 在 Code Review 中能识别这八类模式；
- 看到 `lock_guard + co_await` 立即条件反射地指出：如果跨线程恢复是 UB，即使同线程也通常是工程禁用模式；
- 能解释为什么标准没规定协程帧布局——这正是跨编译器陷阱难统一检测的根因。

## 约束

- 骨架代码无需补代码即可编译。要看见真正的崩溃/泄漏，请取消相应
  `// throw` / `// 危险代码` 行的注释，并配 ASan / MSVC `/RTC1` 运行；
- 中文注释保留，禁止把陷阱的"修复版"误改回 BAD 版；
- 八个陷阱中至少跑一遍 `-fsanitize=address`，记录哪些能被 ASan 抓到，
  哪些抓不到（陷阱 3 / 5 通常 ASan 抓不到，须靠 TSan / 静态分析）。

## 提示

- 单线程测试不会触发陷阱 3，请用至少两线程的执行环境；
- 陷阱 4 在 MSVC / Clang / GCC 上的清理与诊断差异是 J-2 的素材；
- 陷阱 6/8 的关键不是"临时量立即悬空"，而是把当前 yield 窗口内才有效的借用值保存到下一次 resume 之后。
