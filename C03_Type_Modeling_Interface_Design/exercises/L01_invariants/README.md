# L01：类不变量

先读 `../../chapters/01-class-invariants.md`。你只编辑 `src/student/bounded_int.hpp`。

实现 `l01::BoundedInt`：构造时给定 `value/min/max`，合法范围包含端点；`min > max` 是配置错误；`set()` 修改值；非法构造或非法修改抛出异常；非法修改失败后旧值不变。

运行：

```powershell
cmake -S C03_Type_Modeling_Interface_Design/exercises/L01_invariants -B build/c03-l01 -G "Visual Studio 18 2026" -A x64 -DTYPE_STUDY_TEST_STUDENTS=ON
cmake --build build/c03-l01 --config Debug
ctest --test-dir build/c03-l01 -C Debug --output-on-failure
```

解析：范围检查属于类型边界。调用者拿到 `BoundedInt` 后，可以相信 `value()` 永远在 `[min(), max()]` 内。构造阶段失败表示没有产生完整对象；`set()` 失败表示已有对象保持旧值。坏变体允许非法构造，checker 会拒绝。


## IDE 入口

从本课 `exercises` 根目录或本题目录生成 Visual Studio 18 2026 x64 工程。主项目是 `L01_invariants_student`；默认学生测试关闭时仍生成该项目，但它是 `EXCLUDE_FROM_ALL`，需显式构建。
学生只编辑：`src/student/bounded_int.hpp`。 `checks/`、`validation/`、`src/reference/`、diagnostic、support 目标是只读对照/验证/实验入口，保留在题目分组内。
修改 Student 后先重新构建对应目标，再运行 CTest 或 README 中列出的检查命令。
