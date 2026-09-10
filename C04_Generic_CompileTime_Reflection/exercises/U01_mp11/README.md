# U01：用 Boost.Mp11 重写字段 schema 计算

先读[第23章：Boost.Mp11](../../chapters/23-mp11.md)。本题复用 A02 的 `Person`、`Order` 和 `field<name, member>` 场景，只把手写 type_list 迁移到 Mp11。

学生只编辑 `src/student/mp11_schema_tools.hpp`。初始 Student 提供可实例化的公开 API 占位，能编译，但 `schema_map_t<Person>` 仍是空 map，运行检查会在真实 schema 形状上失败。完成实现后不需要切任何宏或完成标记。Reference 和 `validation/good` 是两份独立完成体；`validation/bad` 会通过大部分类型检查，但动态索引越界没有先检查，运行检查必须拒绝。

## 任务

实现这些公开接口：

- `key<"id">`：把字段名变成类型 key。
- `schema_map_t<Record>`：把 `std::tuple<field<name, ptr>...>` 转成 Mp11 map。
- `find_field_t<Record, Name>`：找到字段 pair，缺键返回 `void`。
- `required_field_t<Record, Name>`：缺键时给出 `mp11 map key not found` 诊断。
- `field_count_v<Record>` 与 `keys_are_unique_v<Record>`：type 到 value 的计算。
- `insert_field_t`、`replace_field_t`、`update_field_t`：展示 Mp11 map 的真实语义。
- `transform_fields_t`、`filter_fields_t`、`unique_types_t`、`product_t`、`lazy_provider_t`：覆盖 map/filter/unique/product/惰性。
- `get_by_key<Name>(object)`：按 key 返回真实成员表达式。
- `visit_field_type<Record>(index, visitor)`：运行时索引先做边界检查，再进入类型分派。

动态索引不能把越界值直接传给 `mp_with_index`。先判断 `index >= field_count_v<Record>` 并抛出 `std::out_of_range`，再做类型分派。

空schema也是合法元map，但没有任何有效动态索引：调用分派应抛出`std::out_of_range`，visitor零次调用，空分支不提供正常返回值。运行时的边界`if`不能阻止`mp_with_index<0>`在编译期实例化，因此还要用`if constexpr (count > 0)`隔离该调用；先查类型层支持域，再检查运行时索引。

## 本机命令

```powershell
cmake -S C04_Generic_CompileTime_Reflection/exercises/U01_mp11 -B C04_Generic_CompileTime_Reflection/exercises/U01_mp11/build/local -G "Visual Studio 18 2026" -A x64 -DGENERIC_STUDY_ENABLE_META_LIBS=ON
cmake --build C04_Generic_CompileTime_Reflection/exercises/U01_mp11/build/local --config Debug
ctest --test-dir C04_Generic_CompileTime_Reflection/exercises/U01_mp11/build/local -C Debug --output-on-failure
```
