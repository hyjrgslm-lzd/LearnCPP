# 练习 E1：ABI 边界、opaque handle 与 C consumer

先读 [05：ABI 边界、所有权与错误通道](../../chapters/05-abi-boundaries.md)。本题固定一份 C ABI：

```c
lesson_status lesson_create(unsigned requested_abi, int seed, lesson_engine** out_engine);
lesson_status lesson_eval(lesson_engine* engine, int input, int* out_value);
void lesson_destroy(lesson_engine* engine);
```

规则：ABI 版本为 `1`；`seed` 和 `input` 都在 `[-1000, 1000]`；成功时 `eval = seed + input`；失败时用 status 返回，不抛异常跨边界。

## Part 1：Reference 契约

运行：

```powershell
ctest --test-dir build --tests-regex E1_abi_reference --output-on-failure
```

Reference 同时检查 static 和 shared 两种链接方式。

**解析：** 检查器会验证 ABI 查询、版本拒绝、范围拒绝、null 参数、成功 create/eval、失败不改输出哨兵、同库 destroy，以及 `destroy(nullptr)`。它不会重复 destroy 已释放指针，因为那本身违反契约。

## Part 2：异常转换

运行：

```powershell
ctest --test-dir build --tests-regex E1_abi_bad_alloc --output-on-failure
```

**解析：** 这是私有测试编译变体：同一 reference 实现用 private compile definition 控制 `seed == 999` 时抛 `std::bad_alloc`。检查器要求返回 `LESSON_INTERNAL_ERROR` 且 handle 仍为 null。不耗尽系统内存，也不暴露生产故障注入 API。

## Part 3：真实 C consumer

运行：

```powershell
ctest --test-dir build --tests-regex E1_abi_c_consumer --output-on-failure
```

**解析：** C 程序包含 `lesson_api.h`，调用 create/eval/destroy 并得到 `42`。这证明当前边界是 C 可消费的函数 API；不证明 C 能处理 C++ 类、重载、异常或 STL。

## Part 4：layout 观察

运行：

```powershell
ctest --test-dir build --tests-regex E1_abi_observation_layout --output-on-failure
```

**解析：** 程序打印两个普通结构体的 size/alignment，说明字段顺序和对齐会进入布局。`lesson_engine` 在公共头里是不完整类型，所以这些内部布局变化不会泄露成 ABI 承诺。

## Student

学生只改 `src/student/lesson_api.cpp`。Starter 会构建，但 `E1_abi_student` 必须失败，因为 create/eval 尚未实现。

运行：

```powershell
ctest --test-dir build --tests-regex E1_abi_student --output-on-failure
```

完成实现时，必须保持：

- `lesson_create()` 在 `out_engine` 非空时先写 null，再检查 ABI 和 seed。
- `lesson_eval()` 失败时不改 `out_value`。
- `lesson_destroy(nullptr)` 可调用。
- C++ 异常不能跨出 API。
