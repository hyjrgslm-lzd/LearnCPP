# 练习 E1：ABI 边界、opaque handle 与 C consumer

先读 [05：ABI 边界、所有权与错误通道](../../chapters/05-abi-boundaries.md)。本题固定一份 C ABI：

```c
lesson_status lesson_create(unsigned requested_abi, int seed, lesson_engine** out_engine);
lesson_status lesson_eval(lesson_engine* engine, int input, int* out_value);
void lesson_destroy(lesson_engine* engine);
```

规则：ABI 版本为 `1`；`seed` 和 `input` 都在 `[-1000, 1000]`；成功时 `eval = seed + input`；失败时用 status 返回，不抛异常跨边界。

## VS 日常入口

先按[总构建指南](../BUILD_GUIDE.md)生成并构建 `vs-study`，打开整章解决方案并选择 `Debug | x64`。本节命令从 `C01_Build_Compile_Link/exercises` 目录执行。

将 `E1_abi_student` 设为启动项目；学生实现位于 `Support/E1_lesson_student_static` 的 `Student` 节点：[lesson_api.cpp](src/student/lesson_api.cpp)。学生库与检查程序保留分离，以维持 API 边界；两者均显示 [lesson_api.h](include/lesson_api.h)。

学生库与学生运行项目始终生成，`ENGINEERING_STUDY_TEST_STUDENTS` 只控制 CTest 注册。可选参考实现及对应检查由 `ENGINEERING_STUDY_BUILD_REFERENCE` 控制，`vs-study` 默认同时启用这两个开关。

除明确标注的独立工具步骤外，下文命令也从 `exercises` 目录执行，使用已构建的 `vs-study`。

## Part 1：Reference 契约

运行：

```powershell
ctest --preset vs-study -R '^E1_abi_reference'
```

Reference 同时检查 static 和 shared 两种链接方式。

**解析：** 检查器会验证 ABI 查询、版本拒绝、范围拒绝、null 参数、成功 create/eval、失败不改输出哨兵、同库 destroy，以及 `destroy(nullptr)`。它不会重复 destroy 已释放指针，因为那本身违反契约。

## Part 2：异常转换

运行：

```powershell
ctest --preset vs-study -R '^E1_abi_bad_alloc$'
```

**解析：** 这是私有测试编译变体：同一 reference 实现用 private compile definition 控制 `seed == 999` 时抛 `std::bad_alloc`。检查器要求返回 `LESSON_INTERNAL_ERROR` 且 handle 仍为 null。不耗尽系统内存，也不暴露生产故障注入 API。

## Part 3：真实 C consumer

运行：

```powershell
ctest --preset vs-study -R '^E1_abi_c_consumer$'
```

**解析：** C 程序包含 `lesson_api.h`，调用 create/eval/destroy 并得到 `42`。这证明当前边界是 C 可消费的函数 API；不证明 C 能处理 C++ 类、重载、异常或 STL。

## Part 4：layout 观察

运行：

```powershell
ctest --preset vs-study -R '^E1_abi_observation_layout$'
```

**解析：** 程序打印两个普通结构体的 size/alignment，说明字段顺序和对齐会进入布局。`lesson_engine` 在公共头里是不完整类型，所以这些内部布局变化不会泄露成 ABI 承诺。

## Student

学生只改 `src/student/lesson_api.cpp`。Starter 会构建，但 `E1_abi_student` 必须失败，因为 create/eval 尚未实现。

运行：

```powershell
cmake --build --preset vs-study --target E1_abi_student
ctest --preset vs-study -R '^E1_abi_student$'
```

完成实现时，必须保持：

- `lesson_create()` 在 `out_engine` 非空时先写 null，再检查 ABI 和 seed。
- `lesson_eval()` 失败时不改 `out_value`。
- `lesson_destroy(nullptr)` 可调用。
- C++ 异常不能跨出 API。
