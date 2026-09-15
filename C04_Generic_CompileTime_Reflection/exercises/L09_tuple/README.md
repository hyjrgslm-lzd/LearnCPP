# 练习 L09：异构 tuple 遍历

先读 `../../chapters/09-tuple-traversal.md`。只编辑 `src/student/tuple_for_each.hpp`。

## Part 1：按索引遍历

实现 `for_each_tuple(tuple, callback)`，对 tuple 每个元素调用一次 callback。

解析：用 `std::tuple_size_v<std::remove_reference_t<Tuple>>` 和 `std::make_index_sequence<N>` 生成索引。

## Part 2：保留引用和值类别

callback 接收 `std::get<I>(std::forward<Tuple>(tuple))` 的真实结果。左值 tuple 给左值引用，const tuple 给 const 引用，右值 tuple 能移动 move-only 元素。

解析：函数体里的 `tuple` 是具名变量，不 forward 就会把右值变左值。

## Part 3：callback 是同一个左值

callback 对象应被复用，不要为每个元素复制一个。

解析：很多 callback 带状态，如计数器、输出迭代器和诊断收集器。

## Part 4：异常边界

callback 抛异常时停止后续访问，异常传播给调用者；已经发生的副作用不回滚。

解析：这是普通异常语义。遍历工具不提供事务。


## IDE 入口

从本课 `exercises` 根目录或本题目录生成 Visual Studio 18 2026 x64 工程。主项目是 `L09_tuple_student`；默认学生测试关闭时仍生成该项目，但它是 `EXCLUDE_FROM_ALL`，需显式构建。
学生只编辑：`src/student/tuple_for_each.hpp`。 `checks/`、`validation/`、`src/reference/`、diagnostic、support 目标是只读对照/验证/实验入口，保留在题目分组内。
修改 Student 后先重新构建对应目标，再运行 CTest 或 README 中列出的检查命令。
