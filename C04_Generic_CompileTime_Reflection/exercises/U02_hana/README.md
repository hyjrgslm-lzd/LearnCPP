# U02：Boost.Hana 异构记录观察

先读[第24章：Boost.Hana](../../chapters/24-hana.md)。本单元是观察和迁移题，不提供 Student/Reference 分层，也不要求你补完整框架代码。目标是把 A02 的字段 schema 思路迁移到 Hana：编译期 key 决定类型和字段选择，运行期对象只提供被访问或被修改的值。

运行 `U02_hana_record_observation`，先观察三件事：

- `hana::type_c<T>` 和 `hana::integral_c<T, v>` 是编译期值，能参与类型推导和 `static_assert`。
- `BOOST_HANA_STRING("id")` 生成编译期 key，`hana::map` 用它做 `find` / `at_key`。
- map 的 value 可以是运行时对象的指针或引用，但运行时值不能反过来决定新 C++ 类型。

再读并运行 `U02_hana_migration_solution`。它用一个陌生的 `SensorReading`，避免只复述 `Person` 示例。你要能解释下面的迁移边界：

1. `type_schema` 是编译期 key 到字段类型的 map。`hana::at_key(type_schema, voltage_key) == hana::type_c<double>` 是类型证明；缺键用 `hana::find(type_schema, missing_key)`，不能用 `at_key` 硬取。
2. `copied_values` 保存字段值副本。改 map 里的 `channel` 或 `unit` 不会改 `SensorReading`。
3. `borrowed_values` 保存 `std::ref(reading.member)`。改 `hana::at_key(borrowed_values, key).get()` 会写回 `reading`。
4. `const auto const_borrowed_values = borrowed_values` 只让 Hana map 自身变 const；`std::reference_wrapper<T>::get() const` 仍返回 `T&`，所以它不会自动把被引用的 `reading` 变成 const。
5. 用户输入、配置文件字符串或网络数据属于运行期值，不能直接生成新的 `BOOST_HANA_STRING(user_text)` 或新的 C++ 类型。需要运行期分发表时，应先把输入匹配到一组已写死的编译期 key。

验收标准：

- Debug/Release 下 `U02_hana_record_observation` 和 `U02_hana_migration_solution` 都能构建并通过 CTest。
- 你能指出 `find` 表示可选查询，`at_key` 表示必须存在的编译期 key。
- 你能说明 copy map 与 `std::ref` borrowed map 的所有权和生命周期差异。

## 本机命令

```powershell
cmake -S C04_Generic_CompileTime_Reflection/exercises/U02_hana -B C04_Generic_CompileTime_Reflection/exercises/U02_hana/build/local -G "Visual Studio 18 2026" -A x64 -DGENERIC_STUDY_ENABLE_META_LIBS=ON
cmake --build C04_Generic_CompileTime_Reflection/exercises/U02_hana/build/local --config Debug
ctest --test-dir C04_Generic_CompileTime_Reflection/exercises/U02_hana/build/local -C Debug --output-on-failure
```
