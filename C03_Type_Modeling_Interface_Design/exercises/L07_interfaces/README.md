# L07：接口形状、借用与最小公开面

先读 `../../chapters/07-interfaces-and-polymorphism.md`。你只编辑 `src/student/owner.hpp`。

实现 `l07::Owner`：

```cpp
Owner(std::initializer_list<int>);
std::span<const int> view() const & noexcept;
std::span<const int> view() && = delete;
std::vector<int> snapshot() const;
void replace_all(std::vector<int> values) noexcept;
```

Part 1：`view()` 是 lvalue 借用。它不复制元素，不能抛异常，返回内容跟当前 owner 一致。

Part 2：`snapshot()` 是值快照。拿到 snapshot 后再 `replace_all()`，旧 snapshot 不变。

Part 3：`replace_all()` 按值接收，再 `swap` 提交。它不分配，不抛异常。

Part 4：`view() && = delete`。临时对象不能借出 view；运行 checker 会用 `requires` 检查当前选中实现，另有 Reference 编译负例记录编译器诊断。

解析：借用接口要绑定 owner 生命期，所以用 `const &` 限定；值快照脱离 owner，所以返回 `std::vector<int>`。`noexcept` 只说明函数不抛异常，不能代替 strong guarantee。


## IDE 入口

从本课 `exercises` 根目录或本题目录生成 Visual Studio 18 2026 x64 工程。主项目是 `L07_interfaces_student`；默认学生测试关闭时仍生成该项目，但它是 `EXCLUDE_FROM_ALL`，需显式构建。
学生只编辑：`src/student/owner.hpp`。 `checks/`、`validation/`、`src/reference/`、diagnostic、support 目标是只读对照/验证/实验入口，保留在题目分组内。
修改 Student 后先重新构建对应目标，再运行 CTest 或 README 中列出的检查命令。
