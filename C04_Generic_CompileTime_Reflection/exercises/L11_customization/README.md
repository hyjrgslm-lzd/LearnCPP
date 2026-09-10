# 练习 L11：`read_value` customization point object

先阅读 [11. 自定义点：从已知类型函数到 `read_value` CPO](../../chapters/11-customization-points.md)。本题只编辑 `src/student/read_value.hpp`。Reference 在 `src/reference/read_value.hpp`，good/bad 控制实现分别在 `validation/good/read_value.hpp` 和 `validation/bad/read_value.hpp`，公共检查在 `checks/read_value_checks.cpp`。

目标接口是 `c04::read_value(object)`。它是一个函数对象，不是一个普通函数模板。函数对象放在 `namespace c04`，测试类型放在别的 namespace，避免 hidden friend 和 CPO 变量同名时误用普通查找。

## Part 1：成员路径基线

先支持有成员函数的对象：

```cpp
object.read_value()
```

要求按当前对象的 cv/ref 条件检查可调用性。`T&`、`const T&`、`T&&` 不是同一个场景。成员返回 `int&` 时，`c04::read_value(object)` 也必须返回 `int&`，不能返回 `int`。

解析：这里需要转发引用、`std::forward<T>` 和 `decltype(auto)`。`auto` 会丢引用，普通具名参数会把右值变成左值。checker 会检查引用地址，不能只返回同一个数值。

## Part 2：ADL 路径

当成员不可调用时，尝试未限定调用：

```cpp
read_value(object)
```

这个调用必须让 ADL 找到对象类型关联 namespace 里的自由函数或 hidden friend。实现内部要用 detail namespace 加 poison pill 隔离，避免未限定查找找到 `c04::read_value` 这个 CPO 对象并递归。

解析：不要写 `c04::read_value(object)`，那是调用 CPO 自己；也不要写固定 namespace 的自由函数名，那会漏掉 hidden friend。

checker 还覆盖一个容易漏的正例：类型里有 `read_value(int)` 成员，但没有零参数成员，同时同 namespace 有合法 `read_value(object)` ADL 函数。正确实现必须忽略那个不可调用成员，选择 ADL，并保留 ADL 返回引用和 `noexcept`。

## Part 3：成员优先

如果一个类型同时提供成员 `read_value()` 和 ADL `read_value(object)`，选择成员。

解析：最简单可靠的写法是两个 `operator()` 重载：第一个要求 member 可调用；第二个要求 member 不可调用且 ADL 可调用。这样优先级写在约束里，读者不用猜模板偏序。

## Part 4：不可调用要被约束拒绝

成员存在但签名不匹配，不算合法路径。没有成员也没有 ADL 的类型，`requires { c04::read_value(object); }` 应为 `false`。

解析：错误应停在候选集阶段，而不是在函数体中硬调用后产生长模板错误。Student 占位会提供过宽 fallback，所以能编译，但 checker 会用无路径对象拒绝它。

## Part 5：异常规格

`c04::read_value(object)` 的 `noexcept` 必须等于实际选择的底层表达式。底层成员或 ADL 不抛，CPO 也不抛；底层可能抛，CPO 不能承诺不抛。

解析：使用 `noexcept(noexcept(expr))`。不要统一写死 `noexcept`，也不要完全不写异常规格。

## 验证

单题学生检查：

```powershell
cmake -S C04_Generic_CompileTime_Reflection/exercises/L11_customization -B build/sample-author-student -G "Visual Studio 18 2026" -A x64 -DGENERIC_STUDY_BUILD_REFERENCE=OFF -DGENERIC_STUDY_TEST_STUDENTS=ON
cmake --build build/sample-author-student --config Debug
ctest --test-dir build/sample-author-student -C Debug --output-on-failure
```

Reference/good 应通过全部检查。Student 占位和 bad 控制实现应能编译，但运行 checker 非零退出，并至少出现：

```text
check failed: no-path object must not satisfy c04_readable
```

前置反例观察：

```powershell
cmake -S C04_Generic_CompileTime_Reflection/exercises/L11_customization -B build/sample-author-r2 -G "Visual Studio 18 2026" -A x64
cmake --build build/sample-author-r2 --config Debug --target L11_customization_counterexamples
.\build\sample-author-r2\Debug\L11_customization_counterexamples.exe
```

`observations/counterexamples.cpp` 安全观察四类错误：`auto` 丢引用、忘记 `std::forward` 改变右值调用、未隔离 ADL 导致无路径对象被 CPO 自己接受、写死 `noexcept` 谎报异常规格。它不会实际触发无限递归或 `std::terminate()`；递归路径用深度哨兵截断，`noexcept` 路径只检查编译期异常规格。
