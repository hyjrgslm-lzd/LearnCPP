# A02：字段投影 DSL

先读[字段选择 DSL 正文](../../chapters/19-field-projection-dsl.md)。本题复用 P1 的公开普通成员聚合支持域，但自带 `checks/projection_schema.hpp`，不会修改 P1。

学生只编辑 `src/student/field_projection.hpp`。完整答案在 `src/reference`；`validation/good` 是独立完成体；`validation/bad` 会复制字段并按 schema 顺序投影，检查器必须拒绝。

## Part 1：固定字段基线

先运行 `A02_known_projection_baseline`。它只做 `Person::id`、`Person::name` 的手写访问，证明正确结果应该是引用、可写回，并且顺序可由调用点决定。

## Part 2：单字段 get

实现：

```cpp
template<fixed_string Name, class T>
constexpr decltype(auto) get(T&& object);
```

`get<"id">(person)` 返回真实成员表达式，保留 cv/ref。未知字段在编译期拒绝。字段名来自 schema，而不是运行时字符串查表。

## Part 3：投影 project

实现：

```cpp
template<fixed_string Spec, class T>
constexpr auto project(T& object);
```

`Spec` 是逗号分隔 identifier 列表。空串返回空 tuple；空组件、重复字段、未知字段、非法 identifier 拒绝。返回 tuple 引用，顺序与 DSL 一致。只接受左值，避免从临时对象返回悬垂引用。

## 检查

检查器覆盖：

- `get` 的可写引用、const引用和右值成员表达式。
- `project<"name,id">` 保留调用者顺序。
- tuple 中引用能写回原对象。
- 空投影为 `tuple<>`。
- unknown、duplicate、empty component、invalid identifier 和 rvalue project 的独立诊断。

本题为 U01 Mp11 提供稳定场景契约：字段 key 是编译期字符串，schema 是 `std::tuple<field<name, member>>`，字段值仍是运行时对象成员。

## 本机命令

```powershell
cmake -S C04_Generic_CompileTime_Reflection/exercises/A02_field_projection -B C04_Generic_CompileTime_Reflection/exercises/A02_field_projection/build/local -G "Visual Studio 18 2026" -A x64
cmake --build C04_Generic_CompileTime_Reflection/exercises/A02_field_projection/build/local --config Debug
ctest --test-dir C04_Generic_CompileTime_Reflection/exercises/A02_field_projection/build/local -C Debug --output-on-failure
```
