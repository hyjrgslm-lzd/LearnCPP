# 练习 L05：特殊成员与深复制

先读 [05. 特殊成员](../../chapters/05-special-members.md)。本题是实现题，学生只编辑 `src/student/int_buffer.hpp` 和 `src/student/int_buffer.cpp`。

| Part | 操作 | 检查 |
|---|---|---|
| L05-1 | 实现初始化列表构造和析构 | 从 `l05_support::allocate` 获取 storage，析构释放 |
| L05-2 | 实现复制构造和复制赋值 | 复制后两个 buffer 有独立 storage，互相修改不影响 |
| L05-3 | 实现移动构造和移动赋值 | 转交 handle，源对象按本题契约变空 |
| L05-4 | 处理自赋值和自移动 | 不泄漏，不重复释放 |

checker 的 storage model 在 `checks/support/memory_model.hpp`。Student 不能自己报告 live 计数；检查器直接读取 fixture。

运行：

```powershell
cmake -S C02_Objects_Lifetime_Ownership/exercises/L05_special_members -B C02_Objects_Lifetime_Ownership/exercises/L05_special_members/build/c02-foundation-author -G "Visual Studio 18 2026" -A x64
cmake --build C02_Objects_Lifetime_Ownership/exercises/L05_special_members/build/c02-foundation-author --config Debug
ctest --test-dir C02_Objects_Lifetime_Ownership/exercises/L05_special_members/build/c02-foundation-author -C Debug --output-on-failure
```

解析：深复制必须分配新 storage。浅复制会让两个对象共享同一 slot，修改 source 会改变 copy，并可能导致重复释放；bad_shallow_copy 会被独立 storage 检查拒绝。移动赋值必须先释放目标旧 storage，再接管源 storage；bad_move_leak 会让 live count 停在 2，被 checker 拒绝。


## IDE 入口

从本课 `exercises` 根目录或本题目录生成 Visual Studio 18 2026 x64 工程。主项目是 `L05_special_members_student`；默认学生测试关闭时仍生成该项目，但它是 `EXCLUDE_FROM_ALL`，需显式构建。
学生只编辑：`src/student/int_buffer.cpp`、`src/student/int_buffer.hpp`。 `checks/`、`validation/`、diagnostic、support 目标是只读对照/验证/实验入口，保留在题目分组内。
修改 Student 后先重新构建对应目标，再运行 CTest 或 README 中列出的检查命令。
