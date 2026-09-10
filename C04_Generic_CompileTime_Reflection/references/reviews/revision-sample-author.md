# A02 字段投影 DSL 作者自查交接

范围：`chapters/19-field-projection-dsl.md` 与 `exercises/A02_field_projection/`。未修改根 `CMakeLists.txt`、根导航、根 `StudySetup.cmake` 或 P1 文件。

## 契约核对

- 正文从 P1 已登记公开普通成员聚合出发，先给固定字段正确基线，再进入 `get<"id">` 与 `project<"id,name">`。
- `get<Name>(object)` 按 schema 生成真实成员表达式，保留 cv/ref，并允许右值字段表达式被立即消费。
- `project<Spec>(object)` 只接受左值，包括 const 左值；`Person{}` 与 `std::move(const Person)` 均由 deleted overload 拒绝。
- DSL 支持空串空投影；逗号分隔 ASCII identifier；首/尾/连续空项、嵌入 NUL、非法字符、重复字段、未知字段均有独立诊断。
- 投影返回 tuple 引用，顺序来自 DSL；检查器验证 `project<"name,id">` 的写回与顺序，不把 schema 顺序当答案。
- A02 自带 `projection_schema.hpp`，字段/key 形态为 `std::tuple<field<"name", &Record::member>>`，可供后续 U01 Mp11 复用场景契约。

## 独立性

- `validation/good/field_projection.hpp` 先于 Reference 落盘；落盘时 SHA256 为 `22FB5B72EEFA1EB4463E3E0CDB755112F87B26CE77263C759CD5CB4042CDDC26`。
- 后续根据编译和主线程边界反馈修正 good，最终 SHA256 为 `A9AC4AC4797283ABED0E9008CB787A48E6D1D44C36E0E8A1D8971FCC70F680E8`。
- good 未 include Reference、`src/reference` 或 `validation/good` 以外答案路径；Reference 使用独立 `reference_detail` 实现。

## 已验证

见 `references/validation/revision-20260910/sample-projection-evidence.md`。

## 交接状态

作者自查：ITERATE 后已补正文推导深度、rvalue/const rvalue 边界、尾逗号和嵌入 NUL 诊断、Student 可构建初态，以及原始命令证据。独立审查：slot 已预留，尚需非作者按遮 Reference 流程复核正文可推导性、Student 初态、good 独立性和诊断覆盖。
