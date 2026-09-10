# C04 L07-L10 核心正确体盲写复核记录

日期：2026-09-10

## 结论

本轮完成了 L07/L08/L09/L10 四个 `validation/good` 正确完成体的独立盲写、冻结、覆盖、Reference 后置复核和 CTest 验证。此记录只证明这四个核心正确体的实现证据，不宣布 C04 全课审查通过。

最终 `validation/good` 哈希如下：

- `L07_packs/validation/good/pack_tools.hpp`: `44CF0CBAB053448749C79F3578125D59DF6725AE09476F9178B8371543CFD13C`
- `L08_type_lists/validation/good/type_list_tools.hpp`: `993D3684653610CF69909B997D905DCC0783DAA08FD852BFB8B41E1F9B1514D2`
- `L09_tuple/validation/good/tuple_for_each.hpp`: `90DEC28AE086FE74A8C41687B178B9ACED42D96E411653B8E358BA951E938BA6`
- `L10_constexpr/validation/good/constexpr_tools.hpp`: `C72742879475CC8943CE71296AAE59CC334986F9F942A9BCBD203A207085DB77`

## 真正的盲写范围

盲写阶段只读取：

- 四题 README
- 四题 `src/student/*`
- 四题 `checks/*`
- `chapters/07-packs-nttp.md`
- `chapters/08-type-lists.md`
- `chapters/09-tuple-traversal.md`
- `chapters/10-constant-evaluation.md`

盲写阶段未读取任何现有 `validation/good`、`validation/bad`、`src/reference` 或旧审查记录。初稿和 SHA256 冻结在：

- `references/validation/revision-20260910/blind-core/README.md`
- `references/validation/revision-20260910/blind-core/L07/pack_tools.hpp`
- `references/validation/revision-20260910/blind-core/L08/type_list_tools.hpp`
- `references/validation/revision-20260910/blind-core/L09/tuple_for_each.hpp`
- `references/validation/revision-20260910/blind-core/L10/constexpr_tools.hpp`

## 冻结后修复

- L08 初稿问题：不可调用 `value_sig` 进入 checker concept 时，`transform_completion_signatures` 直接展开 `transform_one`，MSVC 报未定义类型硬错误。修复：追加 `L08/type_list_tools.v2.hpp`，给列表偏特化增加签名可变换约束，让不可调用场景表现为 concept `false`。
- L10 初稿问题：`consteval` 函数循环内用局部变量参与 `static_assert`，MSVC 报 `C2131`。修复：追加 `L10/constexpr_tools.v2.hpp`，改用模板递归解析，非法字符和溢出在模板实例化层面诊断。
- L09 初稿行为通过，但 Reference 后置复核发现头文件直接使用 `std::remove_reference_t` 却未显式包含 `<type_traits>`。修复：追加 `L09/tuple_for_each.v2.hpp`，只补自包含 include。

L07 初稿未做冻结后修复。

## Reference 后置复核差异

- L07：行为与 Reference 一致。差异只在 `fixed_string` 复制实现，盲写版用手写循环，Reference 用 `std::copy_n`。
- L08：v2 行为与 Reference 一致。差异只在 helper 组织方式：盲写版有独立 `push_back` 与 `transformable_signature` concept，Reference 直接在 `requires` 表达式中检查。
- L09：v2 行为与 Reference 一致。Reference 多一层 `auto&& callback = f`，盲写版直接把具名参数 `f` 作为同一个左值传入 helper；二者都保持 callback 复用。
- L10：v2 行为与 Reference 一致。Reference 用 `consteval` 函数模板递归，盲写版用递归类模板承载检查，并由 `consteval parse_decimal()` 取值。

## 验证证据

构建目录：

- `C04_Generic_CompileTime_Reflection/exercises/build/revision-blind-good`

配置：

```text
cmake -S C04_Generic_CompileTime_Reflection\exercises -B C04_Generic_CompileTime_Reflection\exercises\build\revision-blind-good -A x64
```

结果：配置成功，生成器为 Visual Studio 18 2026，MSVC 19.51.36256.0。

构建：

```text
cmake --build C04_Generic_CompileTime_Reflection\exercises\build\revision-blind-good --target L07_packs_validation_good L08_type_lists_validation_good L09_tuple_validation_good L10_constexpr_validation_good --config Debug --parallel
```

结果：四个 `validation_good` 目标全部生成成功。

CTest：

```text
ctest --test-dir C04_Generic_CompileTime_Reflection\exercises\build\revision-blind-good -C Debug -R "^(L07_packs_validation_good|L08_type_lists_validation_good|L08_lazy_type_eager_bad|L09_tuple_validation_good|L10_constexpr_validation_good|L10_.*_diagnostic)$" --output-on-failure
```

结果：11/11 通过，0 失败。

覆盖项：

- L07 空包 identity、fold 方向、顺序调用、NTTP/fixed_string/template-template 应用。
- L08 `map/filter/concat/unique`、惰性 provider、完成签名保留与去重、不可调用 `value_sig` 概念拒绝。
- L09 空 tuple、左到右、callback 复用、mutable/const/rvalue tuple 借用、异常停止和副作用保留。
- L10 合法非空 ASCII 非负 int 解析、前导零、`INT_MAX`、临时 `std::vector` 常量求值、`if consteval` 运行时/编译期分支、非法字符/空串/NUL/符号/空白/溢出诊断。

## 未测项和边界

- 未运行全 C04 课程矩阵、ASan 或 F01 frontier capability probes。
- 未验证 `validation/bad` 负例目标，本任务只要求四个核心正确体与 L08/L10 diagnostic 边界。
- 未修改共享 `StudySetup.cmake` 或 core 脚本。第一次 CTest 因 VS 生成器 platform 为空导致 diagnostic 脚本参数缺失；已通过删除本任务专属构建目录并用 `-A x64` 重配规避。
